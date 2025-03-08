/**
 ******************************************************************************
 * @file    isp_platform.h
 * @author  AIS Application Team
 * @brief   Header file of ISP middleware services.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __ISP_PLATFORM_H
#define __ISP_PLATFORM_H


/* Includes ------------------------------------------------------------------*/
#if defined (STM32N657xx)
#include "stm32n6xx_hal.h"
#else
#error Add header files for your specific board
#endif

#endif /* __ISP_PLATFORM_H */
