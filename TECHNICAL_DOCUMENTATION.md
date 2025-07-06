# STM32N6 Object Detection System - Technical Documentation

## Overview

This project implements a real-time object detection and face recognition system using the STM32N6570-DK discovery board. The system leverages the NPU (Neural Processing Unit) for AI model inference, dual DCMIPP pipes for camera processing, and ISP for image enhancement.

## Architecture

### System Components

- **MCU**: STM32N6570 with Cortex-M55 core running at 600 MHz
- **NPU**: Hardware accelerated neural network processing unit
- **Camera Interface**: DCMIPP (Digital Camera Interface Pixel Pipeline) with dual pipes
- **Display**: LTDC (LCD-TFT Display Controller) with dual-layer support
- **External Memory**: 128MB PSRAM for frame buffers and AI model data
- **Communication**: UART streaming for PC integration

### Software Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Application Layer                        │
├─────────────────────────────────────────────────────────────┤
│  Face Detection  │  Face Recognition  │  Object Tracking   │
├─────────────────────────────────────────────────────────────┤
│              AI Runtime & NPU Interface                     │
├─────────────────────────────────────────────────────────────┤
│     Camera Middleware    │    Display Utils    │  PC Stream │
├─────────────────────────────────────────────────────────────┤
│                    HAL Drivers                              │
├─────────────────────────────────────────────────────────────┤
│                    Hardware Layer                           │
└─────────────────────────────────────────────────────────────┘
```

## Key Features

### 1. Face Detection and Recognition Pipeline

The system implements a sophisticated face detection and recognition pipeline with three distinct states:

#### State Machine
- **SEARCH**: Continuously scans for faces in the camera feed
- **VERIFY**: Performs face recognition on detected faces
- **TRACK**: Tracks verified faces and periodically re-verifies identity

#### Face Detection Model
- **Model**: CenterFace quantized integer model
- **Input**: 320x240 RGB images
- **Output**: Bounding boxes and confidence scores
- **Post-processing**: Non-maximum suppression and confidence thresholding

#### Face Recognition Model
- **Model**: MobileFaceNet quantized model
- **Input**: 96x112 RGB face crops
- **Output**: 128-dimensional embedding vectors
- **Similarity**: Cosine similarity with configurable threshold (0.55)

### 2. Camera Processing Pipeline

#### DCMIPP Configuration
- **Pipe 1**: Continuous mode for LCD display (full resolution)
- **Pipe 2**: Snapshot mode for AI inference (downscaled)
- **ISP**: Automatic image enhancement based on lighting conditions

#### Image Processing Flow
1. Raw camera data → DCMIPP → ISP enhancement
2. Pipe 1 → LCD frame buffer (continuous display)
3. Pipe 2 → AI inference buffer (on-demand capture)
4. Color space conversion (RGB to CHW format for neural networks)

### 3. Display System

#### Dual-Layer LTDC
- **Layer 0**: Background - live camera feed
- **Layer 1**: Overlay - detection boxes, labels, and UI elements

#### Visual Feedback
- Bounding boxes around detected faces
- Color-coded status indicators:
  - Green: Verified/authorized face
  - Red: Unverified/unauthorized face
  - Yellow: Tracking state
- Similarity scores and confidence levels

### 4. Object Tracking

#### Kalman Filter-based Tracking
- **State Vector**: [x, y, w, h, vx, vy, vw, vh]
- **Prediction**: Motion model for position and velocity
- **Update**: Measurement update from detections
- **Association**: Hungarian algorithm for track-detection matching

#### Track Management
- **Initialization**: New tracks from unassociated detections
- **Maintenance**: Age-based track pruning
- **Re-identification**: Face recognition for track verification

## Technical Specifications

### Memory Layout
- **SRAM**: 8MB for program execution and stack
- **PSRAM**: 128MB for frame buffers and AI model data
- **External Flash**: 128MB for program storage and AI model weights

### Performance Metrics
- **Camera FPS**: 30 FPS at full resolution
- **AI Inference**: ~100ms for face detection + recognition
- **Display Refresh**: 60 FPS with hardware acceleration
- **Power Consumption**: ~2W typical operation

### Neural Network Models

#### Face Detection (CenterFace)
- **Input Shape**: 1x3x240x320 (CHW format)
- **Quantization**: INT8 with calibration dataset
- **Inference Time**: ~50ms on NPU
- **Memory**: ~2MB model size

#### Face Recognition (MobileFaceNet)
- **Input Shape**: 1x3x112x96 (CHW format)
- **Quantization**: INT8 per-channel quantization
- **Inference Time**: ~30ms on NPU
- **Memory**: ~1MB model size

## Code Organization

### Core Modules

#### main.c
- Application entry point and main loop
- State machine implementation
- Integration of all subsystems

#### face_utils.c
- Face detection and recognition utilities
- Embedding computation and similarity calculation
- Face crop extraction and preprocessing

#### tracking.c
- Kalman filter implementation
- Track management and association
- Motion prediction and state estimation

#### app_cam.c
- Camera configuration and control
- DCMIPP and ISP setup
- Frame buffer management

#### display_utils.c
- Graphics rendering utilities
- Bounding box drawing
- Text overlay and UI elements

#### pc_stream.c
- UART communication protocol
- JPEG compression for streaming
- Detection result serialization

### Configuration

#### Build Options
- **Camera Module**: IMX335, VD55G1, or VD66GY support
- **Camera Orientation**: 0°, 90°, 180°, 270° rotation
- **Aspect Ratio**: Fullscreen, letterbox, or crop modes
- **Debug Level**: Runtime logging and profiling

#### Preprocessing Parameters
- **Input Resolution**: 320x240 for AI inference
- **Color Format**: RGB888
- **Normalization**: [0-255] → [0-1] scaling
- **Data Layout**: CHW (Channel-Height-Width) format

## Development Environment

### Required Tools
- **STM32CubeIDE 1.17.0**: Primary development environment
- **STM32CubeProgrammer 2.18.0**: Flash programming tool
- **STEdgeAI 2.0.0**: AI model optimization and quantization
- **ARM GCC Toolchain**: Cross-compilation toolchain

### Build Process
1. **Model Preparation**: Convert and quantize AI models using STEdgeAI
2. **Code Generation**: Generate C code from quantized models
3. **Compilation**: Build firmware using Make or STM32CubeIDE
4. **Programming**: Flash firmware and model weights to external memory

### Debugging and Profiling
- **ST-Link Debugger**: Real-time debugging and profiling
- **Serial Monitor**: Runtime logging and performance metrics
- **PC Client**: Visual debugging with live video stream

## PC Integration

### Python Tools
- **pc_uart_client.py**: GUI client for live video streaming
- **run_face_recognition.py**: Face enrollment and embedding generation
- **compare_detections.py**: Model validation and accuracy testing

### Communication Protocol
- **Baud Rate**: 921600 bps
- **Data Format**: Binary protocol with JPEG compression
- **Packet Structure**: Header + payload + checksum
- **Flow Control**: Hardware flow control for reliable transmission

## Known Limitations

### Hardware Constraints
- **Memory**: Limited by 128MB PSRAM for large models
- **Processing**: Real-time performance depends on model complexity
- **Power**: High power consumption during NPU operation

### Software Limitations
- **Color Format**: Only RGB888 input format supported
- **Quantization**: Only UINT8 quantization supported
- **Aspect Ratio**: Fullscreen mode has known issues

## Future Enhancements

### Planned Features
- **Multi-face Recognition**: Support for multiple enrolled faces
- **Gesture Recognition**: Integration of pose estimation models
- **Edge Learning**: On-device model fine-tuning capabilities
- **Security Features**: Encrypted model storage and secure boot

### Performance Optimizations
- **Model Compression**: Further quantization and pruning
- **Pipeline Optimization**: Parallel processing of detection and recognition
- **Memory Management**: Dynamic allocation and garbage collection

## Troubleshooting

### Common Issues
1. **Camera Not Detected**: Check camera module connection and compatibility
2. **NPU Errors**: Verify model quantization and memory allocation
3. **Display Issues**: Confirm LTDC configuration and frame buffer allocation
4. **PC Communication**: Check UART settings and cable connections

### Debug Procedures
1. **Serial Logging**: Enable debug output for runtime diagnostics
2. **Model Validation**: Use PC tools to verify model accuracy
3. **Performance Profiling**: Monitor inference times and memory usage
4. **Hardware Testing**: Verify camera and display functionality

This documentation provides a comprehensive overview of the STM32N6 Object Detection System, covering both technical implementation details and practical usage guidelines.