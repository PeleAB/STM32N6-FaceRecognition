# Student Mode Implementation

This project includes a student mode system that allows students to implement key computer vision and AI algorithms while learning embedded programming.

## How Student Mode Works

The student mode uses conditional compilation (`#ifdef STUDENT_MODE`) to switch between:
- **Student implementations**: Stub functions with detailed TODO comments and implementation hints
- **Instructor implementations**: Complete, working functions

## Files with Student Mode

### 1. Image Processing (`Src/crop_img.c`)
Functions to implement:
- `img_crop()` - Basic image cropping
- `img_rgb_to_chw_float()` - RGB to CHW float conversion
- `img_rgb_to_chw_float_norm()` - RGB to CHW with normalization
- `img_crop_resize()` - Crop and resize with nearest neighbor
- `img_crop_align()` - Advanced: Face alignment with rotation (optional)
- `img_crop_align565_to_888()` - Advanced: RGB565 to RGB888 conversion (optional)

**Difficulty**: Beginner to Advanced  
**Key concepts**: Image formats, memory layouts, mathematical transformations

### 2. Face Recognition Utils (`Src/face_utils.c`)
Functions to implement:
- `embedding_cosine_similarity()` - Calculate cosine similarity between face embeddings

**Difficulty**: Beginner  
**Key concepts**: Vector mathematics, cosine similarity, face recognition

### 3. Embedding Management (`Src/target_embedding.c`)
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
make clean
make STUDENT_MODE=1
```

### 3. Disable Student Mode
Comment out the define in `Inc/app_config.h`:
```c
// #define STUDENT_MODE
```

Or build without the flag:
```bash
make clean
make STUDENT_MODE=0
```

## Implementation Guidelines

### Start with Easy Functions
1. Begin with `face_utils.c` - single function, clear mathematical formula
2. Move to `target_embedding.c` - vector operations and averaging
3. Try basic image processing in `crop_img.c`

### Debugging Tips
- Use printf statements for debugging (ITM printf is available)
- Start with simple test cases
- Check boundary conditions (NULL pointers, zero dimensions)
- Verify mathematical operations step by step

### Key Constants
Available in `Inc/target_embedding.h`:
- `EMBEDDING_SIZE` - Size of face embedding vectors (128)
- `EMBEDDING_BANK_SIZE` - Maximum embeddings in bank (10)

## Expected Outputs

When correctly implemented, the system will:
1. Detect faces in camera frames
2. Recognize faces by comparing embeddings
3. Display results on LCD and stream to PC

## Implementation Structure

Each function includes:
- Detailed TODO comments explaining the algorithm
- Step-by-step implementation hints
- Mathematical formulas and concepts
- Parameter explanations
- Unused parameter warnings to avoid compiler warnings

## Testing

The workshop includes test images and scenarios to verify your implementations work correctly with the complete face detection and recognition pipeline.

## Educational Benefits

This approach allows students to:
- Learn by implementing algorithms rather than just reading code
- Understand mathematical concepts through practical application
- Practice embedded programming in C
- Build working knowledge of computer vision systems
- Experience the complete development cycle from stub to working code

Good luck with your implementations!