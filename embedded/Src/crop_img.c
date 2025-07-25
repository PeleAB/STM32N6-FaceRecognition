 /**
 ******************************************************************************
 * @file    crop_img.c
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
#include "crop_img.h"
#include <assert.h>
#include <math.h>
#include <string.h>
#include "dummy_dual_buffer.h"

/* ========================================================================= */
/* IMPLEMENTATION FUNCTIONS                                                 */
/* ========================================================================= */

void img_rgb_to_chw_float(uint8_t *src_image, float32_t *dst_img,
                          const uint32_t src_stride, const uint16_t width,
                          const uint16_t height)
{
  /* CHW layout: channel-first order */
  /* Optimized: use channel pointers to reduce address calculations */
  const uint32_t channel_size = height * width;
  float32_t *r_channel = dst_img;
  float32_t *g_channel = dst_img + channel_size;
  float32_t *b_channel = dst_img + 2 * channel_size;
  
  for (uint16_t y = 0; y < height; y++)
  {
    const uint8_t *pIn = src_image + y * src_stride;
    const uint32_t row_offset = y * width;
    
    for (uint16_t x = 0; x < width; x++)
    {
      const uint32_t pixel_idx = row_offset + x;
      r_channel[pixel_idx] = (float32_t)pIn[0];
      g_channel[pixel_idx] = (float32_t)pIn[1];
      b_channel[pixel_idx] = (float32_t)pIn[2];
      pIn += 3;
    }
  }
}

void img_rgb_to_chw_float_norm(uint8_t *src_image, float32_t *dst_img,
                          const uint32_t src_stride, const uint16_t width,
                          const uint16_t height)
{
  /* CHW layout: channel-first order */
  /* Optimized: precompute constants and unroll inner loop */
  const float scale = 1.0f / 127.5f;
  const uint32_t channel_size = height * width;
  float32_t *r_channel = dst_img;
  float32_t *g_channel = dst_img + channel_size;
  float32_t *b_channel = dst_img + 2 * channel_size;
  
  for (uint16_t y = 0; y < height; y++)
  {
    const uint8_t *pIn = src_image + y * src_stride;
    const uint32_t row_offset = y * width;
    
    for (uint16_t x = 0; x < width; x++)
    {
      const uint32_t pixel_idx = row_offset + x;
      /* Unrolled loop with precomputed scaling: (pixel / 127.5) - 1.0 */
      r_channel[pixel_idx] = ((float32_t)pIn[0]) * scale - 1.0f;
      g_channel[pixel_idx] = ((float32_t)pIn[1]) * scale - 1.0f;
      b_channel[pixel_idx] = ((float32_t)pIn[2]) * scale - 1.0f;
      pIn += 3;
    }
  }
}

void img_crop_resize(uint8_t *src_image, uint8_t *dst_img,
                     const uint16_t src_width, const uint16_t src_height,
                     const uint16_t dst_width, const uint16_t dst_height,
                     const uint16_t bpp, int x0, int y0,
                     int crop_width, int crop_height)
{
  /* Optimized version with memcpy and reduced bounds checking */
  const int src_width_limit = src_width - 1;
  const int src_height_limit = src_height - 1;
  
  for (int y = 0; y < dst_height; y++)
  {
    int src_y = y0 + (y * crop_height) / dst_height;
    if (src_y < 0) src_y = 0;
    if (src_y > src_height_limit) src_y = src_height_limit;
    
    const uint8_t *src_row = src_image + src_y * src_width * bpp;
    uint8_t *dst_row = dst_img + y * dst_width * bpp;
    
    for (int x = 0; x < dst_width; x++)
    {
      int src_x = x0 + (x * crop_width) / dst_width;
      if (src_x < 0) src_x = 0;
      if (src_x > src_width_limit) src_x = src_width_limit;
      
      const uint8_t *pIn = src_row + src_x * bpp;
      uint8_t *pOut = dst_row + x * bpp;
      
      /* Use memcpy for multi-byte pixel copy, faster than loop */
      memcpy(pOut, pIn, bpp);
    }
  }
}

