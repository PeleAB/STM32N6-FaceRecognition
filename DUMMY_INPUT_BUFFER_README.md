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

## Test Pattern Details

### Color Values
- **Sky**: RGB(135, 206, 235) - Light blue
- **Face Center**: RGB(255, 219, 172) - Light skin tone
- **Face Edges**: RGB(240, 200, 160) - Darker skin tone
- **Hair**: RGB(139, 69, 19) - Brown
- **Eyes**: RGB(0, 0, 0) - Black
- **Mouth**: RGB(220, 20, 60) - Pink
- **Neck/Background**: RGB(101, 67, 33) - Dark brown

### Geometric Layout
- **Image Size**: 128x128 pixels, 3 bytes per pixel (RGB)
- **Face Center**: (64, 64)
- **Face Radius**: ~35 pixels from center
- **Eyes**: Rows 55-65, columns 45-55 and 75-85
- **Mouth**: Rows 75-80, columns 55-75

## Usage Instructions

### Enable Dummy Input Buffer
1. Open `Inc/app_config.h`
2. Uncomment the line: `#define DUMMY_INPUT_BUFFER`
3. Rebuild the project

### Expected Output
When enabled, you should see console output like:
```
📸 PIPELINE STAGE 1: Frame Capture
🔄 Loading dummy input buffer (overriding camera input)...
🎯 Initializing dummy input buffer for testing...
✅ Dummy input buffer initialized with test pattern
✅ Dummy input loaded: 128x128 RGB image (49152 bytes)
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
- **Implementation**: `Src/main.c` (lines 149-236)
- **Usage**: `Src/main.c` (lines 904-907)

## Benefits
- **Consistent Testing**: Same input every time
- **Debugging**: Step through pipeline with known data
- **Verification**: Compare outputs with expected results
- **Learning**: Understand data transformations visually