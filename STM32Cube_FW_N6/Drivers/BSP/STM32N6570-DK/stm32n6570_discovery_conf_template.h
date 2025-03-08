/**
  ******************************************************************************
  * @file    stm32n6570_discovery_conf.h
  * @author  MCD Application Team
  * @brief   configuration file.
  *          This file should be copied to the application folder and renamed
  *          to stm32n6570_discovery_conf.h
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
#ifndef STM32N6570_DISCOVERY_CONF_H
#define STM32N6570_DISCOVERY_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32n6xx_hal.h"

/* COM defines */
#define USE_COM_LOG                         0U
#define USE_BSP_COM_FEATURE                 0U

/* Touch Sensing controller defines */
#define USE_FT5336_TS_CTRL                  1U
/* TS supported features defines */
#define USE_TS_GESTURE                      1U
#define USE_TS_MULTI_TOUCH                  1U
/* Default TS touch number */
#define TS_TOUCH_NBR                        2U

/* LCD defines */
/* The values provided as example below point at SRAM1 area and allow
   to fit two layers in 800x480 RGB-565 format.

   To use SRAM3, SRAM4, SRAM5 or SRAM6, it is required
   1/ to clock them
       e.g. LL_MEM_EnableClock(LL_MEM_AXISRAMi);
   2/ to clear the shutdown bit SRAMSD of RAMCFG_AXISRAMxCR register
*/
#define LCD_LAYER_0_ADDRESS                 0x34000000 /* SRAM1 */
#define LCD_LAYER_1_ADDRESS                 0x340C0000 /* SRAM1 */


/* Audio codec defines */
#define USE_AUDIO_CODEC_WM8904

/* Default Audio IN internal buffer size */
#define DEFAULT_AUDIO_IN_BUFFER_SIZE        2048U

/* IRQ priorities (Default is 15 as lowest priority level) */
#define BSP_SDRAM_IT_PRIORITY               15U
#define BSP_BUTTON_USER1_IT_PRIORITY        15U
#define BSP_BUTTON_USER2_IT_PRIORITY        15U
#define BSP_BUTTON_TAMP_IT_PRIORITY         15U
#define BSP_AUDIO_OUT_IT_PRIORITY           14U
#define BSP_AUDIO_IN_IT_PRIORITY            15U
#define BSP_SD_IT_PRIORITY                  14U
#define BSP_SD_RX_IT_PRIORITY               14U
#define BSP_SD_TX_IT_PRIORITY               15U
#define BSP_TS_IT_PRIORITY                  15U

#define BSP_CAMERA_ISP_DEFAULT_WHITE_BALANCE    255U
#define BSP_CAMERA_ISP_DEFAULT_EXPOSURE         128U
#define BSP_CAMERA_ISP_DEFAULT_CONTRAST         130U
#define BSP_CAMERA_ISP_STATISTICS_AREA_HEIGHT   1940
#define BSP_CAMERA_ISP_STATISTICS_AREA_WIDTH    2592

#ifdef __cplusplus
}
#endif

#endif /* STM32N6570_DISCOVERY_CONF_H */
