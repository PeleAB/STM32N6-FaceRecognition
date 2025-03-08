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