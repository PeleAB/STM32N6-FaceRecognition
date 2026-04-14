/**
  ******************************************************************************
  * @file    mve_matop.h
  * @author  AIS Team
  * @brief   MVE Image processing library mathematic operation optimized functions

  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

#ifndef __MVE_MATOP_H__
#define __MVE_MATOP_H__

#include "stm32ipl_imlib.h"
#include "stm32ipl_imlib_int.h"

#include "mve_imlib.h"

static inline void mve_imlib_difference_line_op_grayscale(image_t *img, int line, void *other, image_t *mask)
{
  uint8_t *p_data_0 = IMAGE_COMPUTE_GRAYSCALE_PIXEL_ROW_PTR(img, line);
  uint8_t *p_data_1 = (uint8_t *)other;
  if (NULL == mask) {
    for (int i = 0; i < img->w; i+=16) {
      mve_pred16_t p = vctp8q(img->w-i);
      uint8x16_t u8x16_data_0 = vldrbq_z_u8(p_data_0, p);
      uint8x16_t u8x16_data_1 = vldrbq_z_u8(p_data_1, p);
      uint8x16_t u8x16_data_out = vabdq_u8(u8x16_data_0, u8x16_data_1);
      vstrbq_p_u8(p_data_0, u8x16_data_out, p);
      p_data_0 += 16;
      p_data_1 += 16;
    }
  }
  else
  {
    for (int i = 0; i < img->w; i+=16) {
      mve_pred16_t p_mask = mve_image_get_mask_pixel(mask, i, line);
      uint8x16_t u8x16_data_0 = vldrbq_z_u8(p_data_0, p_mask);
      uint8x16_t u8x16_data_1 = vldrbq_z_u8(p_data_1, p_mask);
      uint8x16_t u8x16_data_out = vabdq_u8(u8x16_data_0, u8x16_data_1);
      vstrbq_p_u8(p_data_0, u8x16_data_out, p_mask);
      p_data_0 += 16;
      p_data_1 += 16;
    }
  }
}

static inline void mve_imlib_difference_line_op_rgb888(image_t *img, int line, void *other, image_t *mask)
{
  rgb888_t *ptr_rgb888_0 = IMAGE_COMPUTE_RGB888_PIXEL_ROW_PTR(img, line);
  rgb888_t *ptr_rgb888_1 = (rgb888_t *)other;
  uint8x16_t u8x16_incr = vidupq_n_u8(0, 1) * 3;
  if (NULL == mask) {
    for (int i = 0; i < img->w; i+=16) {
      mve_pred16_t pred = vctp8q(img->w-i);
      uint8x16_t u8x16_r_0 = vldrbq_gather_offset_z_u8(&(ptr_rgb888_0->r), u8x16_incr, pred);
      uint8x16_t u8x16_g_0 = vldrbq_gather_offset_z_u8(&(ptr_rgb888_0->g), u8x16_incr, pred);
      uint8x16_t u8x16_b_0 = vldrbq_gather_offset_z_u8(&(ptr_rgb888_0->b), u8x16_incr, pred);
      uint8x16_t u8x16_r_1 = vldrbq_gather_offset_z_u8(&(ptr_rgb888_1->r), u8x16_incr, pred);
      uint8x16_t u8x16_g_1 = vldrbq_gather_offset_z_u8(&(ptr_rgb888_1->g), u8x16_incr, pred);
      uint8x16_t u8x16_b_1 = vldrbq_gather_offset_z_u8(&(ptr_rgb888_1->b), u8x16_incr, pred);
      uint8x16_t u8x16_data_out;
      u8x16_data_out = vabdq_u8(u8x16_r_0, u8x16_r_1);
      vstrbq_scatter_offset_p_u8(&(ptr_rgb888_0->r), u8x16_incr, u8x16_data_out, pred);
      u8x16_data_out = vabdq_u8(u8x16_g_0, u8x16_g_1);
      vstrbq_scatter_offset_p_u8(&(ptr_rgb888_0->g), u8x16_incr, u8x16_data_out, pred);
      u8x16_data_out = vabdq_u8(u8x16_b_0, u8x16_b_1);
      vstrbq_scatter_offset_p_u8(&(ptr_rgb888_0->b), u8x16_incr, u8x16_data_out, pred);
      ptr_rgb888_0 += 16;
      ptr_rgb888_1 += 16;
    }
  }
  else
  {
    for (int i = 0; i < img->w; i+=16) {
      mve_pred16_t pred = mve_image_get_mask_pixel(mask, i, line);
      uint8x16_t u8x16_r_0 = vldrbq_gather_offset_z_u8(&(ptr_rgb888_0->r), u8x16_incr, pred);
      uint8x16_t u8x16_g_0 = vldrbq_gather_offset_z_u8(&(ptr_rgb888_0->g), u8x16_incr, pred);
      uint8x16_t u8x16_b_0 = vldrbq_gather_offset_z_u8(&(ptr_rgb888_0->b), u8x16_incr, pred);
      uint8x16_t u8x16_r_1 = vldrbq_gather_offset_z_u8(&(ptr_rgb888_1->r), u8x16_incr, pred);
      uint8x16_t u8x16_g_1 = vldrbq_gather_offset_z_u8(&(ptr_rgb888_1->g), u8x16_incr, pred);
      uint8x16_t u8x16_b_1 = vldrbq_gather_offset_z_u8(&(ptr_rgb888_1->b), u8x16_incr, pred);
      uint8x16_t u8x16_data_out;
      u8x16_data_out = vabdq_u8(u8x16_r_0, u8x16_r_1);
      vstrbq_scatter_offset_p_u8(&(ptr_rgb888_0->r), u8x16_incr, u8x16_data_out, pred);
      u8x16_data_out = vabdq_u8(u8x16_g_0, u8x16_g_1);
      vstrbq_scatter_offset_p_u8(&(ptr_rgb888_0->g), u8x16_incr, u8x16_data_out, pred);
      u8x16_data_out = vabdq_u8(u8x16_b_0, u8x16_b_1);
      vstrbq_scatter_offset_p_u8(&(ptr_rgb888_0->b), u8x16_incr, u8x16_data_out, pred);
      ptr_rgb888_0 += 16;
      ptr_rgb888_1 += 16;
    }
  }
}
#endif /* __MVE_MATOP_H__ */
