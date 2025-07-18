 /**
 ******************************************************************************
 * @file    crop_img.c
 * @author  GPM Application Team
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

void img_crop(uint8_t *src_image, uint8_t *dst_img, const uint32_t src_stride,
              const uint16_t dst_width, const uint16_t height,
              const uint16_t dst_bpp)
{
  const uint8_t *pIn = src_image;
  uint8_t *pOut = dst_img;
  const uint32_t dst_line_size = (dst_width * dst_bpp);

  /* Copy line per line */
  for (uint32_t i = 0; i < height; i++)
  {
    memcpy(pOut, pIn + (i * src_stride), dst_line_size);
    pOut += dst_line_size;
  }
}



void img_rgb_to_chw_float(uint8_t *src_image, float32_t *dst_img,
                          const uint32_t src_stride, const uint16_t width,
                          const uint16_t height)
{
  /* CHW layout: channel-first order */
  for (uint16_t y = 0; y < height; y++)
  {
    const uint8_t *pIn = src_image + y * src_stride;
    for (uint16_t x = 0; x < width; x++)
    {
      dst_img[y * width + x] = (float32_t)pIn[0];
      dst_img[height * width + y * width + x] = (float32_t)pIn[1];
      dst_img[2 * height * width + y * width + x] = (float32_t)pIn[2];
      pIn += 3;
    }
  }
}

void img_rgb_to_chw_float_norm(uint8_t *src_image, float32_t *dst_img,
                          const uint32_t src_stride, const uint16_t width,
                          const uint16_t height)
{
  /* CHW layout: channel-first order */
  for (uint16_t y = 0; y < height; y++)
  {
    const uint8_t *pIn = src_image + y * src_stride;
    for (uint16_t x = 0; x < width; x++)
    {
      dst_img[y * width + x] = (((float32_t)pIn[0])-127.5)/127.5;
      dst_img[height * width + y * width + x] = (((float32_t)pIn[1])-127.5)/127.5;
      dst_img[2 * height * width + y * width + x] = (((float32_t)pIn[2])-127.5)/127.5;
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
  for (int y = 0; y < dst_height; y++)
  {
    int src_y = y0 + (y * crop_height) / dst_height;
    if (src_y < 0) src_y = 0;
    if (src_y >= src_height) src_y = src_height - 1;
    for (int x = 0; x < dst_width; x++)
    {
      int src_x = x0 + (x * crop_width) / dst_width;
      if (src_x < 0) src_x = 0;
      if (src_x >= src_width) src_x = src_width - 1;
      const uint8_t *pIn = src_image + (src_y * src_width + src_x) * bpp;
      uint8_t *pOut = dst_img + (y * dst_width + x) * bpp;
      for (int c = 0; c < bpp; c++)
      {
        pOut[c] = pIn[c];
      }
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
  float angle = -atan2f(right_eye_y - left_eye_y, right_eye_x - left_eye_x);
  float cos_a = cosf(angle);
  float sin_a = sinf(angle);
  float dst_full = (dst_width > dst_height) ? (float)dst_width : (float)dst_height;
  float offset_x = (dst_full - (float)dst_width) * 0.5f;
  float offset_y = (dst_full - (float)dst_height) * 0.5f;

  for (uint16_t y = 0; y < dst_height; y++)
  {
    float ny = ((float)y + offset_y + 0.5f) / dst_full - 0.5f;
    for (uint16_t x = 0; x < dst_width; x++)
    {
      float nx = ((float)x + offset_x + 0.5f) / dst_full - 0.5f;
      float src_x = x_center + (nx * width) * cos_a + (ny * height) * sin_a;
      float src_y = y_center + (ny * height) * cos_a - (nx * width) * sin_a;
      if (src_x < 0) src_x = 0;
      if (src_x >= src_width) src_x = src_width - 1;
      if (src_y < 0) src_y = 0;
      if (src_y >= src_height) src_y = src_height - 1;
      const uint8_t *pIn = src_image + ((uint32_t)src_y * src_width + (uint32_t)src_x) * bpp;
      uint8_t *pOut = dst_img + ((uint32_t)y * dst_width + (uint32_t)x) * bpp;
      for (uint16_t c = 0; c < bpp; c++)
      {
        pOut[c] = pIn[c];
      }
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
  float angle = -atan2f(right_eye_y - left_eye_y, right_eye_x - left_eye_x);
  float cos_a = cosf(angle);
  float sin_a = sinf(angle);

  float dst_full = (dst_width > dst_height) ? (float)dst_width : (float)dst_height;
  float offset_x = (dst_full - (float)dst_width) * 0.5f;
  float offset_y = (dst_full - (float)dst_height) * 0.5f;

  for (uint16_t y = 0; y < dst_height; y++)
  {
    float ny = ((float)y + offset_y + 0.5f) / dst_full - 0.5f;
    for (uint16_t x = 0; x < dst_width; x++)
    {
      float nx = ((float)x + offset_x + 0.5f) / dst_full - 0.5f;
      float src_x = x_center + (nx * width) * cos_a + (ny * height) * sin_a;
      float src_y = y_center + (ny * height) * cos_a - (nx * width) * sin_a;
      if (src_x < 0) src_x = 0;
      if (src_x >= src_width) src_x = src_width - 1;
      if (src_y < 0) src_y = 0;
      if (src_y >= src_height) src_y = src_height - 1;
      const uint16_t *pIn = (const uint16_t *)src_image +
                            ((uint32_t)src_y * src_stride + (uint32_t)src_x);
      uint8_t *pOut = dst_img + ((uint32_t)y * dst_width + (uint32_t)x) * 3;
      uint16_t px = *pIn;
      pOut[0] = (uint8_t)(((px >> 11) & 0x1F) << 3);
      pOut[1] = (uint8_t)(((px >> 5) & 0x3F) << 2);
      pOut[2] = (uint8_t)((px & 0x1F) << 3);
    }
  }
}
