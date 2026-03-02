/**
 ******************************************************************************
 * @file    freertos_platform.h
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

#ifndef BSP_FREERTOS_PLATFORM_H
#define BSP_FREERTOS_PLATFORM_H

#include <stdint.h>
#include "stm32n6xx_hal.h"

void TIM4_Config(void);
uint32_t TIM4_Get_Value(void);

#endif /* BSP_FREERTOS_PLATFORM_H */

