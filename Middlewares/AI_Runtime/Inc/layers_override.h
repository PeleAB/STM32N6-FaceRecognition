/**
  ******************************************************************************
  * @file    layers_override.h
  * @author  AST Embedded Analytics Research Platform
  * @brief   header file of AI platform layers override defines
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2018 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
#ifndef LAYERS_OVERRIDE_H
#define LAYERS_OVERRIDE_H

#include "ai_common_config.h"


/*!
 * @defgroup layers Layers Override Definitions
 * @brief definition
 *
 */

/* Override default fixed point functions with optimized STM32 versions */
//#ifdef HAS_STM32
  #define DENSE_FIXED_FUNC_OVERRIDE
  #define RELU_FIXED_FUNC_OVERRIDE
  #define RELU_MAX_FIXED_FUNC_OVERRIDE
  #define TANH_FIXED_FUNC_OVERRIDE
  #define SIGMOID_FIXED_FUNC_OVERRIDE
  #define SOFTMAX_FIXED_FUNC_OVERRIDE
  #define CONV2D_FIXED_FUNC_OVERRIDE
  #define CONV2D_NL_POOL_FIXED_FUNC_OVERRIDE
  #define MAXPOOL_FIXED_FUNC_OVERRIDE
  #define AVPOOL_FIXED_FUNC_OVERRIDE
//#else /* HAS_STM32 */
  /* Default fixed point implementations not available */
//#endif /* HAS_STM32 */

/* Override default integer-only functions with optimized STM32 versions */
//#ifdef HAS_STM32
  #define DENSE_INTEGER_FUNC_OVERRIDE
  #define RELU_INTEGER_FUNC_OVERRIDE
  #define TANH_INTEGER_FUNC_OVERRIDE
  #define SIGMOID_INTEGER_FUNC_OVERRIDE
  #define SOFTMAX_INTEGER_FUNC_OVERRIDE
  #define CONV2D_INTEGER_FUNC_OVERRIDE
  #define CONV2D_NL_POOL_INTEGER_FUNC_OVERRIDE
  #define MAXPOOL_INTEGER_FUNC_OVERRIDE
  #define AVPOOL_INTEGER_FUNC_OVERRIDE
  #define BN_INTEGER_FUNC_OVERRIDE
  #define DW_SSSA8_CH_FUNC_OVERRIDE
  #define DW_SSSA8_CH_NL_POOL_FUNC_OVERRIDE
  #define PW_SSSA8_CH_FUNC_OVERRIDE
  #define PW_SSSA8_CH_NL_POOL_FUNC_OVERRIDE
  #define CONV2D_RGB_SSSA8_CH_FUNC_OVERRIDE
  #define CONV2D_SSSA8_CH_FUNC_OVERRIDE
  #define CONV2D_SSSA8_CH_NL_POOL_FUNC_OVERRIDE

//#else /* HAS_STM32 */
  /* Default fixed point implementations not available */
//#endif /* HAS_STM32 */

#endif    /*LAYERS_OVERRIDE_H*/
