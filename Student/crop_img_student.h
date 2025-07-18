/**
******************************************************************************
* @file    crop_img_student.h
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

#ifndef CROP_IMG_STUDENT_H
#define CROP_IMG_STUDENT_H

#include "arm_math.h"

/* ========================================================================= */
/* STUDENT IMPLEMENTATION FUNCTIONS                                          */
/* ========================================================================= */
/* These functions are for student implementation.                          */
/* Students should implement these functions according to specifications.   */
/* ========================================================================= */

/**
 * @brief Basic image cropping function
 * @param src_image Source image buffer
 * @param dst_img Destination image buffer
 * @param src_stride Source image stride (bytes per line)
 * @param dst_width Destination image width
 * @param dst_height Destination image height
 * @param bpp Bytes per pixel
 * @note Students should implement line-by-line copying with memcpy
 */
void img_crop(uint8_t *src_image, uint8_t *dst_img, const uint32_t src_stride,
              const uint16_t dst_width, const uint16_t dst_height,
              const uint16_t bpp);


/**
 * @brief Convert RGB image to CHW float format
 * @param src_image Source RGB image (uint8_t format)
 * @param dst_img Destination float image buffer
 * @param src_stride Source image stride
 * @param width Image width
 * @param height Image height
 * @note CHW format: Channel x Height x Width (channel-first)
 */
void img_rgb_to_chw_float(uint8_t *src_image, float32_t *dst_img,
                          const uint32_t src_stride, const uint16_t width,
                          const uint16_t height);

/**
 * @brief Convert RGB image to CHW float format with normalization
 * @param src_image Source RGB image (uint8_t format)
 * @param dst_img Destination float image buffer
 * @param src_stride Source image stride
 * @param width Image width
 * @param height Image height
 * @note Apply normalization: (pixel - 127.5) / 127.5
 */
void img_rgb_to_chw_float_norm(uint8_t *src_image, float32_t *dst_img,
                          const uint32_t src_stride, const uint16_t width,
                          const uint16_t height);

/**
 * @brief Crop and align image based on eye positions (advanced)
 * @param src_image Source image buffer
 * @param dst_img Destination image buffer
 * @param src_width Source image width
 * @param src_height Source image height
 * @param dst_width Destination image width
 * @param dst_height Destination image height
 * @param bpp Bytes per pixel
 * @param x_center Face center X coordinate
 * @param y_center Face center Y coordinate
 * @param width Face width
 * @param height Face height
 * @param left_eye_x Left eye X coordinate
 * @param left_eye_y Left eye Y coordinate
 * @param right_eye_x Right eye X coordinate
 * @param right_eye_y Right eye Y coordinate
 * @note Advanced function involving rotation based on eye alignment
 */
void img_crop_align(uint8_t *src_image, uint8_t *dst_img,
                    const uint16_t src_width, const uint16_t src_height,
                    const uint16_t dst_width, const uint16_t dst_height,
                    const uint16_t bpp, float x_center, float y_center,
                    float width, float height, float left_eye_x,
                    float left_eye_y, float right_eye_x, float right_eye_y);

/**
 * @brief Crop and align image converting RGB565 to RGB888 (advanced)
 * @param src_image Source image buffer (RGB565 format)
 * @param src_stride Source image stride
 * @param dst_img Destination image buffer (RGB888 format)
 * @param src_width Source image width
 * @param src_height Source image height
 * @param dst_width Destination image width
 * @param dst_height Destination image height
 * @param x_center Face center X coordinate
 * @param y_center Face center Y coordinate
 * @param width Face width
 * @param height Face height
 * @param left_eye_x Left eye X coordinate
 * @param left_eye_y Left eye Y coordinate
 * @param right_eye_x Right eye X coordinate
 * @param right_eye_y Right eye Y coordinate
 * @note Advanced function with format conversion and rotation
 */
void img_crop_align565_to_888(uint8_t *src_image, uint16_t src_stride,
                              uint8_t *dst_img,
                              const uint16_t src_width, const uint16_t src_height,
                              const uint16_t dst_width, const uint16_t dst_height,
                              float x_center, float y_center,
                              float width, float height, float left_eye_x,
                              float left_eye_y, float right_eye_x, float right_eye_y);

#endif /* CROP_IMG_STUDENT_H */
