/**
  ******************************************************************************
  * @file    mve_filter.h
  * @author  AIS Team
  * @brief   MVE Image processing library filter functions with 8-bits
  *          inputs/outputs

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

#ifndef __MVE_FILTER__
#define __MVE_FILTER__

#include "arm_math.h"
#include "imlib.h"

int mve_imlib_median_filter_grayscale(image_t *img, const int ksize, float percentile, bool threshold, int offset, bool invert, image_t *mask);
int mve_imlib_morph_u8(image_t *img, const int ksize, const uint8_t *krn, const float m, const int b, bool threshold, int offset, bool invert, image_t *mask);

#endif /* __MVE_FILTER__ */
