 /**
 ******************************************************************************
 * @file    utils.c
 * @author  GPM Application Team
 *
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

#include "utils.h"

void Run_Inference(NN_Instance_TypeDef *network_instance)
{
  LL_ATON_RT_RetValues_t ll_aton_rt_ret;

  do {
    /* Execute first/next step of Cube.AI/ATON runtime */
    ll_aton_rt_ret = LL_ATON_RT_RunEpochBlock(network_instance);
    /* Wait for next event */
    if (ll_aton_rt_ret == LL_ATON_RT_WFE)
      LL_ATON_OSAL_WFE();
  } while (ll_aton_rt_ret != LL_ATON_RT_DONE);

  LL_ATON_RT_Reset_Network(network_instance);
}

