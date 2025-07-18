# Student Mode Implementation

This directory contains student implementation stub files for the EdgeAI Workshop embedded project.

## Overview

When `STUDENT_MODE` is enabled, the build system uses the student implementation files instead of the complete instructor implementations. This allows students to:

- Learn key computer vision and AI algorithms by implementing them
- Understand mathematical concepts like cosine similarity, IoU, and image processing
- Practice embedded programming in C
- Build working knowledge of face detection and recognition systems

## Files

### 1. Image Processing (`crop_img_student.c/.h`)
Functions to implement:
- `img_crop()` - Basic image cropping
- `img_rgb_to_hwc_float()` - RGB to HWC float conversion
- `img_rgb_to_chw_float()` - RGB to CHW float conversion
- `img_rgb_to_chw_float_norm()` - RGB to CHW with normalization
- `img_rgb_to_chw_s8()` - RGB to CHW signed 8-bit
- `img_crop_resize()` - Crop and resize with nearest neighbor
- `img_crop_align()` - Advanced: Face alignment with rotation (optional)
- `img_crop_align565_to_888()` - Advanced: RGB565 to RGB888 conversion (optional)

**Difficulty**: Beginner to Advanced  
**Key concepts**: Image formats, memory layouts, mathematical transformations

### 2. Face Recognition Utils (`face_utils_student.c/.h`)
Functions to implement:
- `embedding_cosine_similarity()` - Calculate cosine similarity between face embeddings

**Difficulty**: Beginner  
**Key concepts**: Vector mathematics, cosine similarity, face recognition

### 3. Embedding Management (`target_embedding_student.c/.h`)
Functions to implement:
- `embeddings_bank_init()` - Initialize embedding bank
- `embeddings_bank_add()` - Add normalized embedding to bank
- `embeddings_bank_reset()` - Reset the bank
- `embeddings_bank_count()` - Get current count
- `compute_target()` - Compute average embedding (private function)

**Difficulty**: Intermediate  
**Key concepts**: Vector normalization, averaging, memory management

## How to Use

### 1. Enable Student Mode
In `Inc/app_config.h`, ensure this line is uncommented:
```c
#define STUDENT_MODE
```

### 2. Build with Student Mode
```bash
make STUDENT_MODE=1
```

### 3. Disable Student Mode
Comment out the define in `Inc/app_config.h`:
```c
// #define STUDENT_MODE
```

Or build without the flag:
```bash
make STUDENT_MODE=0
```

## Implementation Guidelines

### Start with Easy Functions
1. Begin with `face_utils_student.c` - single function, clear mathematical formula
2. Move to `target_embedding_student.c` - vector operations and averaging
3. Try basic image processing in `crop_img_student.c`

### Debugging Tips
- Use printf statements for debugging (ITM printf is available)
- Start with simple test cases
- Check boundary conditions (NULL pointers, zero dimensions)
- Verify mathematical operations step by step

### Key Constants
Available in `app_constants.h`:
- `EMBEDDING_SIZE` - Size of face embedding vectors (128)
- `EMBEDDING_BANK_SIZE` - Maximum embeddings in bank (10)

## Expected Outputs

When correctly implemented, the system will:
1. Detect faces in camera frames
2. Recognize faces by comparing embeddings
3. Display results on LCD and stream to PC

## Testing

The workshop includes test images and scenarios to verify your implementations work correctly with the complete face detection and recognition pipeline.

## Help

Each stub file includes:
- Detailed TODO comments explaining the algorithm
- Step-by-step implementation hints
- Mathematical formulas and concepts
- Parameter explanations

Good luck with your implementations!