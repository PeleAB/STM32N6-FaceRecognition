/**
  ******************************************************************************
  * @file    mve_binary.h
  * @author  AIS Team
  * @brief   MVE Image processing library binary functions 

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

#ifndef __MVE_BINARY__
#define __MVE_BINARY__

#include "imlib.h"

void mve_imlib_binary_binary(image_t *out, image_t *img, color_thresholds_list_lnk_data_t *thresholds, bool invert, bool zero, image_t *mask);
void mve_imlib_binary_grayscale(image_t *out, image_t *img, color_thresholds_list_lnk_data_t *thresholds, bool invert, bool zero, image_t *mask);
void mve_imlib_binary_rgb565(image_t *out, image_t *img, color_thresholds_list_lnk_data_t *thresholds, bool invert, bool zero, image_t *mask);
void mve_imlib_binary_rgb888(image_t *out, image_t *img, color_thresholds_list_lnk_data_t *thresholds, bool invert, bool zero, image_t *mask);

int mve_imlib_erode_dilate_grayscale(image_t *img, int ksize, int threshold, int e_or_d, image_t *mask);

#endif /* __MVE_BINARY__ */
