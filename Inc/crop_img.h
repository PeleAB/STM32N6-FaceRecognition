 /**
 ******************************************************************************
 * @file    crop_img.h
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
#ifndef CROP_IMG
#define CROP_IMG
#include "arm_math.h"


void img_crop(uint8_t *src_image, uint8_t *dst_img, const uint32_t src_width,
              const uint16_t dst_width, const uint16_t dst_height,
              const uint16_t bpp);

#endif