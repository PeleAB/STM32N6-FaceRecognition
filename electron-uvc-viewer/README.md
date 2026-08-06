# STM32N6 Face Recognition Viewer

React, TypeScript, and Electron desktop UI for the STM32N6 face-recognition
firmware. It displays the H.264 UVC stream and communicates with the board over
the ST-LINK virtual COM port.

## Features

- low-latency live video with FFmpeg fallback on Windows
- UART packet log and board telemetry
- persistent parameter controls
- live camera enrollment with progress
- offline enrollment from multiple PC photos
- persistent gallery list, delete, and clear operations

## Requirements

- Node.js 20 or newer
- FFmpeg on `PATH` for the DirectShow fallback
- Python with `numpy`, `opencv-python`, and `onnxruntime` for photo enrollment

## Development

```bash
npm install
npm run dev
```

## Production build

```bash
npm install
npm run build
```

Connect the camera USB port and ST-LINK USB port before starting the app. In
the sidebar, select `STM32 uvc`, choose the ST-LINK COM port, and click
**Connect**.
