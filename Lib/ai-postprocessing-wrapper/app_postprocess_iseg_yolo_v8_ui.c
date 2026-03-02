 /**
 ******************************************************************************
 * @file    app_postprocess_iseg_yolo_v8_ui.c
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

#if POSTPROCESS_TYPE == POSTPROCESS_ISEG_YOLO_V8_UI
uint8_t _iseg_mask[AI_YOLOV8_SEG_PP_MASK_SIZE * AI_YOLOV8_SEG_PP_MASK_SIZE * AI_YOLOV8_SEG_PP_MAX_BOXES_LIMIT];
iseg_pp_outBuffer_t out_detections[AI_YOLOV8_SEG_PP_MAX_BOXES_LIMIT];
iseg_yolov8_pp_scratchBuffer_s8_t scratch_detections[AI_YOLOV8_SEG_PP_TOTAL_BOXES];
float32_t _out_buf_mask[AI_YOLOV8_SEG_PP_MASK_NB];
int8_t _out_buf_mask_s8[AI_YOLOV8_SEG_PP_MASK_NB * AI_YOLOV8_SEG_PP_TOTAL_BOXES];

int32_t app_postprocess_init(void *params_postprocess, NN_Instance_TypeDef *NN_Instance)
{
  int32_t error = AI_ISEG_POSTPROCESS_ERROR_NO;
  iseg_yolov8_pp_static_param_t *params = (iseg_yolov8_pp_static_param_t *) params_postprocess;
  params->nb_classes = AI_YOLOV8_SEG_PP_NB_CLASSES;
  params->nb_total_boxes = AI_YOLOV8_SEG_PP_TOTAL_BOXES;
  params->max_boxes_limit = AI_YOLOV8_SEG_PP_MAX_BOXES_LIMIT;
  params->conf_threshold = AI_YOLOV8_SEG_PP_CONF_THRESHOLD;
  params->iou_threshold = AI_YOLOV8_SEG_PP_IOU_THRESHOLD;
  params->size_masks = AI_YOLOV8_SEG_PP_MASK_SIZE;
  params->raw_output_scale = AI_YOLOV8_SEG_SCALE;
  params->raw_output_zero_point = AI_YOLOV8_SEG_ZERO_POINT;
  params->nb_masks = AI_YOLOV8_SEG_PP_MASK_NB;
  params->mask_raw_output_zero_point = AI_YOLOV8_SEG_MASK_ZERO_POINT;
  params->mask_raw_output_scale = AI_YOLOV8_SEG_MASK_SCALE;
  params->pMask = _out_buf_mask;
  params->pTmpBuff = scratch_detections;
  for (size_t i = 0; i < AI_YOLOV8_SEG_PP_TOTAL_BOXES; i++) {
    scratch_detections[i].pMask = &_out_buf_mask_s8[i * AI_YOLOV8_SEG_PP_MASK_NB];
  }
  for (size_t i = 0; i < AI_YOLOV8_SEG_PP_MAX_BOXES_LIMIT; i++) {
    out_detections[i].pMask = &_iseg_mask[i * AI_YOLOV8_SEG_PP_MASK_SIZE * AI_YOLOV8_SEG_PP_MASK_SIZE];
  }
  error = iseg_yolov8_pp_reset(params);
  return error;
}

int32_t app_postprocess_run(void *pInput[], int nb_input, void *pOutput, void *pInput_param)
{
  assert(nb_input == 2);
  int32_t error = AI_ISEG_POSTPROCESS_ERROR_NO;
  iseg_pp_out_t *pSegOutput = (iseg_pp_out_t *) pOutput;
  pSegOutput->pOutBuff = out_detections;
  iseg_yolov8_pp_in_centroid_t pp_input =
  {
      .pRaw_detections = (int8_t *) pInput[0],
      .pRaw_masks = (int8_t *) pInput[1]
  };
  error = iseg_yolov8_pp_process_int8(&pp_input, pOutput,
                                      (iseg_yolov8_pp_static_param_t *) pInput_param);

  return error;
}
#endif

