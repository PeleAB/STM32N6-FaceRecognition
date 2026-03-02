 /**
 ******************************************************************************
 * @file    app_postprocess_od_yolov8_ui.c
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

#if POSTPROCESS_TYPE == POSTPROCESS_OD_YOLO_V8_UI
static int8_t scratch_buffer[AI_OD_YOLOV8_PP_TOTAL_BOXES * 6];
static od_pp_outBuffer_t out_detections[AI_OD_YOLOV8_PP_TOTAL_BOXES];

int32_t app_postprocess_init(void *params_postprocess, NN_Instance_TypeDef *NN_Instance)
{
  int32_t error = AI_OD_POSTPROCESS_ERROR_NO;
  od_yolov8_pp_static_param_t *params = (od_yolov8_pp_static_param_t *) params_postprocess;
  const LL_Buffer_InfoTypeDef *buffers_info = LL_ATON_Output_Buffers_Info(NN_Instance);
  params->raw_output_scale = *(buffers_info[0].scale);
  params->raw_output_zero_point = *(buffers_info[0].offset);
  params->nb_classes = AI_OD_YOLOV8_PP_NB_CLASSES;
  params->nb_total_boxes = AI_OD_YOLOV8_PP_TOTAL_BOXES;
  params->max_boxes_limit = AI_OD_YOLOV8_PP_MAX_BOXES_LIMIT;
  params->conf_threshold = AI_OD_YOLOV8_PP_CONF_THRESHOLD;
  params->iou_threshold = AI_OD_YOLOV8_PP_IOU_THRESHOLD;
  params->pScratchBuff = scratch_buffer;
  error = od_yolov8_pp_reset(params);

  return error;
}

int32_t app_postprocess_run(void *pInput[], int nb_input, void *pOutput, void *pInput_param)
{
  assert(nb_input == 1);
  int32_t error = AI_OD_POSTPROCESS_ERROR_NO;
  od_pp_out_t *pObjDetOutput = (od_pp_out_t *) pOutput;
  pObjDetOutput->pOutBuff = out_detections;
  od_yolov8_pp_in_centroid_t pp_input = {
      .pRaw_detections = (int8_t *) pInput[0]
  };
  error = od_yolov8_pp_process_int8(&pp_input, pObjDetOutput,
                                    (od_yolov8_pp_static_param_t *) pInput_param);
  return error;
}
#endif

