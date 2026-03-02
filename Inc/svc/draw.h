 /**
 ******************************************************************************
 * @file    draw.h
 * @author  GPM Application Team
 *
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2023 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
#ifndef DRAW
#define DRAW

#include <stdint.h>

#include "fonts.h"

typedef struct {
  uint16_t width;
  uint16_t height;
  uint8_t *data;
} DRAW_Font_t;

int DRAW_FontSetup(sFONT *p_font_in, DRAW_Font_t *p_font);
void DRAW_RectArgbHw(uint8_t *p_dst, int dst_width, int dst_height, int x_pos, int y_pos, int width, int height,
                     uint32_t color);
void DRAW_FillArgbHw(uint8_t *p_dst, int dst_width, int dst_height, int x_pos, int y_pos, int width, int height,
                     uint32_t color);
void DRAW_PrintfArgbHw(DRAW_Font_t *p_font, uint8_t *p_dst, int dst_width, int dst_height, int x_pos, int y_pos,
                       const char * format, ...);
void DRAW_CopyArgbHW(uint8_t *p_dst, int dst_width, int dst_height, uint8_t *p_src, int src_width, int src_height,
                     int x_offset, int y_offset);

/* Implement this if you are using Hw family API */
void DRAW_HwLock(void *dma2d_handle);
void DRAW_HwUnlock(void);
void DRAW_Wfe(void);
void DRAW_Signal(void);

#endif
