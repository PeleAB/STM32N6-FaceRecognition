/*---------------------------------------------------------------------------------------------
#  * Copyright (c) 2023 STMicroelectronics.
#  * All rights reserved.
#  *
#  * This software is licensed under terms that can be found in the LICENSE file in
#  * the root directory of this software component.
#  * If no LICENSE file comes with this software, it is provided AS-IS.
#  *--------------------------------------------------------------------------------------------*/

#ifndef __POSTPROCESS_CONF_H__
#define __POSTPROCESS_CONF_H__


#ifdef __cplusplus
  extern "C" {
#endif

#include "arm_math.h"

#define POSTPROCESS_TYPE POSTPROCESS_CUSTOM

/* CenterFace grid configuration (input 128x128, stride 4) */
#define AI_FD_PP_GRID_WIDTH                (32)
#define AI_FD_PP_GRID_HEIGHT               (32)

/* Tuning — can be modified by the application */
#define AI_FD_PP_CONF_THRESHOLD            (0.5f)
#define AI_FD_PP_IOU_THRESHOLD             (0.3f)
#define AI_FD_PP_MAX_BOXES_LIMIT           (10)

#ifdef __cplusplus
  }
#endif

#endif      /* __POSTPROCESS_CONF_H__  */
