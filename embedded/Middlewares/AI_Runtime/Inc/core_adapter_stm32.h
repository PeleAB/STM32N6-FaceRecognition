/**
  ******************************************************************************
  * @file    core_adapter_stm32.h
  * @author  AST Embedded Analytics Research Platform
  * @brief   Adapter / Wrapper interface for STM32 MCUs
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  
  ******************************************************************************
  */
#ifndef CORE_ADAPTER_STM32_H
#define CORE_ADAPTER_STM32_H


#if defined HAS_STM32

#if defined USE_STM32_TIMER
#if !defined HAS_STM32_HAL
#define HAS_STM32_HAL
#endif
#endif

#if defined HAS_STM32_HAL

#if defined STM32L053xx
#include "stm32l0xx_hal.h"
#elif defined STM32L471xE
#include "stm32l4xx_hal.h"
#elif defined STM32F401xE
#include "stm32f4xx_hal.h"
#elif defined STM32F746xx
#include "stm32f7xx_hal.h"
#elif defined STM32H743xx
#include "stm32h7xx_hal.h"
#elif defined STM32L552xx
#include "stm32l5xx_hal.h"
#endif

#endif

#include <cmsis_compiler.h>

#if !defined(__WEAK)
#error __WEAK symbol for STM32 is not defined
#endif

#else       /* defined HAS_STM32 */

#ifndef __WEAK
#define __WEAK                 __attribute__((weak))
#endif

#endif      /* defined HAS_STM32 */

#define AI_DECLARE_WEAK        __WEAK

#endif      /*CORE_ADAPTER_STM32_H*/
