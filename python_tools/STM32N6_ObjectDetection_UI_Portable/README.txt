# STM32N6 Object Detection UI - Portable Version

## Quick Start

1. **Connect your STM32N6570-DK board** to your computer via USB
2. **Run the application** by double-clicking `STM32N6_ObjectDetection_UI.exe`
3. **Select the correct COM port** from the dropdown
4. **Choose the baud rate** (default: 7372800)
5. **Click Connect** to start receiving data

## Features

- **Real-time image display** from STM32N6 camera
- **Serial communication** with embedded system
- **Performance statistics** (FPS, frame count, uptime)
- **Modern dark theme** interface
- **Minimal dependencies** - no TensorFlow required

## System Requirements

- **Windows 10/11** (64-bit)
- **Available USB/Serial port** for STM32N6 connection
- **No additional software installation required**

## Troubleshooting

### No COM ports detected
- Ensure STM32N6570-DK is connected via USB
- Install STM32 Virtual COM Port drivers if needed
- Check Windows Device Manager for COM ports

### Connection fails
- Verify correct COM port selection
- Try different baud rates (921600, 1843200, 3686400, 7372800)
- Check cable connections
- Ensure embedded firmware is running

### No image display
- Verify embedded system is streaming data
- Check serial communication logs in the application
- Ensure correct protocol compatibility

## Support

For issues and updates, visit the project repository.

Generated with Claude Code - STM32N6 Object Detection Project
