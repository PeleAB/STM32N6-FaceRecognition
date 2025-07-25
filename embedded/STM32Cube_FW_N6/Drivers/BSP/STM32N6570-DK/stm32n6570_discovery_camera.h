/**
  ******************************************************************************
  * @file    stm32n6570_discovery_camera.h
  * @author  MCD Application Team
  * @brief   This file contains the common defines and functions prototypes for
  *          the stm32n6570_discovery_camera.c driver.
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
#ifndef STM32N6570_DISCOVERY_CAMERA_H
#define STM32N6570_DISCOVERY_CAMERA_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32n6570_discovery_errno.h"
#include "stm32n6570_discovery_conf.h"
#include "camera.h"
#include "imx335.h"
#include "isp_api.h"

/** @addtogroup BSP
  * @{
  */

/** @addtogroup STM32N6570_DISCOVERY
  * @{
  */


/** @defgroup STM32N6570_DISCOVERY_CAMERA CAMERA
  * @{
  */

/** @defgroup STM32N6570_DISCOVERY_CAMERA_Exported_Types CAMERA Exported Types
  * @{
  */
typedef struct
{
  uint32_t Resolution;
  uint32_t PixelFormat;
  uint32_t LightMode;
  uint32_t ColorEffect;
  int32_t  Brightness;
  int32_t  Saturation;
  int32_t  Contrast;
  int32_t  HueDegree;
  uint32_t MirrorFlip;
  uint32_t Zoom;
  uint32_t NightMode;
  uint32_t IsMspCallbacksValid;
} CAMERA_Ctx_t;

typedef struct
{
  uint32_t Resolution;
  uint32_t LightMode;
  uint32_t SpecialEffect;
  uint32_t Brightness;
  uint32_t Saturation;
  uint32_t Contrast;
  uint32_t HueDegree;
  uint32_t Gain;
  uint32_t Exposure;
  uint32_t MirrorFlip;
  uint32_t Zoom;
  uint32_t NightMode;
  uint32_t ExposureMode;
  uint32_t SensorInfo;
  uint32_t TestPattern;
  uint32_t ColorEffect;
} CAMERA_Capabilities_t;

#if (USE_HAL_DCMIPP_REGISTER_CALLBACKS > 0)
typedef struct
{
  void (* pMspInitCb)(DCMIPP_HandleTypeDef *);
  void (* pMspDeInitCb)(DCMIPP_HandleTypeDef *);
} BSP_CAMERA_Cb_t;
#endif /* (USE_HAL_DCMIPP_REGISTER_CALLBACKS > 0) */
/**
  * @}
  */

/** @defgroup STM32N6570_DISCOVERY_CAMERA_Exported_Constants CAMERA Exported Constants
  * @{
  */
/* Camera instance number */
#define CAMERA_INSTANCES_NBR            1U

/* Camera capture mode */
#define CAMERA_MODE_CONTINUOUS          DCMIPP_MODE_CONTINUOUS
#define CAMERA_MODE_SNAPSHOT            DCMIPP_MODE_SNAPSHOT

/* Camera resolutions */

#define CAMERA_R2592x1944               IMX335_R2592_1944 /* 2592x1940 Resolution */

/* Camera Pixel Format */
#define CAMERA_PF_RAW_RGGB10            IMX335_RAW_RGGB10 /* Pixel Format RAW_RGGB10 */


/* Brightness */
#define CAMERA_BRIGHTNESS_MIN          -4
#define CAMERA_BRIGHTNESS_MAX           4

/* Saturation */
#define CAMERA_SATURATION_MIN          -4
#define CAMERA_SATURATION_MAX           4

/* Contrast */
#define CAMERA_CONTRAST_MIN            -4
#define CAMERA_CONTRAST_MAX             4

/* Hue Control */
#define CAMERA_HUEDEGREE_MIN           -6
#define CAMERA_HUEDEGREE_MAX            5

/* Mirror/Flip */
#define CAMERA_MIRRORFLIP_NONE          0x00U   /* Set camera normal mode     */
#define CAMERA_MIRRORFLIP_FLIP          0x01U   /* Set camera flip config     */
#define CAMERA_MIRRORFLIP_MIRROR        0x02U   /* Set camera mirror config   */

/* Zoom */
#define CAMERA_ZOOM_x8                  0x00U   /* Set zoom to x8             */
#define CAMERA_ZOOM_x4                  0x11U   /* Set zoom to x4             */
#define CAMERA_ZOOM_x2                  0x22U   /* Set zoom to x2             */
#define CAMERA_ZOOM_x1                  0x44U   /* Set zoom to x1             */

/* Color Effect */
#define CAMERA_COLOR_EFFECT_NONE        0x00U   /* No effect                  */
#define CAMERA_COLOR_EFFECT_BLUE        0x01U   /* Blue effect                */
#define CAMERA_COLOR_EFFECT_RED         0x02U   /* Red effect                 */
#define CAMERA_COLOR_EFFECT_GREEN       0x04U   /* Green effect               */
#define CAMERA_COLOR_EFFECT_BW          0x08U   /* Black and White effect     */
#define CAMERA_COLOR_EFFECT_SEPIA       0x10U   /* Sepia effect               */
#define CAMERA_COLOR_EFFECT_NEGATIVE    0x20U   /* Negative effect            */

/* Light Mode */
#define CAMERA_LIGHT_AUTO               0x00U   /* Light Mode Auto            */
#define CAMERA_LIGHT_SUNNY              0x01U   /* Light Mode Sunny           */
#define CAMERA_LIGHT_OFFICE             0x02U   /* Light Mode Office          */
#define CAMERA_LIGHT_HOME               0x04U   /* Light Mode Home            */
#define CAMERA_LIGHT_CLOUDY             0x08U   /* Light Mode Claudy          */

