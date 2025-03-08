/**
  ******************************************************************************
  * @file    cmw_imx335.c
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

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "cmw_imx335.h"
#include "cmw_camera.h"
#include "imx335_reg.h"
#include "imx335.h"
#ifndef ISP_MW_TUNING_TOOL_SUPPORT
#include "isp_param_conf.h"
#endif


static int CMW_IMX335_GetResType(uint32_t width, uint32_t height, uint32_t*res)
{
  if (width == 2592 && height == 1944)
  {
    *res = IMX335_R2592_1944;
  }
  else
  {
    return CMW_ERROR_WRONG_PARAM;
  }
  return 0;
}

static int32_t CMW_IMX335_getMirrorFlipConfig(uint32_t Config)
{
  int32_t ret;

  switch (Config)
  {
    case CMW_MIRRORFLIP_NONE:
      ret = IMX335_MIRROR_FLIP_NONE;
      break;
    case CMW_MIRRORFLIP_FLIP:
      ret = IMX335_FLIP;
      break;
    case CMW_MIRRORFLIP_MIRROR:
      ret = IMX335_MIRROR;
      break;
    case CMW_MIRRORFLIP_FLIP_MIRROR:
    default:
      ret = IMX335_MIRROR_FLIP;
      break;
  }

  return ret;
}

static int32_t CMW_IMX335_DeInit(void *io_ctx)
{
  int ret = CMW_ERROR_NONE;
  ret = ISP_DeInit(&((CMW_IMX335_t *)io_ctx)->hIsp);
  if (ret)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  ret = IMX335_DeInit(&((CMW_IMX335_t *)io_ctx)->ctx_driver);
  if (ret)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }
  return ret;
}

static int32_t CMW_IMX335_ReadID(void *io_ctx, uint32_t *Id)
{
  return IMX335_ReadID(&((CMW_IMX335_t *)io_ctx)->ctx_driver, Id);
}

static int32_t CMW_IMX335_SetGain(void *io_ctx, int32_t gain)
{
  return IMX335_SetGain(&((CMW_IMX335_t *)io_ctx)->ctx_driver, gain);
}

static int32_t CMW_IMX335_SetExposure(void *io_ctx, int32_t exposure)
{
  return IMX335_SetExposure(&((CMW_IMX335_t *)io_ctx)->ctx_driver, exposure);
}

static int32_t CMW_IMX335_SetFrequency(void *io_ctx, int32_t frequency)
{
  return IMX335_SetFrequency(&((CMW_IMX335_t *)io_ctx)->ctx_driver, frequency);
}

static int32_t CMW_IMX335_SetFramerate(void *io_ctx, int32_t framerate)
{
  return IMX335_SetFramerate(&((CMW_IMX335_t *)io_ctx)->ctx_driver, framerate);
}

static int32_t CMW_IMX335_SetMirrorFlip(void *io_ctx, uint32_t config)
{
  int32_t mirrorFlip = CMW_IMX335_getMirrorFlipConfig(config);
  return IMX335_MirrorFlipConfig(&((CMW_IMX335_t *)io_ctx)->ctx_driver, mirrorFlip);
}

static int32_t CMW_IMX335_GetSensorInfo(void *io_ctx, ISP_SensorInfoTypeDef *info)
{
  if ((io_ctx ==  NULL) || (info == NULL))
  {
    return CMW_ERROR_WRONG_PARAM;
  }

  if (sizeof(info->name) >= strlen(IMX335_NAME) + 1)
  {
    strcpy(info->name, IMX335_NAME);
  }
  else
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  info->bayer_pattern = IMX335_BAYER_PATTERN;
  info->color_depth = IMX335_COLOR_DEPTH;
  info->width = IMX335_WIDTH;
  info->height = IMX335_HEIGHT;
  info->gain_min = IMX335_GAIN_MIN;
  info->gain_max = IMX335_GAIN_MAX;
  info->exposure_min = IMX335_EXPOSURE_MIN;
  info->exposure_max = IMX335_EXPOSURE_MAX;

  return CMW_ERROR_NONE;
}

static int32_t CMW_IMX335_SetTestPattern(void *io_ctx, int32_t mode)
{
  return IMX335_SetTestPattern(&((CMW_IMX335_t *)io_ctx)->ctx_driver, mode);
}

static int32_t CMW_IMX335_Init(void *io_ctx, CMW_Sensor_Init_t *initSensor)
{
  int ret = CMW_ERROR_NONE;
  uint32_t resolution;

  ret = CMW_IMX335_GetResType(initSensor->width, initSensor->height, &resolution);
  if (ret)
  {
    return CMW_ERROR_WRONG_PARAM;
  }

  ret = CMW_IMX335_SetMirrorFlip(io_ctx, initSensor->mirrorFlip);
  if (ret)
  {
    return CMW_ERROR_WRONG_PARAM;
  }

  ret = IMX335_Init(&((CMW_IMX335_t *)io_ctx)->ctx_driver, resolution, initSensor->pixel_format);
  if (ret != IMX335_OK)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  return CMW_ERROR_NONE;
}

static int32_t CMW_IMX335_Start(void *io_ctx)
{
#ifndef ISP_MW_TUNING_TOOL_SUPPORT
  int ret;
  /* Statistic area is provided with null value so that it force the ISP Library to get the statistic
   * area information from the tuning file.
   */
  ISP_StatAreaTypeDef isp_stat_area = {0};
  (void) ISP_IQParamCacheInit; /* unused */
  ret = ISP_Init(&((CMW_IMX335_t *)io_ctx)->hIsp, ((CMW_IMX335_t *)io_ctx)->hdcmipp, 0, &((CMW_IMX335_t *)io_ctx)->appliHelpers, &isp_stat_area, &ISP_IQParamCacheInit_IMX335);
  if (ret != ISP_OK)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  ret = ISP_Start(&((CMW_IMX335_t *)io_ctx)->hIsp);
  if (ret != ISP_OK)
  {
      return CMW_ERROR_PERIPH_FAILURE;
  }
