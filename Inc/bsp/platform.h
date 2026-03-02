/**
 ******************************************************************************
 * @file    platform.h
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

#ifndef BSP_PLATFORM_H
#define BSP_PLATFORM_H

#include "stm32n6xx_hal.h"

void SystemClock_Config(void);
void NPURam_enable(void);
void NPUCache_config(void);
void Security_Config(void);
void IAC_Config(void);
void CONSOLE_Config(void);
void Setup_Mpu(void);
void DMA2D_Config(void);
void BSP_EarlyPlatformInit(void);
void BSP_PlatformInit(void);

HAL_StatusTypeDef MX_DCMIPP_ClockConfig(DCMIPP_HandleTypeDef *hdcmipp);
void HAL_PCD_MspInit(PCD_HandleTypeDef *hpcd);
void HAL_CACHEAXI_MspInit(CACHEAXI_HandleTypeDef *hcacheaxi);
void HAL_CACHEAXI_MspDeInit(CACHEAXI_HandleTypeDef *hcacheaxi);

void IAC_IRQHandler(void);

#endif /* BSP_PLATFORM_H */

