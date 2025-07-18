/**
******************************************************************************
* @file    crop_img_student.c
* @author  GPM Application Team (Student Implementation)
*
******************************************************************************
* @attention
*
* Copyright (c) 2024 STMicroelectronics.
* All rights reserved.
*
* This software is licensed under terms that can be found in the LICENSE file
* in the root directory of this software component.
* If no LICENSE file comes with this software, it is provided AS-IS.
*
******************************************************************************
*/

#include "crop_img_student.h"
#include <assert.h>
#include <math.h>
#include <string.h>

/* ========================================================================= */
/* STUDENT IMPLEMENTATION FUNCTIONS                                          */
/* ========================================================================= */
/* TODO: Implement these functions according to the specifications          */
/* ========================================================================= */

/**
 * @brief Basic image cropping function
 * TODO: Implement line-by-line copying from source to destination
 * HINT: Use memcpy to copy each line, calculate proper offsets
 */
void img_crop(uint8_t *src_image, uint8_t *dst_img, const uint32_t src_stride,
              const uint16_t dst_width, const uint16_t dst_height,
              const uint16_t bpp)
{
    /* TODO: Implement image cropping
     * Steps:
     * 1. Calculate destination line size: dst_width * bpp
     * 2. For each line in destination height:
     *    - Calculate source line offset: i * src_stride
     *    - Use memcpy to copy dst_line_size bytes
     *    - Advance destination pointer by dst_line_size
     */
    
    (void)src_image;  // Remove unused parameter warnings
    (void)dst_img;
    (void)src_stride;
    (void)dst_width;
    (void)dst_height;
    (void)bpp;
    
    // STUDENT IMPLEMENTATION GOES HERE
}


/**
 * @brief Convert RGB image to CHW float format
 * TODO: Convert uint8 RGB pixels to float32 in CHW layout
 * HINT: CHW means Channel x Height x Width (channel-first)
 */
void img_rgb_to_chw_float(uint8_t *src_image, float32_t *dst_img,
                          const uint32_t src_stride, const uint16_t width,
                          const uint16_t height)
{
    /* TODO: Convert RGB to CHW float
     * Steps:
     * 1. For each row (y = 0 to height-1):
     *    - Calculate source line pointer: src_image + y * src_stride
     * 2. For each pixel in row (x = 0 to width-1):
     *    - Red channel:   dst_img[y * width + x] = (float32_t)src_pixel[0]
     *    - Green channel: dst_img[height * width + y * width + x] = (float32_t)src_pixel[1]
     *    - Blue channel:  dst_img[2 * height * width + y * width + x] = (float32_t)src_pixel[2]
     *    - Advance source pointer by 3 bytes
     */
    
    (void)src_image;  // Remove unused parameter warnings
    (void)dst_img;
    (void)src_stride;
    (void)width;
    (void)height;
    
    // STUDENT IMPLEMENTATION GOES HERE
}

/**
 * @brief Convert RGB image to CHW float format with normalization
 * TODO: Convert uint8 RGB pixels to normalized float32 in CHW layout
 * HINT: Normalization formula: (pixel - 127.5) / 127.5
 */
void img_rgb_to_chw_float_norm(uint8_t *src_image, float32_t *dst_img,
                          const uint32_t src_stride, const uint16_t width,
                          const uint16_t height)
{
    /* TODO: Convert RGB to CHW float with normalization
     * Steps:
     * 1. Similar to img_rgb_to_chw_float but apply normalization
     * 2. For each channel: normalized_value = (pixel_value - 127.5f) / 127.5f
     * 3. This maps [0, 255] to [-1.0, 1.0]
     */
    
    (void)src_image;  // Remove unused parameter warnings
    (void)dst_img;
    (void)src_stride;
    (void)width;
    (void)height;
    
    // STUDENT IMPLEMENTATION GOES HERE
}


/**
 * @brief Crop and align image based on eye positions (ADVANCED)
 * TODO: Implement face alignment using eye positions
 * HINT: This requires rotation matrix calculations using atan2f, cosf, sinf
 */
void img_crop_align(uint8_t *src_image, uint8_t *dst_img,
                    const uint16_t src_width, const uint16_t src_height,
                    const uint16_t dst_width, const uint16_t dst_height,
                    const uint16_t bpp, float x_center, float y_center,
                    float width, float height, float left_eye_x,
                    float left_eye_y, float right_eye_x, float right_eye_y)
{
    /* TODO: ADVANCED - Implement face alignment with rotation
     * Steps:
     * 1. Calculate rotation angle: angle = -atan2f(right_eye_y - left_eye_y, right_eye_x - left_eye_x)
     * 2. Calculate cos_a = cosf(angle), sin_a = sinf(angle)
     * 3. For each destination pixel, apply inverse rotation to find source coordinates
     * 4. Sample source image with bounds checking
     * 
     * This is an advanced function - start with simpler functions first!
     */
    
    (void)src_image;  // Remove unused parameter warnings
    (void)dst_img;
    (void)src_width;
    (void)src_height;
    (void)dst_width;
    (void)dst_height;
    (void)bpp;
    (void)x_center;
    (void)y_center;
    (void)width;
    (void)height;
    (void)left_eye_x;
    (void)left_eye_y;
    (void)right_eye_x;
    (void)right_eye_y;
    
    // STUDENT IMPLEMENTATION GOES HERE
}

/**
 * @brief Crop and align image converting RGB565 to RGB888 (ADVANCED)
 * TODO: Implement format conversion with alignment
 * HINT: RGB565 format: 5 bits red, 6 bits green, 5 bits blue
 */
void img_crop_align565_to_888(uint8_t *src_image, uint16_t src_stride,
                              uint8_t *dst_img,
                              const uint16_t src_width, const uint16_t src_height,
                              const uint16_t dst_width, const uint16_t dst_height,
                              float x_center, float y_center,
                              float width, float height, float left_eye_x,
                              float left_eye_y, float right_eye_x, float right_eye_y)
{
    /* TODO: ADVANCED - Implement RGB565 to RGB888 conversion with alignment
     * Steps:
     * 1. Similar to img_crop_align but with format conversion
     * 2. RGB565 to RGB888 conversion:
     *    - Red:   ((pixel >> 11) & 0x1F) << 3
     *    - Green: ((pixel >> 5) & 0x3F) << 2
     *    - Blue:  (pixel & 0x1F) << 3
     * 
     * This is an advanced function combining rotation and format conversion!
     */
    
    (void)src_image;  // Remove unused parameter warnings
    (void)src_stride;
    (void)dst_img;
    (void)src_width;
    (void)src_height;
    (void)dst_width;
    (void)dst_height;
    (void)x_center;
    (void)y_center;
    (void)width;
    (void)height;
    (void)left_eye_x;
    (void)left_eye_y;
    (void)right_eye_x;
    (void)right_eye_y;
    
    // STUDENT IMPLEMENTATION GOES HERE
}
