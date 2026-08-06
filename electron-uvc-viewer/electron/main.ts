import { app, BrowserWindow, session, ipcMain, dialog } from 'electron';
import path from 'path';
import { fileURLToPath } from 'url';
import http from 'http';
import { spawn, ChildProcess } from 'child_process';
import { execFile } from 'child_process';
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
            '-loglevel', 'warning',
            '-flags', 'low_delay',
            '-probesize', '1M',
            '-analyzeduration', '500000',
            '-rtbufsize', '512K',
            '-f', 'dshow',
            '-i', `video=${cameraName}`,
            '-an',
            '-c:v', 'mjpeg',
            '-pix_fmt', 'yuv420p',
            '-color_range', 'pc',
            '-threads', '1',
            '-f', 'mpjpeg',
            '-q:v', '5',
            '-flush_packets', '1',
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
    ipcMain.handle('gallery:create-from-photos', async () => {
        const selection = await dialog.showOpenDialog(mainWindow!, {
            title: 'Choose enrollment photos',
            properties: ['openFile', 'multiSelections'],
            filters: [{ name: 'Photos', extensions: ['jpg', 'jpeg', 'png', 'bmp', 'webp'] }]
        });
        if (selection.canceled || selection.filePaths.length === 0)
            return { success: false, canceled: true };
        const repoRoot = path.resolve(__dirname, '..', '..');
        const script = path.join(repoRoot, 'Tools', 'create_face_embedding.py');
        const modelDir = path.join(repoRoot, 'Model');
        return await new Promise(resolve => {
            execFile('python', [script, '--model-dir', modelDir, ...selection.filePaths],
                { maxBuffer: 1024 * 1024 }, (error, stdout, stderr) => {
                    if (error) {
                        resolve({ success: false, error: stderr.trim() || error.message });
                        return;
                    }
                    try {
                        const result = JSON.parse(stdout.trim());
                        resolve({ success: true, embeddingQ7: result.embeddingQ7,
                            photos: result.photos });
                    } catch (e: any) {
                        resolve({ success: false, error: `Invalid embedding output: ${e.message}` });
                    }
                });
        });
    });

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
