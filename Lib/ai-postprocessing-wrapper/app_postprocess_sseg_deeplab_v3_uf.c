 /**
 ******************************************************************************
 * @file    app_postprocess_sseg_deeplab_v3_uf.c
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


#if POSTPROCESS_TYPE == POSTPROCESS_SSEG_DEEPLAB_V3_UF
static uint8_t out_sseg_map[AI_SSEG_DEEPLABV3_PP_WIDTH * AI_SSEG_DEEPLABV3_PP_HEIGHT];

int32_t app_postprocess_init(void *params_postprocess, NN_Instance_TypeDef *NN_Instance)
{
  int32_t error = AI_SSEG_POSTPROCESS_ERROR_NO;
  sseg_deeplabv3_pp_static_param_t *params = (sseg_deeplabv3_pp_static_param_t *) params_postprocess;
  params->nb_classes = AI_SSEG_DEEPLABV3_PP_NB_CLASSES;
  params->width = AI_SSEG_DEEPLABV3_PP_WIDTH;
  params->height = AI_SSEG_DEEPLABV3_PP_HEIGHT;
  error = sseg_deeplabv3_pp_reset(params);
  return error;
}

int32_t app_postprocess_run(void *pInput[], int nb_input, void *pOutput, void *pInput_param)
{
  assert(nb_input == 1);
  int32_t error = AI_SSEG_POSTPROCESS_ERROR_NO;
  sseg_pp_out_t *pSsegOutput = (sseg_pp_out_t *) pOutput;
  pSsegOutput->pOutBuff = out_sseg_map;
  sseg_deeplabv3_pp_in_t pp_input = {
    .pRawData = (float32_t *) pInput[0]
  };
  error = sseg_deeplabv3_pp_process(&pp_input, (sseg_pp_out_t *) pOutput,
                                    (sseg_deeplabv3_pp_static_param_t *) pInput_param);
  return error;
}
#endif

