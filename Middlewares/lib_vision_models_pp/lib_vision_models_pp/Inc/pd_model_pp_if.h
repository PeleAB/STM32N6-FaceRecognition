/**
  ******************************************************************************
  * @file    pd_model_pp_if.h
  * @author  MDG Application Team
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

#ifndef _PD_MODEL_PP_IF_
#define _PD_MODEL_PP_IF_


#ifdef __cplusplus
 extern "C" {
#endif

#include "pd_pp_output_if.h"

typedef struct {
  float32_t x;
  float32_t y;
  float32_t w;
  float32_t h;
} pd_anchor_t;

/* I/O structures for model PD type */
typedef struct pd_model_pp_in
{
  float32_t *pScale;
  float32_t *pLms;
  float32_t *pHeatmap;
  float32_t *pOffset;
} pd_model_pp_in_t;

typedef struct pd_model_static_param {
  uint32_t width;
  uint32_t height;
  uint32_t nb_keypoints;
  float32_t conf_threshold;
  float32_t iou_threshold;
  uint32_t nb_total_boxes;
  uint32_t max_boxes_limit;
  pd_anchor_t *pAnchors;
} pd_model_pp_static_param_t;


int32_t pd_model_pp_reset(pd_model_pp_static_param_t *pInput_static_param);

int32_t pd_model_pp_process(pd_model_pp_in_t *pInput,
                        pd_postprocess_out_t *pOutput,
                        pd_model_pp_static_param_t *pInput_static_param);


#ifdef __cplusplus
  }
#endif

#endif // _PD_MODEL_PP_IF_
