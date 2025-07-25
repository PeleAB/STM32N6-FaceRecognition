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
#else
#error Add header files for your specific board
#endif


/* ########################## Module Selection ############################## */
/**
  * @brief This is the list of modules to be used in the HAL driver
  */
#define USE_IMX335_SENSOR
#define USE_VD66GY_SENSOR
#define USE_VD55G1_SENSOR

#ifdef __cplusplus
}
#endif

#endif /* CMW_CAMERA_CONF_H */
