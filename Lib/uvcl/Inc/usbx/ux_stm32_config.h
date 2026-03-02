 /**
 ******************************************************************************
 * @file    ux_stm32_config.h
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
#ifndef __UX_STM32_CONFIG_H__
#define __UX_STM32_CONFIG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32n6xx_hal.h"

#define UX_DCD_STM32_MAX_ED 9

#define USBD_HAL_ISOINCOMPLETE_CALLBACK

#ifdef __cplusplus
}
#endif
#endif  /* __UX_STM32_CONFIG_H__ */

