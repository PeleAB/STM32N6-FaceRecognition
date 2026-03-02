/**
 ******************************************************************************
 * @file    sysobj_camera.c
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

#include <assert.h>
#include <stdio.h>

#include "sysobj_camera.h"
#include "app/app_config.h"
#include "utils.h"

static int sensor_width;
static int sensor_height;
static int venc_width;
static int venc_height;
static int sensor_mirror_flip = CMW_MIRRORFLIP_NONE;

static const char *sensor_names[] = {
  "CMW_UNKNOWN",
  "CMW_VD66GY",
  "CMW_IMX335",
  "CMW_VD55G1",
};

static void CAM_setSensorInfo(CMW_Sensor_Name_t sensor)
{
  int sensor_name_idx = 0;

  switch (sensor) {
  case CMW_VD66GY_Sensor:
    sensor_width = SENSOR_VD66GY_WIDTH;
    sensor_height = SENSOR_VD66GY_HEIGHT;
    sensor_mirror_flip = SENSOR_VD66GY_FLIP;
    venc_width = VENC_VD66GY_WIDTH;
    venc_height = VENC_VD66GY_HEIGHT;
    sensor_name_idx = 1;
    break;
  case CMW_IMX335_Sensor:
    sensor_width = SENSOR_IMX335_WIDTH;
    sensor_height = SENSOR_IMX335_HEIGHT;
    sensor_mirror_flip = SENSOR_IMX335_FLIP;
    venc_width = VENC_IMX335_WIDTH;
    venc_height = VENC_IMX335_HEIGHT;
    sensor_name_idx = 2;
    break;
  case CMW_VD55G1_Sensor:
    sensor_width = SENSOR_VD55G1_WIDTH;
    sensor_height = SENSOR_VD55G1_HEIGHT;
    sensor_mirror_flip = SENSOR_VD55G1_FLIP;
    venc_width = VENC_VD55G1_WIDTH;
    venc_height = VENC_VD55G1_HEIGHT;
    sensor_name_idx = 3;
    break;
  default:
    assert(0);
  }
  printf("Detected %s\n", sensor_names[sensor_name_idx]);
}

/* Keep display output aspect ratio using crop area */
static void CAM_InitCropConfig(CMW_Manual_roi_area_t *roi, int sensor_w, int sensor_h)
{
  const float ratiox = (float)sensor_w / venc_width;
  const float ratioy = (float)sensor_h / venc_height;
  const float ratio = MIN(ratiox, ratioy);

  assert(ratio >= 1);
  assert(ratio < 64);

  roi->width = (uint32_t) MIN(venc_width * ratio, sensor_w);
  roi->height = (uint32_t) MIN(venc_height * ratio, sensor_h);
  roi->offset_x = (sensor_w - roi->width + 1) / 2;
  roi->offset_y = (sensor_h - roi->height + 1) / 2;
}

static void DCMIPP_PipeInitDisplay(int sensor_w, int sensor_h)
{
  CMW_DCMIPP_Conf_t dcmipp_conf;
  uint32_t hw_pitch;
  int ret;

  assert(venc_width >= venc_height);

  dcmipp_conf.output_width = (uint32_t)venc_width;
  dcmipp_conf.output_height = (uint32_t)venc_height;
  dcmipp_conf.output_format = CAPTURE_FORMAT;
  dcmipp_conf.output_bpp = CAPTURE_BPP;
  dcmipp_conf.mode = CMW_Aspect_ratio_manual_roi;
  dcmipp_conf.enable_swap = 0;
  dcmipp_conf.enable_gamma_conversion = 0;
  CAM_InitCropConfig(&dcmipp_conf.manual_conf, sensor_w, sensor_h);
  ret = CMW_CAMERA_SetPipeConfig(DCMIPP_PIPE1, &dcmipp_conf, &hw_pitch);
  assert(ret == HAL_OK);
  assert(hw_pitch == dcmipp_conf.output_width * dcmipp_conf.output_bpp);
}

static void DCMIPP_PipeInitNn(int sensor_w, int sensor_h)
{
  CMW_DCMIPP_Conf_t dcmipp_conf;
  uint32_t hw_pitch;
  int ret;

  dcmipp_conf.output_width = NN_WIDTH;
  dcmipp_conf.output_height = NN_HEIGHT;
  dcmipp_conf.output_format = NN_FORMAT;
  dcmipp_conf.output_bpp = NN_BPP;
  dcmipp_conf.mode = CMW_Aspect_ratio_manual_roi;
  dcmipp_conf.enable_swap = 1;
  dcmipp_conf.enable_gamma_conversion = 0;
  CAM_InitCropConfig(&dcmipp_conf.manual_conf, sensor_w, sensor_h);
  ret = CMW_CAMERA_SetPipeConfig(DCMIPP_PIPE2, &dcmipp_conf, &hw_pitch);
  assert(ret == HAL_OK);
  assert(hw_pitch == dcmipp_conf.output_width * dcmipp_conf.output_bpp);
}