void img_crop_align(uint8_t *src_image, uint8_t *dst_img,
                    const uint16_t src_width, const uint16_t src_height,
                    const uint16_t dst_width, const uint16_t dst_height,
                    const uint16_t bpp, float x_center, float y_center,
                    float width, float height, float left_eye_x,
                    float left_eye_y, float right_eye_x, float right_eye_y)
{
  /* Optimized: precompute all constants outside loops */
  const float angle = -atan2f(right_eye_y - left_eye_y, right_eye_x - left_eye_x);
  const float cos_a = cosf(angle);
  const float sin_a = sinf(angle);
  const float dst_full = (dst_width > dst_height) ? (float)dst_width : (float)dst_height;
  const float offset_x = (dst_full - (float)dst_width) * 0.5f;
  const float offset_y = (dst_full - (float)dst_height) * 0.5f;
  const float inv_dst_full = 1.0f / dst_full;
  const float width_cos = width * cos_a;
  const float width_sin = width * sin_a;
  const float height_cos = height * cos_a;
  const float height_sin = height * sin_a;
  const float src_width_limit = (float)(src_width - 1);
  const float src_height_limit = (float)(src_height - 1);

  for (uint16_t y = 0; y < dst_height; y++)
  {
    const float ny = (((float)y + offset_y + 0.5f) * inv_dst_full) - 0.5f;
    const float ny_height_cos = ny * height_cos;
    const float ny_height_sin = ny * height_sin;
    uint8_t *dst_row = dst_img + y * dst_width * bpp;
    
    for (uint16_t x = 0; x < dst_width; x++)
    {
      const float nx = (((float)x + offset_x + 0.5f) * inv_dst_full) - 0.5f;
      float src_x = x_center + (nx * width_cos) + ny_height_sin;
      float src_y = y_center + ny_height_cos - (nx * width_sin);
      
      /* Clamp with precomputed limits */
      if (src_x < 0.0f) src_x = 0.0f;
      if (src_x > src_width_limit) src_x = src_width_limit;
      if (src_y < 0.0f) src_y = 0.0f;
      if (src_y > src_height_limit) src_y = src_height_limit;
      
      const uint8_t *pIn = src_image + ((uint32_t)src_y * src_width + (uint32_t)src_x) * bpp;
      uint8_t *pOut = dst_row + x * bpp;
      
      /* Use memcpy for pixel copy */
      memcpy(pOut, pIn, bpp);
    }
  }
}

void img_crop_align565_to_888(uint8_t *src_image, uint16_t src_stride,
                              uint8_t *dst_img,
                              const uint16_t src_width, const uint16_t src_height,
                              const uint16_t dst_width, const uint16_t dst_height,
                              float x_center, float y_center,
                              float width, float height, float left_eye_x,
                              float left_eye_y, float right_eye_x, float right_eye_y)
{
  /* Optimized: precompute all constants and reduce redundant calculations */
  const float angle = -atan2f(right_eye_y - left_eye_y, right_eye_x - left_eye_x);
  const float cos_a = cosf(angle);
  const float sin_a = sinf(angle);
  const float dst_full = (dst_width > dst_height) ? (float)dst_width : (float)dst_height;
  const float offset_x = (dst_full - (float)dst_width) * 0.5f;
  const float offset_y = (dst_full - (float)dst_height) * 0.5f;
  const float inv_dst_full = 1.0f / dst_full;
  const float width_cos = width * cos_a;
  const float width_sin = width * sin_a;
  const float height_cos = height * cos_a;
  const float height_sin = height * sin_a;
  const float src_width_limit = (float)(src_width - 1);
  const float src_height_limit = (float)(src_height - 1);
  const uint16_t *src_base = (const uint16_t *)src_image;

  for (uint16_t y = 0; y < dst_height; y++)
  {
    const float ny = (((float)y + offset_y + 0.5f) * inv_dst_full) - 0.5f;
    const float ny_height_cos = ny * height_cos;
    const float ny_height_sin = ny * height_sin;
    uint8_t *dst_row = dst_img + y * dst_width * 3;
    
    for (uint16_t x = 0; x < dst_width; x++)
    {
      const float nx = (((float)x + offset_x + 0.5f) * inv_dst_full) - 0.5f;
      float src_x = x_center + (nx * width_cos) + ny_height_sin;
      float src_y = y_center + ny_height_cos - (nx * width_sin);
      
      /* Clamp with precomputed limits */
      if (src_x < 0.0f) src_x = 0.0f;
      if (src_x > src_width_limit) src_x = src_width_limit;
      if (src_y < 0.0f) src_y = 0.0f;
      if (src_y > src_height_limit) src_y = src_height_limit;
      
      const uint16_t *pIn = src_base + ((uint32_t)src_y * src_stride + (uint32_t)src_x);
      uint8_t *pOut = dst_row + x * 3;
      
      /* Optimized RGB565 to RGB888 conversion with bit operations */
      const uint16_t px = *pIn;
      pOut[0] = (uint8_t)(((px >> 11) & 0x1F) << 3);
      pOut[1] = (uint8_t)(((px >> 5) & 0x3F) << 2);
      pOut[2] = (uint8_t)((px & 0x1F) << 3);
    }
  }
#ifdef DUMMY_INPUT_BUFFER
  memcpy(dst_img, dummy_cropped_face_rgb, dst_width * dst_height * 3);
#endif
}