#endif
  return IMX335_Start(&((CMW_IMX335_t *)io_ctx)->ctx_driver);
}

static int32_t CMW_IMX335_Run(void *io_ctx)
{
#ifndef ISP_MW_TUNING_TOOL_SUPPORT
  int ret;
  ret = ISP_BackgroundProcess(&((CMW_IMX335_t *)io_ctx)->hIsp);
  if (ret != ISP_OK)
  {
      return CMW_ERROR_PERIPH_FAILURE;
  }
#endif
  return CMW_ERROR_NONE;
}

static void CMW_IMX335_PowerOn(CMW_IMX335_t *io_ctx)
{
  io_ctx->ShutdownPin(0);  /* Disable MB1723 2V8 signal  */
  io_ctx->Delay(100);
  io_ctx->EnablePin(0);  /* RESET low (reset active low) */
  io_ctx->Delay(100);
  io_ctx->ShutdownPin(1);  /* Disable MB1723 2V8 signal  */
  io_ctx->Delay(100);
  io_ctx->EnablePin(1);  /* RESET low (reset active low) */
  io_ctx->Delay(100);
}

static void CMW_IMX335_VsyncEventCallback(void *io_ctx, uint32_t pipe)
{
#ifndef ISP_MW_TUNING_TOOL_SUPPORT
  /* Update the ISP frame counter and call its statistics handler */
  switch (pipe)
  {
    case DCMIPP_PIPE0 :
      ISP_IncDumpFrameId(&((CMW_IMX335_t *)io_ctx)->hIsp);
      break;
    case DCMIPP_PIPE1 :
      ISP_IncMainFrameId(&((CMW_IMX335_t *)io_ctx)->hIsp);
      ISP_GatherStatistics(&((CMW_IMX335_t *)io_ctx)->hIsp);
      break;
    case DCMIPP_PIPE2 :
      ISP_IncAncillaryFrameId(&((CMW_IMX335_t *)io_ctx)->hIsp);
      break;
  }
#endif
}

static void CMW_IMX335_FrameEventCallback(void *io_ctx, uint32_t pipe)
{
}

int CMW_IMX335_Probe(CMW_IMX335_t *io_ctx, CMW_Sensor_if_t *imx335_if)
{
  int ret = CMW_ERROR_NONE;
  uint32_t id;
  io_ctx->ctx_driver.IO.Address = io_ctx->Address;
  io_ctx->ctx_driver.IO.Init = io_ctx->Init;
  io_ctx->ctx_driver.IO.DeInit = io_ctx->DeInit;
  io_ctx->ctx_driver.IO.GetTick = io_ctx->GetTick;
  io_ctx->ctx_driver.IO.ReadReg = io_ctx->ReadReg;
  io_ctx->ctx_driver.IO.WriteReg = io_ctx->WriteReg;

  CMW_IMX335_PowerOn(io_ctx);

  ret = IMX335_RegisterBusIO(&io_ctx->ctx_driver, &io_ctx->ctx_driver.IO);
  if (ret != IMX335_OK)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  ret = IMX335_ReadID(&io_ctx->ctx_driver, &id);
  if (ret != IMX335_OK)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }
  if (id != IMX335_CHIP_ID)
  {
      ret = CMW_ERROR_UNKNOWN_COMPONENT;
  }

  memset(imx335_if, 0, sizeof(*imx335_if));
  imx335_if->Init = CMW_IMX335_Init;
  imx335_if->Start = CMW_IMX335_Start;
  imx335_if->DeInit = CMW_IMX335_DeInit;
  imx335_if->Run = CMW_IMX335_Run;
  imx335_if->VsyncEventCallback = CMW_IMX335_VsyncEventCallback;
  imx335_if->FrameEventCallback = CMW_IMX335_FrameEventCallback;
  imx335_if->ReadID = CMW_IMX335_ReadID;
  imx335_if->SetGain = CMW_IMX335_SetGain;
  imx335_if->SetExposure = CMW_IMX335_SetExposure;
  imx335_if->SetFrequency = CMW_IMX335_SetFrequency;
  imx335_if->SetFramerate = CMW_IMX335_SetFramerate;
  imx335_if->SetMirrorFlip = CMW_IMX335_SetMirrorFlip;
  imx335_if->GetSensorInfo = CMW_IMX335_GetSensorInfo;
  imx335_if->SetTestPattern = CMW_IMX335_SetTestPattern;
  return ret;
}
