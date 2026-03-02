 /**
 ******************************************************************************
 * @file    app_postprocess_mpe_yolo_v8_uf.c
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

#if POSTPROCESS_TYPE == POSTPROCESS_MPE_YOLO_V8_UI
static mpe_pp_outBuffer_t out_detections[AI_MPE_YOLOV8_PP_TOTAL_BOXES];
static mpe_pp_keyPoints_t out_keyPoints[AI_MPE_YOLOV8_PP_TOTAL_BOXES * AI_POSE_PP_POSE_KEYPOINTS_NB];
static mpe_pp_keyPoints_s8_t scratchBuffer_keyPoints[AI_MPE_YOLOV8_PP_TOTAL_BOXES * AI_POSE_PP_POSE_KEYPOINTS_NB];
static mpe_pp_scratchBuffer_s8_t scratchBuffer_detections[AI_MPE_YOLOV8_PP_TOTAL_BOXES];

int32_t app_postprocess_init(void *params_postprocess, NN_Instance_TypeDef *NN_Instance)
{
  int32_t error = AI_MPE_PP_ERROR_NO;
  mpe_yolov8_pp_static_param_t *params = (mpe_yolov8_pp_static_param_t *) params_postprocess;
  const LL_Buffer_InfoTypeDef *buffers_info = LL_ATON_Output_Buffers_Info(NN_Instance);
  params->raw_output_scale = *(buffers_info[0].scale);
  params->raw_output_zero_point = *(buffers_info[0].offset);
  params->nb_classes = AI_MPE_YOLOV8_PP_NB_CLASSES;
  params->nb_total_boxes = AI_MPE_YOLOV8_PP_TOTAL_BOXES;
  params->max_boxes_limit = AI_MPE_YOLOV8_PP_MAX_BOXES_LIMIT;
  params->conf_threshold = AI_MPE_YOLOV8_PP_CONF_THRESHOLD;
  params->iou_threshold = AI_MPE_YOLOV8_PP_IOU_THRESHOLD;
  params->nb_keypoints = AI_POSE_PP_POSE_KEYPOINTS_NB;
  for (int i = 0; i < AI_MPE_YOLOV8_PP_TOTAL_BOXES; i++) {
    out_detections[i].pKeyPoints = &out_keyPoints[i * AI_POSE_PP_POSE_KEYPOINTS_NB];
  }
  for (int i = 0; i < AI_MPE_YOLOV8_PP_TOTAL_BOXES; i++) {
    scratchBuffer_detections[i].pKeyPoints = &scratchBuffer_keyPoints[i * AI_POSE_PP_POSE_KEYPOINTS_NB];
  }
  params->pScratchBuffer = scratchBuffer_detections;
  error = mpe_yolov8_pp_reset(params);
  return error;
}

int32_t app_postprocess_run(void *pInput[], int nb_input, void *pOutput, void *pInput_param)
{
  assert(nb_input == 1);
  int32_t error = AI_MPE_PP_ERROR_NO;
  mpe_pp_out_t *pPoseOutput = (mpe_pp_out_t *) pOutput;
  pPoseOutput->pOutBuff = out_detections;
  mpe_yolov8_pp_in_centroid_t pp_input =
  {
      .pRaw_detections = (float32_t *) pInput[0]
  };
  error = mpe_yolov8_pp_process_int8(&pp_input, pPoseOutput,
                                     (mpe_yolov8_pp_static_param_t *) pInput_param);

  return error;
}
#endif

