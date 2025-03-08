 /**
 ******************************************************************************
 * @file    cmw_camera.h
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
#ifndef CMW_CAMERA_H
#define CMW_CAMERA_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "cmw_errno.h"
#include "cmw_camera_conf.h"
#include "cmw_sensors_if.h"

/* Camera capture mode */
#define CMW_MODE_CONTINUOUS          DCMIPP_MODE_CONTINUOUS
#define CMW_MODE_SNAPSHOT            DCMIPP_MODE_SNAPSHOT

/* Mirror/Flip */
#define CMW_MIRRORFLIP_NONE          0x00U   /* Set camera normal mode          */
#define CMW_MIRRORFLIP_FLIP          0x01U   /* Set camera flip config          */
#define CMW_MIRRORFLIP_MIRROR        0x02U   /* Set camera mirror config        */
#define CMW_MIRRORFLIP_FLIP_MIRROR   0x03U   /* Set camera flip + mirror config */


typedef enum {
  CMW_VD66GY_Sensor = 0x0,
  CMW_IMX335_Sensor,
  CMW_VD55G1_Sensor,
} CMW_Sensor_Name_t;

typedef enum {
  CMW_Aspect_ratio_crop = 0x0,
  CMW_Aspect_ratio_fit,
  CMW_Aspect_ratio_fullscreen,
  CMW_Aspect_ratio_manual_roi,
} CMW_Aspect_Ratio_Mode_t;

typedef struct {
  uint32_t width;
  uint32_t height;
  uint32_t offset_x;
  uint32_t offset_y;
} CMW_Manual_roi_area_t;

typedef struct {
  /* Camera settings */
  uint32_t width;
  uint32_t height;
  int fps;
  uint32_t pixel_format;
  int anti_flicker;
  int mirror_flip;
} CMW_CameraInit_t;

typedef struct {
  /* pipe output settings */
  uint32_t output_width;
  uint32_t output_height;
  int output_format;
  int output_bpp;
  int enable_swap;
  int enable_gamma_conversion;
  /*Output buffer of the pipe*/
  int mode;
  /* You must fill manual_conf when mode is CMW_Aspect_ratio_manual_roi */
  CMW_Manual_roi_area_t manual_conf;
} CMW_DCMIPP_Conf_t;


/* Camera exposure mode
 * Some cameras embed their own Auto Exposure algorithm.
 * The following defines allow the user to chose the exposure mode of the camera.
 * Camera exposure mode has no impact if the camera does not support it.
 */
#define CMW_EXPOSUREMODE_AUTO          0x00U   /* Start the camera auto exposure functionnality */
#define CMW_EXPOSUREMODE_AUTOFREEZE    0x01U   /* Stop the camera auto exposure functionnality and freeze the current value */
#define CMW_EXPOSUREMODE_MANUAL        0x02U   /* Set the camera in manual exposure (exposure is control by a software algorithm) */

DCMIPP_HandleTypeDef* CMW_CAMERA_GetDCMIPPHandle();

int32_t CMW_CAMERA_Init( CMW_CameraInit_t *init_conf );
int32_t CMW_CAMERA_DeInit();
int32_t CMW_CAMERA_Run();
int32_t CMW_CAMERA_SetPipeConfig(uint32_t pipe, CMW_DCMIPP_Conf_t *p_conf, uint32_t *pitch);
int32_t CMW_CAMERA_GetSensorName(CMW_Sensor_Name_t *sensorName);

int32_t CMW_CAMERA_Start(uint32_t pipe, uint8_t *pbuff, uint32_t Mode);
int32_t CMW_CAMERA_DoubleBufferStart(uint32_t pipe, uint8_t *pbuff1, uint8_t *pbuff2, uint32_t Mode);
int32_t CMW_CAMERA_Suspend(uint32_t pipe);
int32_t CMW_CAMERA_Resume(uint32_t pipe);

int CMW_CAMERA_SetAntiFlickerMode(int flicker_mode);
int CMW_CAMERA_GetAntiFlickerMode(int *flicker_mode);

int CMW_CAMERA_SetBrightness(int Brightness);
int CMW_CAMERA_GetBrightness(int *Brightness);

int CMW_CAMERA_SetContrast(int Contrast);
int CMW_CAMERA_GetContrast(int *Contrast);

int CMW_CAMERA_SetGain(int32_t Gain);
int CMW_CAMERA_GetGain(int32_t *Gain);

int CMW_CAMERA_SetExposure(int32_t exposure);
int CMW_CAMERA_GetExposure(int32_t *exposure);

int32_t CMW_CAMERA_SetMirrorFlip(int32_t MirrorFlip);
int32_t CMW_CAMERA_GetMirrorFlip(int32_t *MirrorFlip);

int32_t CMW_CAMERA_SetExposureMode(int32_t exposureMode);
int32_t CMW_CAMERA_GetExposureMode(int32_t *exposureMode);

int32_t CMW_CAMERA_SetTestPattern(int32_t mode);
int32_t CMW_CAMERA_GetTestPattern(int32_t *mode);

int32_t CMW_CAMERA_GetSensorInfo(ISP_SensorInfoTypeDef *info);

HAL_StatusTypeDef MX_DCMIPP_ClockConfig(DCMIPP_HandleTypeDef *hdcmipp);

int CMW_CAMERA_PIPE_FrameEventCallback(uint32_t pipe);
int CMW_CAMERA_PIPE_VsyncEventCallback(uint32_t pipe);

#ifdef __cplusplus
}
#endif

#endif /* __MW_CAMERA_H */
