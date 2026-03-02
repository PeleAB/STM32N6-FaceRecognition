 /**
 ******************************************************************************
 * @file    app_postprocess_spe_movenet_ui.c
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


#if POSTPROCESS_TYPE == POSTPROCESS_SPE_MOVENET_UI
static spe_pp_outBuffer_t out_detections[AI_POSE_PP_POSE_KEYPOINTS_NB];

int32_t app_postprocess_init(void *params_postprocess, NN_Instance_TypeDef *NN_Instance)
{
  int32_t error = AI_SPE_POSTPROCESS_ERROR_NO;
  spe_movenet_pp_static_param_t *params = (spe_movenet_pp_static_param_t *) params_postprocess;
  const LL_Buffer_InfoTypeDef *buffers_info = LL_ATON_Output_Buffers_Info(NN_Instance);
  params->raw_scale = *(buffers_info[0].scale);
  params->raw_zero_point = *(buffers_info[0].offset);
  params->heatmap_width = AI_SPE_MOVENET_POSTPROC_HEATMAP_WIDTH;
  params->heatmap_height = AI_SPE_MOVENET_POSTPROC_HEATMAP_HEIGHT;
  params->nb_keypoints = AI_POSE_PP_POSE_KEYPOINTS_NB;
  error = spe_movenet_pp_reset(params);
  return error;
}

int32_t app_postprocess_run(void *pInput[], int nb_input, void *pOutput, void *pInput_param)
{
  assert(nb_input == 1);
  int32_t error = AI_SPE_POSTPROCESS_ERROR_NO;
  spe_pp_out_t *pPoseOutput = (spe_pp_out_t *) pOutput;
  pPoseOutput->pOutBuff = out_detections;
  spe_movenet_pp_in_t pp_input =
  {
      .inBuff = (float32_t *) pInput[0]
  };
  error = spe_movenet_pp_process_int8(&pp_input, pPoseOutput,
                                 (spe_movenet_pp_static_param_t *) pInput_param);
  return error;
}
#endif

