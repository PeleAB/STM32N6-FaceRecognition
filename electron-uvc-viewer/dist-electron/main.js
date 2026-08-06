import { BrowserWindow, app, dialog, ipcMain, session } from "electron";
import path from "path";
import { fileURLToPath } from "url";
import http from "http";
import { execFile, spawn } from "child_process";
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
			"-loglevel",
			"warning",
			"-flags",
			"low_delay",
			"-probesize",
			"1M",
			"-analyzeduration",
			"500000",
			"-rtbufsize",
			"512K",
			"-f",
			"dshow",
			"-i",
			`video=${cameraName}`,
			"-an",
			"-c:v",
			"mjpeg",
			"-pix_fmt",
			"yuv420p",
			"-color_range",
			"pc",
			"-threads",
			"1",
			"-f",
			"mpjpeg",
			"-q:v",
			"5",
			"-flush_packets",
			"1",
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
	ipcMain.handle("gallery:create-from-photos", async () => {
		const selection = await dialog.showOpenDialog(mainWindow, {
			title: "Choose enrollment photos",
			properties: ["openFile", "multiSelections"],
			filters: [{
				name: "Photos",
				extensions: [
					"jpg",
					"jpeg",
					"png",
					"bmp",
					"webp"
				]
			}]
		});
		if (selection.canceled || selection.filePaths.length === 0) return {
			success: false,
			canceled: true
		};
		const repoRoot = path.resolve(__dirname, "..", "..");
		const script = path.join(repoRoot, "Tools", "create_face_embedding.py");
		const modelDir = path.join(repoRoot, "Model");
		return await new Promise((resolve) => {
			execFile("python", [
				script,
				"--model-dir",
				modelDir,
				...selection.filePaths
			], { maxBuffer: 1024 * 1024 }, (error, stdout, stderr) => {
				if (error) {
					resolve({
						success: false,
						error: stderr.trim() || error.message
					});
					return;
				}
				try {
					const result = JSON.parse(stdout.trim());
					resolve({
						success: true,
						embeddingQ7: result.embeddingQ7,
						photos: result.photos
					});
				} catch (e) {
					resolve({
						success: false,
						error: `Invalid embedding output: ${e.message}`
					});
				}
			});
		});
	});
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
