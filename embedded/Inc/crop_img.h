 /**
 ******************************************************************************
 * @file    crop_img.h
 * @author  PeleAB
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
#ifndef CROP_IMG
#define CROP_IMG
#include "arm_math.h"
#include "app_config.h"

/* Image processing function prototypes */

void img_rgb_to_chw_float(uint8_t *src_image, float32_t *dst_img,
                          const uint32_t src_stride, const uint16_t width,
                          const uint16_t height);

void img_rgb_to_chw_float_norm(uint8_t *src_image, float32_t *dst_img,
                          const uint32_t src_stride, const uint16_t width,
                          const uint16_t height);

void img_crop_resize(uint8_t *src_image, uint8_t *dst_img,
                     const uint16_t src_width, const uint16_t src_height,
                     const uint16_t dst_width, const uint16_t dst_height,
                     const uint16_t bpp, int x0, int y0,
                     int crop_width, int crop_height);

void img_crop_align(uint8_t *src_image, uint8_t *dst_img,
                    const uint16_t src_width, const uint16_t src_height,
                    const uint16_t dst_width, const uint16_t dst_height,
                    const uint16_t bpp, float x_center, float y_center,
                    float width, float height, float left_eye_x,
                    float left_eye_y, float right_eye_x, float right_eye_y);

void img_crop_align565_to_888(uint8_t *src_image, uint16_t src_stride,
                              uint8_t *dst_img,
                              const uint16_t src_width, const uint16_t src_height,
                              const uint16_t dst_width, const uint16_t dst_height,
                              float x_center, float y_center,
                              float width, float height, float left_eye_x,
                              float left_eye_y, float right_eye_x, float right_eye_y);

#endif /* CROP_IMG */
