import { app, BrowserWindow, session, ipcMain } from 'electron';
import path from 'path';
import { fileURLToPath } from 'url';
import http from 'http';
import { spawn, ChildProcess } from 'child_process';
import { SerialPort } from 'serialport';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

// Server to handle H.264 camera streams from DirectShow because Chromium depreciated WebRTC hardware endpoints.
let activeFfmpegProcesses: ChildProcess[] = [];
const streamServer = http.createServer((req, res) => {
    if (req.url?.startsWith('/stream')) {
        // Clear out any old streams immediately.
        activeFfmpegProcesses.forEach(p => p.kill('SIGKILL'));
        activeFfmpegProcesses = [];

        const urlParams = new URL(req.url, `http://${req.headers.host}`);
        const cameraName = urlParams.searchParams.get('camera') || 'STM32 uvc';

        res.writeHead(200, {
            'Content-Type': 'multipart/x-mixed-replace;boundary=ffmpeg',
            'Cache-Control': 'no-cache, no-store, must-revalidate',
            'Connection': 'close',
            'Pragma': 'no-cache',
            'Expires': '0',
            'Access-Control-Allow-Origin': '*'
        });

        const localFfmpeg = spawn('ffmpeg', [
            '-hide_banner',
            '-f', 'dshow',
            '-i', `video=${cameraName}`,
            '-f', 'mpjpeg',
            '-q:v', '5',
            '-'
        ]);

        activeFfmpegProcesses.push(localFfmpeg);

        localFfmpeg.stdout?.pipe(res);

        localFfmpeg.stderr?.on('data', (data) => {
            console.error(`ffmpeg stderr: ${data}`);
        });

        localFfmpeg.on('error', (err) => {
            console.error('Failed to start ffmpeg:', err);
        });

        req.on('close', () => {
            localFfmpeg.kill('SIGKILL');
            activeFfmpegProcesses = activeFfmpegProcesses.filter(p => p !== localFfmpeg);
        });
    } else {
        res.writeHead(404);
        res.end();
    }
});
streamServer.listen(8089, '127.0.0.1');

let mainWindow: BrowserWindow | null = null;

function createWindow() {
    const win = new BrowserWindow({
        width: 1200,
        height: 800,
        webPreferences: {
            preload: path.join(__dirname, 'preload.js'),
            contextIsolation: true,
            nodeIntegration: true,
        }
    });

    mainWindow = win;

    // Automatically grant permission for media streaming (UVC camera)
    session.defaultSession.setPermissionRequestHandler((webContents, permission, callback) => {
        if (permission === 'media') {
            callback(true);
        } else {
            callback(false);
        }
    });

    if (process.env.VITE_DEV_SERVER_URL) {
        win.loadURL(process.env.VITE_DEV_SERVER_URL);
        win.webContents.openDevTools();
    } else {
        win.loadFile(path.join(__dirname, '../dist/index.html'));
    }
}

let currentPort: any = null;

app.whenReady().then(() => {
    ipcMain.handle('serial:list', async () => {
        try {
            const ports = await SerialPort.list();
            return ports;
        } catch (e: any) {
            console.error('Error listing ports', e);
            return [];
        }
    });

    ipcMain.handle('serial:connect', async (_event, path: string, baudRate: number) => {
        return new Promise((resolve) => {
            if (currentPort && currentPort.isOpen) {
                currentPort.close();
            }

            try {
                currentPort = new SerialPort({ path, baudRate, autoOpen: false });
                currentPort.open((err: any) => {
                    if (err) {
                        resolve({ success: false, error: err.message });
                    } else {
                        currentPort.on('data', (data: Buffer) => {
                            if (mainWindow) {
                                mainWindow.webContents.send('serial:data', { type: 'in', data: new Uint8Array(data) });
                            }
                        });
                        resolve({ success: true });
                    }
                });
            } catch (err: any) {
                resolve({ success: false, error: err.message });
            }
        });
    });

    ipcMain.handle('serial:disconnect', async () => {
        return new Promise((resolve) => {
            if (currentPort && currentPort.isOpen) {
                currentPort.close((err: any) => {
                    if (err) resolve({ success: false, error: err.message });
                    else resolve({ success: true });
                });
            } else {
                resolve({ success: true });
            }
        });
    });

    ipcMain.handle('serial:send', async (_event, data: Uint8Array) => {
        return new Promise((resolve) => {
            if (!currentPort || !currentPort.isOpen) {
                resolve({ success: false, error: 'Port not open' });
                return;
            }
            // IPC strips `Uint8Array` prototypes, turning it into `{0: 170, 1: ...}`.
            // Buffer.from() drops this silently resulting in empty transmissions (or null-fill).
            const isPlainObject = typeof data === 'object' && !Array.isArray(data) && !(data instanceof Buffer) && !(data instanceof Uint8Array);
            const bytesArray = isPlainObject ? Object.values(data) : data;
            const buf = Buffer.from(bytesArray as any);
            console.log("SENDING EXACT BYTES OVER SERIAL:", buf);

            currentPort.write(buf, (err: any) => {
                if (err) resolve({ success: false, error: err.message });
                else {
                    if (mainWindow) {
                        mainWindow.webContents.send('serial:data', { type: 'out', data: new Uint8Array(data) });
                    }
                    resolve({ success: true });
                }
            });
        });
    });

    createWindow();

    app.on('activate', () => {
        if (BrowserWindow.getAllWindows().length === 0) {
            createWindow();
        }
    });
});

app.on('window-all-closed', () => {
    if (process.platform !== 'darwin') {
        app.quit();
    }
});
