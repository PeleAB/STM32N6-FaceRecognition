 /**
 ******************************************************************************
 * @file    app_postprocess_od_yolov2_uf.c
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

#if POSTPROCESS_TYPE == POSTPROCESS_OD_YOLO_V2_UI
static uint8_t scratch_buffer[AI_OD_YOLOV2_PP_GRID_WIDTH * AI_OD_YOLOV2_PP_GRID_HEIGHT * AI_OD_YOLOV2_PP_NB_ANCHORS * sizeof(od_pp_outBuffer_t)];

int32_t app_postprocess_init(void *params_postprocess, void *network_instance)
{
  NN_Instance_TypeDef *NN_Instance = (NN_Instance_TypeDef *)network_instance;
  int32_t error = AI_OD_POSTPROCESS_ERROR_NO;
  od_yolov2_pp_static_param_t *params = (od_yolov2_pp_static_param_t *) params_postprocess;
  const LL_Buffer_InfoTypeDef *buffers_info = LL_ATON_Output_Buffers_Info(NN_Instance);
  params->raw_scale = *(buffers_info[0].scale);
  params->raw_zero_point = *(buffers_info[0].offset);
  params->conf_threshold = AI_OD_YOLOV2_PP_CONF_THRESHOLD;
  params->iou_threshold = AI_OD_YOLOV2_PP_IOU_THRESHOLD;
  params->nb_anchors = AI_OD_YOLOV2_PP_NB_ANCHORS;
  params->nb_classes = AI_OD_YOLOV2_PP_NB_CLASSES;
  params->grid_height = AI_OD_YOLOV2_PP_GRID_HEIGHT;
  params->grid_width = AI_OD_YOLOV2_PP_GRID_WIDTH;
  params->nb_input_boxes = AI_OD_YOLOV2_PP_NB_INPUT_BOXES;
  params->pAnchors = AI_OD_YOLOV2_PP_ANCHORS;
  params->max_boxes_limit = AI_OD_YOLOV2_PP_MAX_BOXES_LIMIT;
  params->pScratchBuffer = scratch_buffer;
  error = od_yolov2_pp_reset(params);
  return error;
}

int32_t app_postprocess_run(void *pInput[], int nb_input, void *pOutput, void *pInput_param)
{
  assert(nb_input == 1);
  int32_t error = AI_OD_POSTPROCESS_ERROR_NO;
  od_yolov2_pp_in_t pp_input = {
    .pRaw_detections = (float32_t *) pInput[0]
  };
  error = od_yolov2_pp_process_int8(&pp_input, (od_pp_out_t *) pOutput,
                                    (od_yolov2_pp_static_param_t *) pInput_param);
  return error;
}
#endif

