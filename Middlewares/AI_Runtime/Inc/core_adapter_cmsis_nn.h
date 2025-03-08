/**
  ******************************************************************************
  * @file    core_adapter_cmsis_nn.h
  * @author  AST Embedded Analytics Research Platform
  * @brief   Adapter / Wrapper interface for ARM CMSIS-NN
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
#ifndef __CORE_ADAPTER_CMSIS_NN_H_
#define __CORE_ADAPTER_CMSIS_NN_H_

#include <stdarg.h>

#include <arm_math.h>
#include "st_uint8_nnsupportfunctions.h"
#include "st_int8_nnsupportfunctions.h"
#include "st_sssa8_ch_nnfunctions.h"
#include "st_ssua8_ch_nnfunctions.h"
#include "st_uaua8_ch_nnfunctions.h"
#include "st_sssa8_nnfunctions.h"
#include "st_ssua8_nnfunctions.h"
#include "st_uaua8_nnfunctions.h"
#ifdef AI_CMSISNN_USAGE
#include <arm_nnfunctions.h>
#endif

#include "ai_platform.h"


#endif		/*__CORE_ADAPTER_CMSIS_NN_H_*/
