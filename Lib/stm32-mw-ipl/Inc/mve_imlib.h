/**
  ******************************************************************************
  * @file    mve_imlib.h
  * @author  AIS Team
  * @brief   MVE Image processing library generic optimized functions

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

#ifndef __MVE_IMLIB_H__
#define __MVE_IMLIB_H__

#include "stm32ipl_imlib.h"
#include "stm32ipl_imlib_int.h"

mve_pred16_t mve_image_get_mask_pixel(image_t *ptr, int x, int y);

#endif /* __MVE_IMLIB_H__ */
