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

void img_rgb_to_hwc_float(uint8_t *src_image, float32_t *dst_img,
                          const uint32_t src_stride, const uint16_t width,
                          const uint16_t height)
{
  const float32_t scale = 1.f / 128.f;


  for (int i = 0; i< width*width*3; i++)
  {
        dst_img[i] = ((float32_t) src_image[i]) * scale - 1.f;
  }
}

void img_rgb_to_chw_float(uint8_t *src_image, float32_t *dst_img,
                          const uint32_t src_stride, const uint16_t width,
                          const uint16_t height)
{
  const float32_t scale = 1.f / 128.f;
  for (uint16_t y = 0; y < height; y++)
  {
    const uint8_t *line = src_image + y * src_stride;
    for (uint16_t x = 0; x < width; x++)
    {
      uint32_t idx = y * width + x;
      dst_img[idx] = ((float32_t)line[0]) * scale - 1.f;                     /* R */
      dst_img[width * height + idx] = ((float32_t)line[1]) * scale - 1.f;     /* G */
      dst_img[2 * width * height + idx] = ((float32_t)line[2]) * scale - 1.f; /* B */
      line += 3;
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