/* Night Mode */
#define CAMERA_NIGHT_MODE_SET           0x00U   /* Disable night mode         */
#define CAMERA_NIGHT_MODE_RESET         0x01U   /* Enable night mode          */

#define NRST_CAM_PIN                    GPIO_PIN_8
#define NRST_CAM_PORT                   GPIOC

#define EN_CAM_PIN                      GPIO_PIN_2
#define EN_CAM_PORT                     GPIOD

#define CAMERA_IMX335_ADDRESS          0x34U

#define LCD_FRAME_BUFFER_SIZE          640*480*2U

/**
  * @}
  */

/** @addtogroup STM32N6570_DISCOVERY_CAMERA_Exported_Variables
  * @{
  */
extern void                *Camera_CompObj;
extern DCMIPP_HandleTypeDef hcamera_dcmipp;
extern CAMERA_Ctx_t         Camera_Ctx[];
extern ISP_HandleTypeDef    hcamera_isp;
/**
  * @}
  */

/** @addtogroup STM32N6570_DISCOVERY_CAMERA_Exported_Functions
  * @{
  */
int32_t BSP_CAMERA_Init(uint32_t Instance, uint32_t Resolution, uint32_t PixelFormat);
int32_t BSP_CAMERA_DeInit(uint32_t Instance);
#if (USE_HAL_DCMIPP_REGISTER_CALLBACKS > 0)
int32_t BSP_CAMERA_RegisterDefaultMspCallbacks(uint32_t Instance);
int32_t BSP_CAMERA_RegisterMspCallbacks(uint32_t Instance, BSP_CAMERA_Cb_t *CallBacks);
#endif /* (USE_HAL_DCMIPP_REGISTER_CALLBACKS > 0) */
int32_t BSP_CAMERA_Start(uint32_t Instance, uint8_t *pbuff, uint32_t Mode);
int32_t BSP_CAMERA_DoubleBufferStart(uint32_t Instance, uint8_t *pbuff1, uint8_t *pbuff2, uint32_t Mode);
int32_t BSP_CAMERA_Suspend(uint32_t Instance);
int32_t BSP_CAMERA_Resume(uint32_t Instance);
int32_t BSP_CAMERA_Stop(uint32_t Instance);
int32_t BSP_CAMERA_GetCapabilities(uint32_t Instance, CAMERA_Capabilities_t *Capabilities);

int32_t BSP_CAMERA_SetResolution(uint32_t Instance, uint32_t Resolution);
int32_t BSP_CAMERA_GetResolution(uint32_t Instance, uint32_t *Resolution);

int32_t BSP_CAMERA_SetPixelFormat(uint32_t Instance, uint32_t PixelFormat);
int32_t BSP_CAMERA_GetPixelFormat(uint32_t Instance, uint32_t *PixelFormat);

int32_t BSP_CAMERA_SetLightMode(uint32_t Instance, uint32_t LightMode);
int32_t BSP_CAMERA_GetLightMode(uint32_t Instance, uint32_t *LightMode);

int32_t BSP_CAMERA_SetColorEffect(uint32_t Instance, uint32_t ColorEffect);
int32_t BSP_CAMERA_GetColorEffect(uint32_t Instance, uint32_t *ColorEffect);

int32_t BSP_CAMERA_SetBrightness(uint32_t Instance, int32_t Brightness);
int32_t BSP_CAMERA_GetBrightness(uint32_t Instance, int32_t *Brightness);

int32_t BSP_CAMERA_SetSaturation(uint32_t Instance, int32_t Saturation);
int32_t BSP_CAMERA_GetSaturation(uint32_t Instance, int32_t *Saturation);

int32_t BSP_CAMERA_SetContrast(uint32_t Instance, int32_t Contrast);
int32_t BSP_CAMERA_GetContrast(uint32_t Instance, int32_t *Contrast);

int32_t BSP_CAMERA_SetHueDegree(uint32_t Instance, int32_t HueDegree);
int32_t BSP_CAMERA_GetHueDegree(uint32_t Instance, int32_t *HueDegree);

int32_t BSP_CAMERA_SetMirrorFlip(uint32_t Instance, uint32_t MirrorFlip);
int32_t BSP_CAMERA_GetMirrorFlip(uint32_t Instance, uint32_t *MirrorFlip);

int32_t BSP_CAMERA_SetZoom(uint32_t Instance, uint32_t Zoom);
int32_t BSP_CAMERA_GetZoom(uint32_t Instance, uint32_t *Zoom);

int32_t BSP_CAMERA_EnableNightMode(uint32_t Instance);
int32_t BSP_CAMERA_DisableNightMode(uint32_t Instance);

int32_t BSP_CAMERA_HwReset(uint32_t Instance);
int32_t BSP_CAMERA_PwrDown(uint32_t Instance);


void    BSP_CAMERA_LineEventCallback(uint32_t Instance);
void    BSP_CAMERA_FrameEventCallback(uint32_t Instance);
void    BSP_CAMERA_VsyncEventCallback(uint32_t Instance);
void    BSP_CAMERA_PipeErrorCallback(uint32_t Instance);
void    BSP_CAMERA_ErrorCallback(uint32_t Instance);

void    BSP_CAMERA_IRQHandler(uint32_t Instance);

int32_t BSP_CAMERA_BackgroundProcess(void);

/* This function can be modified in case the current settings (e.g. Pipe config, DCMIPP and CSI config)
   need to be changed for specific application needs */
HAL_StatusTypeDef MX_DCMIPP_Init(DCMIPP_HandleTypeDef *hdcmipp);
HAL_StatusTypeDef MX_DCMIPP_ClockConfig(DCMIPP_HandleTypeDef *hdcmipp);


/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* STM32N6570_DISCOVERY_CAMERA_H */
