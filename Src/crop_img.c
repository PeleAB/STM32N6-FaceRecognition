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
  const float32_t scale = 1.f / 128.f;
  float32_t *dst_r = dst_img;
  float32_t *dst_g = dst_r + width * height;
  float32_t *dst_b = dst_g + width * height;

  for (int y = height - 1; y >= 0; y--)
  {
    uint8_t *src = src_image + y * src_stride;
    for (int x = width - 1; x >= 0; x--)
    {
      uint8_t r = src[3 * x + 0];
      uint8_t g = src[3 * x + 1];
      uint8_t b = src[3 * x + 2];
      dst_r[y * width + x] = ((float32_t) r) * scale - 1.f;
      dst_g[y * width + x] = ((float32_t) g) * scale - 1.f;
      dst_b[y * width + x] = ((float32_t) b) * scale - 1.f;
    }
  }
}