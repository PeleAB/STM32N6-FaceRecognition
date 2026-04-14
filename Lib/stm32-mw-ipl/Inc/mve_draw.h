/**
  ******************************************************************************
  * @file    mve_draw.h
  * @author  AIS Team
  * @brief   MVE Image processing library drawing functions with 8-bits
  *          inputs/outputs

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

#ifndef __MVE_DRAW__
#define __MVE_DRAW__

#include "arm_math.h"
#include "imlib.h"

void point_fill_mve(image_t *img, int cx, int cy, int r0, int r1, int c);

void point_fill_mve_binary(image_t *img, int cx, int cy, int r0, int r1, int c);
void point_fill_mve_grayscale(image_t *img, int cx, int cy, int r0, int r1, int c);
void point_fill_mve_rgb888(image_t *img, int cx, int cy, int r0, int r1, int c);
void point_fill_mve_rgb565(image_t *img, int cx, int cy, int r0, int r1, int c);

#endif /* __MVE_DRAW__ */
