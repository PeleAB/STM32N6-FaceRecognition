/**
 ******************************************************************************
 * @file   draw_dewarp.h
 * @author SRA AI Application Team
 * @brief  STM32 Image Processing Library - helpers draw triangle routine
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

#ifndef DRAW_DEWARP_H
#define DRAW_DEWARP_H

typedef void (*draw_dewarp_pel)(void *ctx, int x, int y, float u_s, float v_s);

void draw_dewarp_triangle_fill(int x0, int y0, float u0_s, float v0_s, int x1, int y1, float u1_s, float v1_s,
							   int x2, int y2, float u2_s, float v2_s, draw_dewarp_pel dpel, void *ctx);

#endif
