/**
  ******************************************************************************
  * @file    cmw_vd66gy.h
  * @author  MDG Application Team
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

#ifndef CMW_VD6G
#define CMW_VD6G

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdint.h>
#include "cmw_sensors_if.h"
#include "cmw_errno.h"
#include "vd6g.h"
#include "stm32n6xx_hal_dcmipp.h"
#include "isp_api.h"

#define VD66GY_CHIP_ID 0x5603
#define VD66GY_NAME    "VD66GY"

typedef struct
{
  uint16_t Address;
  uint32_t ClockInHz;
  VD6G_Ctx_t ctx_driver;
  ISP_HandleTypeDef hIsp;
  ISP_AppliHelpersTypeDef appliHelpers;
  DCMIPP_HandleTypeDef *hdcmipp;
  uint8_t IsInitialized;
  int32_t (*Init)(void);
  int32_t (*DeInit)(void);
  int32_t (*WriteReg)(uint16_t, uint16_t, uint8_t*, uint16_t);
  int32_t (*ReadReg) (uint16_t, uint16_t, uint8_t*, uint16_t);
  int32_t (*GetTick) (void);
  void (*Delay)(uint32_t delay_in_ms);
  void (*ShutdownPin)(int value);
  void (*EnablePin)(int value);
} CMW_VD66GY_t;

int CMW_VD66GY_Probe(CMW_VD66GY_t *io_ctx, CMW_Sensor_if_t *vd6g_if);

#ifdef __cplusplus
}
#endif

#endif
