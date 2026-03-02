 /**
 ******************************************************************************
 * @file    app_postprocess_mpe_pd_uf.c
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


#include "app_postprocess.h"
#include "app_config.h"
#include <assert.h>

#if POSTPROCESS_TYPE == POSTPROCESS_MPE_PD_UF
/* Must be in app code */
#include "pd_anchors.c"
/* post process algo will not write more than AI_PD_MODEL_PP_MAX_BOXES_LIMIT */
static pd_pp_box_t out_detections[AI_PD_MODEL_PP_MAX_BOXES_LIMIT];
static pd_pp_point_t out_keyPoints[AI_PD_MODEL_PP_MAX_BOXES_LIMIT][AI_PD_MODEL_PP_NB_KEYPOINTS];

int32_t app_postprocess_init(void *params_postprocess, NN_Instance_TypeDef *NN_Instance)
{
  int32_t error;
  pd_model_pp_static_param_t *params = (pd_model_pp_static_param_t *) params_postprocess;
  params->width = AI_PD_MODEL_PP_WIDTH;
  params->height = AI_PD_MODEL_PP_HEIGHT;
  params->nb_keypoints = AI_PD_MODEL_PP_NB_KEYPOINTS;
  params->conf_threshold = AI_PD_MODEL_PP_CONF_THRESHOLD;
  params->iou_threshold = AI_PD_MODEL_PP_IOU_THRESHOLD;
  params->nb_total_boxes = AI_PD_MODEL_PP_TOTAL_DETECTIONS;
  params->max_boxes_limit = AI_PD_MODEL_PP_MAX_BOXES_LIMIT;
  params->pAnchors = g_Anchors;
  for (int i = 0; i < AI_PD_MODEL_PP_MAX_BOXES_LIMIT; i++) {
    out_detections[i].pKps = &out_keyPoints[i][0];
  }
  error = pd_model_pp_reset(params);
  return error;
}

int32_t app_postprocess_run(void *pInput[], int nb_input, void *pOutput, void *pInput_param)
{
  assert(nb_input == 2);
  pd_pp_out_t *pPdOutput = (pd_pp_out_t *) pOutput;
  pd_model_pp_in_t pp_input = {
    .pProbs = (float32_t *) pInput[0],
    .pBoxes = (float32_t *) pInput[1],
  };
  int32_t error;
  pPdOutput->pOutData = out_detections;
  error = pd_model_pp_process(&pp_input, pPdOutput, 
                              (pd_model_pp_static_param_t *) pInput_param);

  return error;
}
#endif