static void DCMIPP_IpPlugInit(DCMIPP_HandleTypeDef *hdcmipp)
{
  DCMIPP_IPPlugConfTypeDef ipplug_conf = { 0 };
  int ret;

  ipplug_conf.MemoryPageSize = DCMIPP_MEMORY_PAGE_SIZE_256BYTES;

  ipplug_conf.Client = DCMIPP_CLIENT2; /* aux pipe */
  ipplug_conf.Traffic = DCMIPP_TRAFFIC_BURST_SIZE_128BYTES;
  ipplug_conf.MaxOutstandingTransactions = DCMIPP_OUTSTANDING_TRANSACTION_NONE;
  ipplug_conf.DPREGStart = 0;
  ipplug_conf.DPREGEnd = 559; /* (4480 bytes / one line) */
  ipplug_conf.WLRURatio = 15; /* 16 parts of BW */
  ret = HAL_DCMIPP_SetIPPlugConfig(hdcmipp, &ipplug_conf);
  assert(ret == HAL_OK);

  ipplug_conf.Client = DCMIPP_CLIENT5; /* main rgb pipe */
  ipplug_conf.Traffic = DCMIPP_TRAFFIC_BURST_SIZE_128BYTES;
  ipplug_conf.MaxOutstandingTransactions = DCMIPP_OUTSTANDING_TRANSACTION_3;
  ipplug_conf.DPREGStart = 560;
  ipplug_conf.DPREGEnd = 639;
  ipplug_conf.WLRURatio = 0; /* 1 parts of BW */
  ret = HAL_DCMIPP_SetIPPlugConfig(hdcmipp, &ipplug_conf);
  assert(ret == HAL_OK);
}

static void DCMIPP_ReduceSpurious(DCMIPP_HandleTypeDef *hdcmipp)
{
  int ret;

  ret = HAL_DCMIPP_PIPE_EnableLineEvent(hdcmipp, DCMIPP_PIPE1, DCMIPP_MULTILINE_128_LINES);
  assert(ret == HAL_OK);
  ret = HAL_DCMIPP_PIPE_DisableLineEvent(hdcmipp, DCMIPP_PIPE1);
  assert(ret == HAL_OK);
}

void CAM_Init(void)
{
  CMW_CameraInit_t cam_conf;
  CMW_Sensor_Name_t sensor;
  int ret;

  ret = CMW_CAMERA_GetSensorName(&sensor);
  assert(ret == CMW_ERROR_NONE);
  CAM_setSensorInfo(sensor);

  /* Let sensor driver choose which width/height to use */
  cam_conf.width = sensor_width;
  cam_conf.height = sensor_height;
  cam_conf.fps = CAMERA_FPS;
  cam_conf.pixel_format = 0; /* Default; Not implemented yet */
  cam_conf.anti_flicker = 0;
  cam_conf.mirror_flip = sensor_mirror_flip;
  ret = CMW_CAMERA_Init(&cam_conf, NULL);
  assert(ret == CMW_ERROR_NONE);
  sensor_width = cam_conf.width;
  sensor_height = cam_conf.height;

  DCMIPP_IpPlugInit(CMW_CAMERA_GetDCMIPPHandle());
  DCMIPP_PipeInitDisplay(cam_conf.width, cam_conf.height);
  DCMIPP_PipeInitNn(cam_conf.width, cam_conf.height);
  DCMIPP_ReduceSpurious(CMW_CAMERA_GetDCMIPPHandle());
}

void CAM_DisplayPipe_Start(uint8_t *display_pipe_dst, uint32_t cam_mode)
{
  int ret;

  ret = CMW_CAMERA_Start(DCMIPP_PIPE1, display_pipe_dst, cam_mode);
  assert(ret == CMW_ERROR_NONE);
}

void CAM_NNPipe_Start(uint8_t *nn_pipe_dst, uint32_t cam_mode)
{
  int ret;

  ret = CMW_CAMERA_Start(DCMIPP_PIPE2, nn_pipe_dst, cam_mode);
  assert(ret == CMW_ERROR_NONE);
}

static int CAM_SetPipeAddress(uint32_t pipe, uint8_t *dst)
{
  int ret;

  ret = HAL_DCMIPP_PIPE_SetMemoryAddress(CMW_CAMERA_GetDCMIPPHandle(), pipe, DCMIPP_MEMORY_ADDRESS_0,
                                         (uint32_t) dst);

  return (ret == HAL_OK) ? 0 : -1;
}

int CAM_DisplayPipe_UpdateAddress(uint8_t *display_pipe_dst)
{
  return CAM_SetPipeAddress(DCMIPP_PIPE1, display_pipe_dst);
}

int CAM_NNPipe_UpdateAddress(uint8_t *nn_pipe_dst)
{
  return CAM_SetPipeAddress(DCMIPP_PIPE2, nn_pipe_dst);
}

void CAM_IspUpdate(void)
{
  int ret;

  ret = CMW_CAMERA_Run();
  assert(ret == CMW_ERROR_NONE);
}

int CAM_GetVencWidth(void)
{
  assert(venc_width);

  return venc_width;
}

int CAM_GetVencHeight(void)
{
  assert(venc_height);

  return venc_height;
}




