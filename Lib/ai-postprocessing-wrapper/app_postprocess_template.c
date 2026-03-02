 /**
 ******************************************************************************
 * @file    app_postprocess_template.c
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

 #if POSTPROCESS_TYPE == POSTPROCESS_CUSTOM
 int32_t app_postprocess_init(void *params_postprocess, NN_Instance_TypeDef *NN_Instance)
 {
 // @User must implement its own app_postprocess_init
 return error;
}

int32_t app_postprocess_run(void *pInput[], int nb_input, void *pOutput, void *pInput_param)
{
   // @User must implement its own app_postprocess_run
   return error;
 }
 #endif

