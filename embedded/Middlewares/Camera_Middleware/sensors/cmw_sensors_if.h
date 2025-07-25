/**
  ******************************************************************************
  * @file    cmw_sensors_if.h
  * @author  GPM Application Team
  * @brief   This header file contains the common defines and functions prototypes
  *          for the camera driver.
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

#ifndef CMW_SENSORS_IF
#define CMW_SENSORS_IF

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "isp_core.h"

typedef struct
{
  uint32_t width;
  uint32_t height;
  int fps;
  uint32_t pixel_format;
  uint32_t mirrorFlip;
} CMW_Sensor_Init_t;

typedef struct
{
  int32_t (*Init)(void *, CMW_Sensor_Init_t *);
  int32_t (*DeInit)(void *);
  int32_t (*Start)(void *);
  int32_t (*Run)(void *);
  void    (*VsyncEventCallback)(void *, uint32_t);
  void    (*FrameEventCallback)(void *, uint32_t);
  int32_t (*Stop)(void *);
  int32_t (*ReadID)(void *, uint32_t *);
  int32_t (*SetBrightness)(void *, int32_t);
  int32_t (*SetSaturation)(void *, int32_t);
  int32_t (*SetContrast)(void *, int32_t);
  int32_t (*SetMirrorFlip)(void *, uint32_t);
  int32_t (*SetResolution)(void *, uint32_t);
  int32_t (*GetResolution)(void *, uint32_t *);
  int32_t (*SetPixelFormat)(void *, uint32_t);
  int32_t (*GetPixelFormat)(void *, uint32_t *);
  int32_t (*SetFrequency)(void *, int32_t);
  int32_t (*SetFramerate)(void*, int32_t);
  int32_t (*SetGain)(void *, int32_t);
  int32_t (*SetExposure)(void *, int32_t);
  int32_t (*SetExposureMode)(void *, int32_t);
  int32_t (*SetFlickerMode)(void *, int32_t);
  int32_t (*GetSensorInfo)(void *, ISP_SensorInfoTypeDef *);
  int32_t (*SetTestPattern)(void *, int32_t);
} CMW_Sensor_if_t;

#ifdef __cplusplus
}
#endif

#endif /* CMW_SENSORS_IF */
