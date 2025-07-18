# Dummy Input Buffer System

This document explains the dummy input buffer system designed to help students test their implementations with consistent, known input data.

## Overview

The dummy input buffer system provides a predefined test image that overrides the camera/PC stream input. This allows students to:
- Test their implementations with consistent input data
- Debug their functions step by step
- Verify their output matches expected results
- Understand the data flow through the processing pipeline

## How It Works

### 1. Configuration
The dummy input buffer is controlled by the `DUMMY_INPUT_BUFFER` define in `Inc/app_config.h`:

```c
#define DUMMY_INPUT_BUFFER  // Enable dummy input buffer
// #define DUMMY_INPUT_BUFFER  // Disable dummy input buffer
```

### 2. Test Pattern Generation
The system generates a simple face-like test pattern (128x128 RGB) with:
- **Sky/background area** (top 32 rows): Light blue background
- **Face area** (middle 64 rows): Skin tone with simple features
  - Face center: Light skin tone
  - Face edges: Slightly darker skin tone
  - Eyes: Two black rectangular areas
  - Mouth: Small pink rectangular area
  - Hair: Brown background around face
- **Neck/lower area** (bottom 32 rows): Darker background

### 3. Pipeline Integration
The dummy input buffer is loaded at **Step 1.1.5** in the `pipeline_stage_capture_and_preprocess` function:

```c
/* Step 1.1: Capture frame from camera or PC stream */
if (app_get_frame(nn_rgb, pitch_nn) != 0) {
    printf("❌ Frame capture failed\n");
    return -1;
}

#ifdef DUMMY_INPUT_BUFFER
/* Step 1.1.5: Override input with dummy buffer for testing */
load_dummy_input_buffer();
#endif

/* Step 1.2: Convert RGB to neural network input format */
```

This ensures that:
- The camera still functions normally (Step 1.1)
- The dummy data overrides the camera input before processing (Step 1.1.5)
- Student implementations receive consistent test data (Step 1.2+)

## Test Image Details

### Real Image Data
The system now uses actual image data from **trump2.jpg** instead of synthetic patterns:
- **Source**: `/Exercises/SamplePics/trump2.jpg`
- **Processing**: Resized to 128x128 pixels using high-quality Lanczos resampling
- **Format**: RGB (3 bytes per pixel)
- **Total Size**: 49,152 bytes (128 × 128 × 3)

### Image Statistics
- **Color Range**: 
  - Red: 1-242 (mean: 126.2)
  - Green: 0-246 (mean: 96.6)
  - Blue: 0-250 (mean: 78.6)
- **Content**: Real face image with natural lighting, skin tones, and features
- **Quality**: High-quality resize maintains facial features for accurate testing

## Usage Instructions

### Enable Dummy Input Buffer
1. Open `Inc/app_config.h`
2. Uncomment the line: `#define DUMMY_INPUT_BUFFER`
3. Rebuild the project

### Expected Output
When enabled, you should see console output like:
```
📸 PIPELINE STAGE 1: Frame Capture
🔄 Loading dummy input buffer (trump2.jpg, 128x128)...
✅ Real image loaded: 128x128 RGB image (49152 bytes)
   🔄 Converting RGB to CHW format for neural network...
```

### Testing Student Implementations
Students can now test their implementations with this consistent input:

1. **img_rgb_to_chw_float()**: Should convert the RGB test pattern to CHW float format
2. **img_crop_align()**: Should process the face area correctly
3. **embedding_cosine_similarity()**: Should work with normalized embeddings
4. **Face detection**: Should detect the simple face pattern

## Ground Truth Data
Future versions will include expected output data for each processing stage, allowing students to verify their implementations produce correct results.

## Implementation Files
- **Configuration**: `Inc/app_config.h`
- **Buffer Header**: `trump2_buffer.h`
- **Buffer Data**: `trump2_buffer.c` (298KB of image data)
- **Implementation**: `Src/main.c` (lines 150-171)
- **Usage**: `Src/main.c` (lines 904-907)
- **Build System**: `Makefile` (trump2_buffer.c added to C_SOURCES)

## Benefits
- **Consistent Testing**: Same input every time
- **Debugging**: Step through pipeline with known data
- **Verification**: Compare outputs with expected results
- **Learning**: Understand data transformations visually