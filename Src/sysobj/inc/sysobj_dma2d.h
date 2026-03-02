/**
 ******************************************************************************
 * @file    fal_dma2d.h
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

#ifndef FAL_DMA2D_H
#define FAL_DMA2D_H

#include <stdint.h>

typedef void (*fal_dma2d_cb_t)(void *user);

typedef struct {
  uint8_t *dst;
  uint32_t dst_width;
  uint32_t dst_height;
  uint8_t *src;
  uint32_t src_width;
  uint32_t src_height;
  uint32_t x_offset;
  uint32_t y_offset;
  fal_dma2d_cb_t on_complete;
  fal_dma2d_cb_t on_error;
  void *user;
} fal_dma2d_blend_t;

typedef struct {
  uint8_t *dst;
  uint32_t dst_width;
  uint32_t dst_height;
  uint32_t width;
  uint32_t height;
  uint32_t x_offset;
  uint32_t y_offset;
  uint32_t color;
  fal_dma2d_cb_t on_complete;
  fal_dma2d_cb_t on_error;
  void *user;
} fal_dma2d_fill_t;

int FAL_DMA2D_Blend(const fal_dma2d_blend_t *cfg);
int FAL_DMA2D_Fill(const fal_dma2d_fill_t *cfg);
void FAL_DMA2D_IRQHandler(void);
void DMA2D_IRQHandler(void);

#endif /* FAL_DMA2D_H */
