/*---------------------------------------------------------------------------------------------
#  * Copyright (c) 2023 STMicroelectronics.
#  * All rights reserved.
#  *
#  * This software is licensed under terms that can be found in the LICENSE file in
#  * the root directory of this software component.
#  * If no LICENSE file comes with this software, it is provided AS-IS.
#  *--------------------------------------------------------------------------------------------*/

/* ---------------    Generated code    ----------------- */
#ifndef __POSTPROCESS_CONF_H__
#define __POSTPROCESS_CONF_H__


#ifdef __cplusplus
  extern "C" {
#endif

#include "arm_math.h"

#define POSTPROCESS_TYPE POSTPROCESS_OD_YOLO_V2_UF

/* I/O configuration */
#define AI_OD_YOLOV2_PP_NB_CLASSES                (1)
#define AI_OD_YOLOV2_PP_NB_ANCHORS                (5)
#define AI_OD_YOLOV2_PP_GRID_WIDTH                (7)
#define AI_OD_YOLOV2_PP_GRID_HEIGHT               (7)
#define AI_OD_YOLOV2_PP_NB_INPUT_BOXES            (AI_OD_YOLOV2_PP_GRID_WIDTH * AI_OD_YOLOV2_PP_GRID_HEIGHT)

/* Anchor boxes */
static const float32_t AI_OD_YOLOV2_PP_ANCHORS[2*AI_OD_YOLOV2_PP_NB_ANCHORS] = {
    0.9883000000f,     3.3606000000f,
    2.1194000000f,     5.3759000000f,
    3.0520000000f,     9.1336000000f,
    5.5517000000f,     9.3066000000f,
    9.7260000000f,     11.1422000000f,
  };

/* --------  Tuning below can be modified by the application --------- */
#define AI_OD_YOLOV2_PP_CONF_THRESHOLD            (0.6)
#define AI_OD_YOLOV2_PP_IOU_THRESHOLD             (0.3)
#define AI_OD_YOLOV2_PP_MAX_BOXES_LIMIT           (10)

#endif      /* __POSTPROCESS_CONF_H__  */


