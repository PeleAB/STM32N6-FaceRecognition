 /**
 ******************************************************************************
 * @file    cmw_camera_conf.h
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef CMW_CAMERA_CONF_H
#define CMW_CAMERA_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#if defined (STM32N657xx)
#include "stm32n6xx_hal.h"
#ifdef STM32N6570_NUCLEO_REV
#include "stm32n6xx_nucleo_bus.h"
#else
#include "stm32n6570_discovery_bus.h"
#endif
#elif defined (STM32MP257Fxx)
#include "stm32mp2xx_hal.h"
#include "stm32mp257f_eval_bus.h"
#else
#error Add header files for your specific board
#endif



/* ########################## Module Selection ############################## */
/**
  * @brief This is the list of modules to be used in the HAL driver
  */

/* This is defined in Makefile or project */


#ifdef __cplusplus
}
#endif

#endif /* CMW_CAMERA_CONF_H */

