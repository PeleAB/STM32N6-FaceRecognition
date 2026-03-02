 /**
 ******************************************************************************
 * @file    app_postprocess_od_st_yolox_uf.c
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

#if POSTPROCESS_TYPE == POSTPROCESS_OD_ST_YOLOX_UI
#define MAX(a,b) (((a)>(b))?(a):(b))
static od_pp_outBuffer_t out_detections[MAX(AI_OD_ST_YOLOX_PP_MAX_BOXES_LIMIT,
                                            AI_OD_ST_YOLOX_PP_L_GRID_WIDTH * AI_OD_ST_YOLOX_PP_L_GRID_HEIGHT +
                                            AI_OD_ST_YOLOX_PP_M_GRID_WIDTH * AI_OD_ST_YOLOX_PP_M_GRID_HEIGHT +
                                            AI_OD_ST_YOLOX_PP_S_GRID_WIDTH * AI_OD_ST_YOLOX_PP_S_GRID_HEIGHT)];

int32_t app_postprocess_init(void *params_postprocess, NN_Instance_TypeDef *NN_Instance)
{
  int32_t error = AI_OD_POSTPROCESS_ERROR_NO;
  od_st_yolox_pp_static_param_t *params = (od_st_yolox_pp_static_param_t *) params_postprocess;
  const LL_Buffer_InfoTypeDef *buffers_info = LL_ATON_Output_Buffers_Info(NN_Instance);
  params->raw_s_scale = *(buffers_info[0].scale);
  params->raw_s_zero_point = *(buffers_info[0].offset);
  params->raw_l_scale = *(buffers_info[1].scale);
  params->raw_l_zero_point = *(buffers_info[1].offset);
  params->raw_m_scale = *(buffers_info[2].scale);
  params->raw_m_zero_point = *(buffers_info[2].offset);
  params->nb_classes = AI_OD_ST_YOLOX_PP_NB_CLASSES;
  params->nb_anchors = AI_OD_ST_YOLOX_PP_NB_ANCHORS;
  params->grid_width_L = AI_OD_ST_YOLOX_PP_L_GRID_WIDTH;
  params->grid_height_L = AI_OD_ST_YOLOX_PP_L_GRID_HEIGHT;
  params->grid_width_M = AI_OD_ST_YOLOX_PP_M_GRID_WIDTH;
  params->grid_height_M = AI_OD_ST_YOLOX_PP_M_GRID_HEIGHT;
  params->grid_width_S = AI_OD_ST_YOLOX_PP_S_GRID_WIDTH;
  params->grid_height_S = AI_OD_ST_YOLOX_PP_S_GRID_HEIGHT;
  params->pAnchors_L = AI_OD_ST_YOLOX_PP_L_ANCHORS;
  params->pAnchors_M = AI_OD_ST_YOLOX_PP_M_ANCHORS;
  params->pAnchors_S = AI_OD_ST_YOLOX_PP_S_ANCHORS;
  params->max_boxes_limit = AI_OD_ST_YOLOX_PP_MAX_BOXES_LIMIT;
  params->conf_threshold = AI_OD_ST_YOLOX_PP_CONF_THRESHOLD;
  params->iou_threshold = AI_OD_ST_YOLOX_PP_IOU_THRESHOLD;
  error = od_st_yolox_pp_reset(params);
  return error;
}

int32_t app_postprocess_run(void *pInput[], int nb_input, void *pOutput, void *pInput_param)
{
  assert(nb_input == 3);
  int32_t error = AI_OD_POSTPROCESS_ERROR_NO;
  ((od_st_yolox_pp_static_param_t *) pInput_param)->nb_detect = 0;
  od_pp_out_t *pObjDetOutput = (od_pp_out_t *) pOutput;
  pObjDetOutput->pOutBuff = out_detections;
  od_st_yolox_pp_in_t pp_input = {
      .pRaw_detections_S = (float32_t *) pInput[0],
      .pRaw_detections_L = (float32_t *) pInput[1],
      .pRaw_detections_M = (float32_t *) pInput[2],
  };
  error = od_st_yolox_pp_process_int8(&pp_input, pObjDetOutput,
                                 (od_st_yolox_pp_static_param_t *) pInput_param);
  return error;
}
#endif

