 /**
 ******************************************************************************
 * @file    draw.c
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
#include "svc/draw.h"

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include "fal_dma2d.h"
#include "utils.h"

#define MAX_LINE_CHAR 64

static void draw_dma2d_cb(void *ctx)
{
  (void) ctx;
  DRAW_Signal();
}

static void draw_dma2d_error_cb(void *ctx)
{
  (void) ctx;
  assert(0);
}

static void draw_copy_argb_hw(uint8_t *p_dst, int dst_width, int dst_height, uint8_t *p_src, int src_width,
                              int src_height, int x_offset, int y_offset)
{
  fal_dma2d_blend_t cfg = {
    .dst = p_dst,
    .dst_width = (uint32_t) dst_width,
    .dst_height = (uint32_t) dst_height,
    .src = p_src,
    .src_width = (uint32_t) src_width,
    .src_height = (uint32_t) src_height,
    .x_offset = (uint32_t) x_offset,
    .y_offset = (uint32_t) y_offset,
    .on_complete = draw_dma2d_cb,
    .on_error = draw_dma2d_error_cb,
    .user = NULL,
  };
  int ret;

  DRAW_HwLock(NULL);
  ret = FAL_DMA2D_Blend(&cfg);
  assert(ret == 0);

  DRAW_Wfe();
  DRAW_HwUnlock();
}

static void draw_fill_argb_hw(uint8_t *p_dst, int dst_width, int dst_height, int src_width, int src_height,
                              int x_offset, int y_offset, uint32_t color)
{
  fal_dma2d_fill_t cfg = {
    .dst = p_dst,
    .dst_width = (uint32_t) dst_width,
    .dst_height = (uint32_t) dst_height,
    .width = (uint32_t) src_width,
    .height = (uint32_t) src_height,
    .x_offset = (uint32_t) x_offset,
    .y_offset = (uint32_t) y_offset,
    .color = color,
    .on_complete = draw_dma2d_cb,
    .on_error = draw_dma2d_error_cb,
    .user = NULL,
  };
  int ret;

  DRAW_HwLock(NULL);
  ret = FAL_DMA2D_Fill(&cfg);
  assert(ret == 0);

  DRAW_Wfe();
  DRAW_HwUnlock();
}

static void draw_hline_argb_hw(uint8_t *p_dst, int dst_width, int dst_height, int x_pos, int y_pos, int len,
                               uint32_t color)
{
  draw_fill_argb_hw(p_dst, dst_width, dst_height, len, 1, x_pos, y_pos, color);
}

static void draw_vline_argb_hw(uint8_t *p_dst, int dst_width, int dst_height, int x_pos, int y_pos, int len,
                               uint32_t color)
{
  draw_fill_argb_hw(p_dst, dst_width, dst_height, 1, len, x_pos, y_pos, color);
}

static void draw_rect_argb_hw(uint8_t *p_dst, int dst_width, int dst_height, int x_pos, int y_pos, int width, int height,
                       uint32_t color)
{
  draw_hline_argb_hw(p_dst, dst_width, dst_height, x_pos, y_pos, width, color);
  draw_hline_argb_hw(p_dst, dst_width, dst_height, x_pos, y_pos + height - 1, width, color);

  draw_vline_argb_hw(p_dst, dst_width, dst_height, x_pos, y_pos, height, color);
  draw_vline_argb_hw(p_dst, dst_width, dst_height, x_pos + width - 1, y_pos, height, color);
}

static void draw_font_cvt(sFONT *p_font_in, uint32_t *dout, uint8_t *din)
{
  uint32_t height, width;
  uint32_t offset;
  uint8_t  *pchar;
  uint32_t line;
  int i, j;

  height = p_font_in->Height;
  width  = p_font_in->Width;

  offset =  8 *((width + 7)/8) -  width;
  for( i = 0; i < height; i++) {
    pchar = (din + (width + 7)/8 * i);

    switch(((width + 7)/8)) {
    case 1:
      line =  pchar[0];
      break;
    case 2:
      line =  (pchar[0]<< 8) | pchar[1];
      break;
    case 3:
    default:
      line =  (pchar[0]<< 16) | (pchar[1]<< 8) | pchar[2];
      break;
    }

    for (j = 0; j < width; j++) {
      if (line & (1 << (width- j + offset- 1)))
        *dout++ = 0xFFFFFFFFUL;
      else
        *dout++ = 0x40000000UL;
    }
  }
}

static int draw_font_setup_with_memory(sFONT *p_font_in, DRAW_Font_t *p_font, void *data, int data_size)
{
  const int nb_char_in_font = '~' - ' ' + 1;
  int bytes_per_char;
  int char_size_in;
  int i;

  bytes_per_char = p_font_in->Width * p_font_in->Height * 4;
  if (data_size < bytes_per_char * nb_char_in_font)
    return -1;

  char_size_in = p_font_in->Height * ((p_font_in->Width + 7) / 8);
  p_font->width = p_font_in->Width;
  p_font->height = p_font_in->Height;
  p_font->data = data;

  for (i = 0; i < nb_char_in_font; i++)
    draw_font_cvt(p_font_in, (uint32_t *) &p_font->data[i * bytes_per_char],
                  (uint8_t *) &p_font_in->table[i * char_size_in]);

  return 0;
}

static void draw_draw_char_argb_hw(DRAW_Font_t *p_font, uint8_t *p_dst, int dst_width, int dst_height, int x_pos,
                                   int y_pos, uint8_t *data)
{
  draw_copy_argb_hw(p_dst, dst_width, dst_height, data, p_font->width, p_font->height, x_pos, y_pos);
}

static void draw_display_char_argb_hw(DRAW_Font_t *p_font, uint8_t *p_dst, int dst_width, int dst_height, int x_pos,
                                      int y_pos, char c)
{
  int char_size = p_font->height * p_font->width * 4;

  draw_draw_char_argb_hw(p_font, p_dst, dst_width, dst_height, x_pos, y_pos, &p_font->data[(c - ' ') * char_size]);
}

static void draw_puts_argb_hw(DRAW_Font_t *p_font, uint8_t *p_dst, int dst_width, int dst_height, int x_pos, int y_pos,
                              char *line)
{
  while (*line != '\0') {
    draw_display_char_argb_hw(p_font, p_dst, dst_width, dst_height, x_pos, y_pos, *line);
    x_pos += p_font->width;
    line++;
  }
}

int DRAW_FontSetup(sFONT *p_font_in, DRAW_Font_t *p_font)
{
  const int nb_char_in_font = '~' - ' ' + 1;
  int bytes_per_char;

  bytes_per_char = p_font_in->Width * p_font_in->Height * 4;
  p_font->data = malloc(nb_char_in_font * bytes_per_char);
  if (!p_font->data)
    return -1;

  return draw_font_setup_with_memory(p_font_in, p_font, p_font->data, nb_char_in_font * bytes_per_char);
}

void DRAW_RectArgbHw(uint8_t *p_dst, int dst_width, int dst_height, int x_pos, int y_pos, int width, int height,
                     uint32_t color)
{
  draw_rect_argb_hw(p_dst, dst_width, dst_height, x_pos, y_pos, width, height, color);
}

void DRAW_FillArgbHw(uint8_t *p_dst, int dst_width, int dst_height, int x_pos, int y_pos, int width, int height,
                     uint32_t color)
{
  draw_fill_argb_hw(p_dst, dst_width, dst_height, x_pos, y_pos, width, height, color);
}

void DRAW_PrintfArgbHw(DRAW_Font_t *p_font, uint8_t *p_dst, int dst_width, int dst_height, int x_pos, int y_pos,
                       const char * format, ...)
{
  char buffer[MAX_LINE_CHAR + 1];
  va_list args;

  buffer[MAX_LINE_CHAR] = '\0';

  va_start(args, format);
  vsnprintf(buffer, MAX_LINE_CHAR, format, args);
  va_end(args);
  draw_puts_argb_hw(p_font, p_dst, dst_width, dst_height, x_pos, y_pos, buffer);
}

void DRAW_CopyArgbHW(uint8_t *p_dst, int dst_width, int dst_height, uint8_t *p_src, int src_width, int src_height,
                     int x_offset, int y_offset)
{
  draw_copy_argb_hw(p_dst, dst_width, dst_height, p_src, src_width, src_height, x_offset, y_offset);
}

WEAK void DRAW_HwLock(void *dma2d_handle)
{
  assert_param(0);
}

WEAK void DRAW_HwUnlock()
{
  assert_param(0);
}

WEAK void DRAW_Wfe()
{
  assert_param(0);
}

WEAK void DRAW_Signal()
{
  assert_param(0);
}

