# STM32N6 Face Recognition UI

Modern DearPyGui-based interface for monitoring and controlling the STM32N6 Face Recognition system.

## Features

- **Real-time Video Display**: Live video feed with face detection overlays
- **Protocol Monitoring**: Statistics and diagnostics for serial communication
- **Face Detection Visualization**: Bounding boxes and confidence scores
- **Embedding Tracking**: Monitor face recognition embeddings
- **Frame History**: View recent frame statistics and compression ratios
- **Modern UI**: Clean, responsive interface built with DearPyGui

## Quick Start

1. **Install Dependencies**:
   ```bash
   pip install -r requirements.txt
   ```

2. **Run the UI**:
   ```bash
   python run_ui.py
   ```
   
   Or directly:
   ```bash
   python face_recognition_ui.py
   ```

## Requirements

- Python 3.8+
- DearPyGui 1.11.0+
- OpenCV 4.8.0+
- NumPy 1.24.0+
- PySerial 3.5+

## UI Layout

### Connection Panel
- **Port Selection**: Dropdown with available serial ports
- **Baud Rate**: Configurable baud rate (default: 7,372,800)
- **Connect/Disconnect**: Connection controls

### Status Panel
- **Connection Status**: Real-time connection state
- **Frame Counter**: Total frames received
- **FPS Display**: Current frames per second
- **Detection/Embedding Counters**: Live statistics

### Video Display
- **Live Feed**: 640x480 video display
- **Detection Overlay**: Optional face detection boxes
- **Controls**: Toggle detection and embedding display

### Data Panels

#### Protocol Statistics
- Messages received/sent
- Bytes transferred
- Error counts (CRC, sync)
- Throughput monitoring

#### Frame History
- Recent frame information
- Compression ratios
- Frame types and sizes
- Timestamps

#### Embeddings
- Recent face embeddings
- Dimension information
- Preview of embedding values

## Protocol Support

The UI uses the STM32N6 serial protocol with:
- **Message Framing**: SOF byte + header + payload + CRC32
- **Error Detection**: CRC32 validation and header checksums
- **Message Types**: Frames, detections, embeddings, metrics, heartbeat
- **Robust Parsing**: Error recovery and sync detection

## Configuration

The UI automatically detects available serial ports and uses sensible defaults:
- **Default Baud Rate**: 7,372,800 (matches embedded side)
- **Buffer Size**: 256KB for high-throughput data
- **Display Rate**: 60 FPS UI updates
- **History Depth**: 100 frames, 10 embeddings

## Troubleshooting

### No Video Signal
- Check serial connection
- Verify baud rate matches embedded side
- Ensure embedded firmware is running

### Connection Issues
- Check port permissions (Linux: add user to dialout group)
- Verify cable connection
- Try different baud rates

### Performance Issues
- Reduce video resolution on embedded side
- Check CPU usage
- Verify USB cable quality

## Development

### Adding New Features
1. **Protocol Messages**: Extend `serial_protocol.py`
2. **UI Elements**: Modify `face_recognition_ui.py`
3. **Handlers**: Add message handlers for new data types

### Code Structure
```
face_recognition_ui.py    # Main UI application
serial_protocol.py       # Protocol parser and message handling
run_ui.py                # Launcher with dependency checking
requirements.txt         # Python dependencies
```

### Message Flow
1. Serial data → `SerialProtocolParser`
2. Parsed messages → UI handlers
3. UI updates → DearPyGui rendering
4. User interactions → Serial commands (future)

## License

Part of the STM32N6 Face Recognition project. See main project LICENSE.