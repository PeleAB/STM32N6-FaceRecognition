import { BrowserWindow, app, ipcMain, session } from "electron";
import path from "path";
import { fileURLToPath } from "url";
import http from "http";
import { spawn } from "child_process";
import { SerialPort } from "serialport";
//#region electron/main.ts
var __filename = fileURLToPath(import.meta.url);
var __dirname = path.dirname(__filename);
var activeFfmpegProcesses = [];
http.createServer((req, res) => {
	if (req.url?.startsWith("/stream")) {
		activeFfmpegProcesses.forEach((p) => p.kill("SIGKILL"));
		activeFfmpegProcesses = [];
		const cameraName = new URL(req.url, `http://${req.headers.host}`).searchParams.get("camera") || "STM32 uvc";
		res.writeHead(200, {
			"Content-Type": "multipart/x-mixed-replace;boundary=ffmpeg",
			"Cache-Control": "no-cache, no-store, must-revalidate",
			"Connection": "close",
			"Pragma": "no-cache",
			"Expires": "0",
			"Access-Control-Allow-Origin": "*"
		});
		const localFfmpeg = spawn("ffmpeg", [
			"-hide_banner",
			"-f",
			"dshow",
			"-i",
			`video=${cameraName}`,
			"-f",
			"mpjpeg",
			"-q:v",
			"5",
			"-"
		]);
		activeFfmpegProcesses.push(localFfmpeg);
		localFfmpeg.stdout?.pipe(res);
		localFfmpeg.stderr?.on("data", (data) => {
			console.error(`ffmpeg stderr: ${data}`);
		});
		localFfmpeg.on("error", (err) => {
			console.error("Failed to start ffmpeg:", err);
		});
		req.on("close", () => {
			localFfmpeg.kill("SIGKILL");
			activeFfmpegProcesses = activeFfmpegProcesses.filter((p) => p !== localFfmpeg);
		});
	} else {
		res.writeHead(404);
		res.end();
	}
}).listen(8089, "127.0.0.1");
var mainWindow = null;
function createWindow() {
	const win = new BrowserWindow({
		width: 1200,
		height: 800,
		webPreferences: {
			preload: path.join(__dirname, "preload.js"),
			contextIsolation: true,
			nodeIntegration: true
		}
	});
	mainWindow = win;
	session.defaultSession.setPermissionRequestHandler((webContents, permission, callback) => {
		if (permission === "media") callback(true);
		else callback(false);
	});
	if (process.env.VITE_DEV_SERVER_URL) {
		win.loadURL(process.env.VITE_DEV_SERVER_URL);
		win.webContents.openDevTools();
	} else win.loadFile(path.join(__dirname, "../dist/index.html"));
}
var currentPort = null;
app.whenReady().then(() => {
	ipcMain.handle("serial:list", async () => {
		try {
			return await SerialPort.list();
		} catch (e) {
			console.error("Error listing ports", e);
			return [];
		}
	});
	ipcMain.handle("serial:connect", async (_event, path, baudRate) => {
		return new Promise((resolve) => {
			if (currentPort && currentPort.isOpen) currentPort.close();
			try {
				currentPort = new SerialPort({
					path,
					baudRate,
					autoOpen: false
				});
				currentPort.open((err) => {
					if (err) resolve({
						success: false,
						error: err.message
					});
					else {
						currentPort.on("data", (data) => {
							if (mainWindow) mainWindow.webContents.send("serial:data", {
								type: "in",
								data: new Uint8Array(data)
							});
						});
						resolve({ success: true });
					}
				});
			} catch (err) {
				resolve({
					success: false,
					error: err.message
				});
			}
		});
	});
	ipcMain.handle("serial:disconnect", async () => {
		return new Promise((resolve) => {
			if (currentPort && currentPort.isOpen) currentPort.close((err) => {
				if (err) resolve({
					success: false,
					error: err.message
				});
				else resolve({ success: true });
			});
			else resolve({ success: true });
		});
	});
	ipcMain.handle("serial:send", async (_event, data) => {
		return new Promise((resolve) => {
			if (!currentPort || !currentPort.isOpen) {
				resolve({
					success: false,
					error: "Port not open"
				});
				return;
			}
			const bytesArray = typeof data === "object" && !Array.isArray(data) && !(data instanceof Buffer) && !(data instanceof Uint8Array) ? Object.values(data) : data;
			const buf = Buffer.from(bytesArray);
			console.log("SENDING EXACT BYTES OVER SERIAL:", buf);
			currentPort.write(buf, (err) => {
				if (err) resolve({
					success: false,
					error: err.message
				});
				else {
					if (mainWindow) mainWindow.webContents.send("serial:data", {
						type: "out",
						data: new Uint8Array(data)
					});
					resolve({ success: true });
				}
			});
		});
	});
	createWindow();
	app.on("activate", () => {
		if (BrowserWindow.getAllWindows().length === 0) createWindow();
	});
});
app.on("window-all-closed", () => {
	if (process.platform !== "darwin") app.quit();
});
//#endregion
