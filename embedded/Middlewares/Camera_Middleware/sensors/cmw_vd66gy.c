/**
  ******************************************************************************
  * @file    cmw_vd66gy.c
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

#include "cmw_vd66gy.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "cmw_camera.h"
#ifndef ISP_MW_TUNING_TOOL_SUPPORT
#include "isp_param_conf.h"
#endif

#include "vd6g.h"

#define VD66GY_REG_MODEL_ID                   0x0000

#define container_of(ptr, type, member) (type *) ((unsigned char *)ptr - offsetof(type,member))

#define MIN(a, b)                           ((a) < (b) ?  (a) : (b))
#define MDECIBEL_TO_LINEAR(mdB)             (pow(10.0, (mdB / 1000.0) / 20.0))
#define LINEAR_TO_MDECIBEL(linearValue)     (1000 * (20.0 * log10(linearValue)))
#define FLOAT_TO_FP58(x)                    (((uint16_t)(x) << 8) | ((uint16_t)((x - (uint16_t)(x)) * 256.0f) & 0xFF))
#define FP58_TO_FLOAT(fp)                   (((fp) >> 8) + ((fp) & 0xFF) / 256.0f)

static int CMW_VD66GY_Read8(CMW_VD66GY_t *pObj, uint16_t addr, uint8_t *value)
{
  return pObj->ReadReg(pObj->Address, addr, value, 1);
}

static int CMW_VD66GY_Read16(CMW_VD66GY_t *pObj, uint16_t addr, uint16_t *value)
{
  uint8_t data[2];
  int ret;

  ret = pObj->ReadReg(pObj->Address, addr, data, 2);
  if (ret)
    return ret;

  *value = (data[1] << 8) | data[0];

  return CMW_ERROR_NONE;
}

static int CMW_VD66GY_Read32(CMW_VD66GY_t *pObj, uint16_t addr, uint32_t *value)
{
  uint8_t data[4];
  int ret;

  ret = pObj->ReadReg(pObj->Address, addr, data, 4);
  if (ret)
    return ret;

  *value = (data[3] << 24) | (data[2] << 16) | (data[1] << 8) | data[0];

  return 0;
}

static int CMW_VD66GY_Write8(CMW_VD66GY_t *pObj, uint16_t addr, uint8_t value)
{
  return pObj->WriteReg(pObj->Address, addr, &value, 1);
}

static int CMW_VD66GY_Write16(CMW_VD66GY_t *pObj, uint16_t addr, uint16_t value)
{
  return pObj->WriteReg(pObj->Address, addr, (uint8_t *) &value, 2);
}

static int CMW_VD66GY_Write32(CMW_VD66GY_t *pObj, uint16_t addr, uint32_t value)
{
  return pObj->WriteReg(pObj->Address, addr, (uint8_t *) &value, 4);
}

static void VD6G_ShutdownPin(struct VD6G_Ctx *ctx, int value)
{
  CMW_VD66GY_t *p_ctx = container_of(ctx, CMW_VD66GY_t, ctx_driver);

  p_ctx->ShutdownPin(value);
}

static int VD6G_Read8(struct VD6G_Ctx *ctx, uint16_t addr, uint8_t *value)
{
  CMW_VD66GY_t *p_ctx = container_of(ctx, CMW_VD66GY_t, ctx_driver);

  return CMW_VD66GY_Read8(p_ctx, addr, value);
}

static int VD6G_Read16(struct VD6G_Ctx *ctx, uint16_t addr, uint16_t *value)
{
  CMW_VD66GY_t *p_ctx = container_of(ctx, CMW_VD66GY_t, ctx_driver);

  return CMW_VD66GY_Read16(p_ctx, addr, value);
}

static int VD6G_Read32(struct VD6G_Ctx *ctx, uint16_t addr, uint32_t *value)
{
  CMW_VD66GY_t *p_ctx = container_of(ctx, CMW_VD66GY_t, ctx_driver);

  return CMW_VD66GY_Read32(p_ctx, addr, value);
}

static int VD6G_Write8(struct VD6G_Ctx *ctx, uint16_t addr, uint8_t value)
{
  CMW_VD66GY_t *p_ctx = container_of(ctx, CMW_VD66GY_t, ctx_driver);

  return CMW_VD66GY_Write8(p_ctx, addr, value);
}

static int VD6G_Write16(struct VD6G_Ctx *ctx, uint16_t addr, uint16_t value)
{
  CMW_VD66GY_t *p_ctx = container_of(ctx, CMW_VD66GY_t, ctx_driver);

  return CMW_VD66GY_Write16(p_ctx, addr, value);
}

static int VD6G_Write32(struct VD6G_Ctx *ctx, uint16_t addr, uint32_t value)
{
  CMW_VD66GY_t *p_ctx = container_of(ctx, CMW_VD66GY_t, ctx_driver);

  return CMW_VD66GY_Write32(p_ctx, addr, value);
}

static int VD6G_WriteArray(struct VD6G_Ctx *ctx, uint16_t addr, uint8_t *data, int data_len)
{
  CMW_VD66GY_t *p_ctx = container_of(ctx, CMW_VD66GY_t, ctx_driver);
  const unsigned int chunk_size = 128;
  uint16_t sz;
  int ret;

  while (data_len) {
    sz = MIN(data_len, chunk_size);
    ret = p_ctx->WriteReg(p_ctx->Address, addr, data, sz);
    if (ret)
      return ret;
    data_len -= sz;
    addr += sz;
    data += sz;
  }

  return 0;
}

static void VD6G_Delay(struct VD6G_Ctx *ctx, uint32_t delay_in_ms)
{
  CMW_VD66GY_t *p_ctx = container_of(ctx, CMW_VD66GY_t, ctx_driver);

  p_ctx->Delay(delay_in_ms);
}

static void VD6G_Log(struct VD6G_Ctx *ctx, int lvl, const char *format, va_list ap)
{
#if 0
  const int current_lvl = VD6G_LVL_DBG(0);

  if (lvl > current_lvl)
    return ;

  vprintf(format, ap);
#endif
}

static int CMW_VD66GY_GetResType(uint32_t width, uint32_t height, VD6G_Res_t *res)
{
  if (width == 320 && height == 240)
  {
    *res = VD6G_RES_QVGA_320_240;
  }
  else if (width == 640 && height == 480)
  {
    *res = VD6G_RES_VGA_640_480;
  }
  else if (width == 1024 && height == 768)
  {
    *res = VD6G_RES_XGA_1024_768;
  }
  else if (width == 1120 && height == 720)
  {
    *res = VD6G_RES_PORTRAIT_1120_720;
  }
  else if (width == 1120 && height == 1364)
  {
    *res = VD6G_RES_FULL_1120_1364;
  }
  else
  {
    return CMW_ERROR_WRONG_PARAM;
  }
  return 0;
}

static VD6G_MirrorFlip_t CMW_VD66GY_getMirrorFlipConfig(uint32_t Config)
{
  VD6G_MirrorFlip_t ret;

  switch (Config)
  {
    case CMW_MIRRORFLIP_NONE:
      ret = VD6G_MIRROR_FLIP_NONE;
      break;
    case CMW_MIRRORFLIP_FLIP:
      ret = VD6G_FLIP;
      break;
    case CMW_MIRRORFLIP_MIRROR:
      ret = VD6G_MIRROR;
      break;
    case CMW_MIRRORFLIP_FLIP_MIRROR:
    default:
      ret = VD6G_MIRROR_FLIP;
      break;
  }

  return ret;
}

static int32_t CMW_VD66GY_Init(void *io_ctx, CMW_Sensor_Init_t *initSensor)
{
  VD6G_Config_t config = { 0 };
  int ret;
  int i;

  if (((CMW_VD66GY_t *)io_ctx)->IsInitialized)
  {
    return CMW_ERROR_NONE;
  }


  config.frame_rate = initSensor->fps;
  ret = CMW_VD66GY_GetResType(initSensor->width, initSensor->height, &config.resolution);
  if (ret)
  {
    return CMW_ERROR_WRONG_PARAM;
  }
  config.ext_clock_freq_in_hz = ((CMW_VD66GY_t *)io_ctx)->ClockInHz;
  config.flip_mirror_mode = CMW_VD66GY_getMirrorFlipConfig(initSensor->mirrorFlip);
  config.line_len = 0;
  config.patgen = VD6G_PATGEN_DISABLE;
  config.flicker = VD6G_FLICKER_FREE_NONE;
  config.out_itf.datalane_nb = 2;
  config.out_itf.clock_lane_swap_enable = 1;
  config.out_itf.data_lane0_swap_enable = 1;
  config.out_itf.data_lane1_swap_enable = 1;
  config.out_itf.data_lanes_mapping_swap_enable = 0;
  for (i = 0; i < VD6G_GPIO_NB; i++)
  {
    config.gpio_ctrl[i] = VD6G_GPIO_GPIO_IN;
  }

  ret = VD6G_Init(&((CMW_VD66GY_t *)io_ctx)->ctx_driver, &config);
  if (ret)
  {
    return CMW_ERROR_PERIPH_FAILURE;
  }

  if (((CMW_VD66GY_t *)io_ctx)->ctx_driver.bayer == VD6G_BAYER_NONE)
  {
    VD6G_DeInit(&((CMW_VD66GY_t *)io_ctx)->ctx_driver);
    return CMW_ERROR_PERIPH_FAILURE;
  }

  ((CMW_VD66GY_t *)io_ctx)->IsInitialized = 1;
  return CMW_ERROR_NONE;
}

static int32_t CMW_VD66GY_Start(void *io_ctx)
{
  int ret = CMW_ERROR_NONE;

#ifndef ISP_MW_TUNING_TOOL_SUPPORT
  /* Statistic area is provided with null value so that it force the ISP Library to get the statistic
   * area information from the tuning file.
   */
  ISP_StatAreaTypeDef isp_stat_area = {0};
  (void) ISP_IQParamCacheInit; /* unused */
  ret = ISP_Init(&((CMW_VD66GY_t *)io_ctx)->hIsp, ((CMW_VD66GY_t *)io_ctx)->hdcmipp, 0, &((CMW_VD66GY_t *)io_ctx)->appliHelpers, &isp_stat_area, &ISP_IQParamCacheInit_VD66GY);
  if (ret != ISP_OK)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  ret = ISP_Start(&((CMW_VD66GY_t *)io_ctx)->hIsp);
  if (ret != ISP_OK)
  {
      return CMW_ERROR_PERIPH_FAILURE;
  }
#endif
  ret = VD6G_Start(&((CMW_VD66GY_t *)io_ctx)->ctx_driver);
  if (ret) {
    VD6G_DeInit(&((CMW_VD66GY_t *)io_ctx)->ctx_driver);
    return CMW_ERROR_PERIPH_FAILURE;
  }
  return CMW_ERROR_NONE;
}

static int32_t CMW_VD66GY_Run(void *io_ctx)
{
#ifndef ISP_MW_TUNING_TOOL_SUPPORT
  int ret;
  ret = ISP_BackgroundProcess(&((CMW_VD66GY_t *)io_ctx)->hIsp);
  if (ret != ISP_OK)
  {
      return CMW_ERROR_PERIPH_FAILURE;
  }
#endif
  return CMW_ERROR_NONE;
}

static int32_t CMW_VD66GY_Stop(void *io_ctx)
{
  int ret = CMW_ERROR_NONE;

  ret = VD6G_Stop(&((CMW_VD66GY_t *)io_ctx)->ctx_driver);
  if (ret)
  {
    return CMW_ERROR_PERIPH_FAILURE;
  }
  return CMW_ERROR_NONE;
}

static int32_t CMW_VD66GY_DeInit(void *io_ctx)
{
  int ret = CMW_ERROR_NONE;

  ret = VD6G_Stop(&((CMW_VD66GY_t *)io_ctx)->ctx_driver);
  if (ret)
  {
    return CMW_ERROR_PERIPH_FAILURE;
  }

  ret = VD6G_DeInit(&((CMW_VD66GY_t *)io_ctx)->ctx_driver);
  if (ret)
  {
    return CMW_ERROR_PERIPH_FAILURE;
  }

  ((CMW_VD66GY_t *)io_ctx)->IsInitialized = 0;
  return CMW_ERROR_NONE;
}

/**
  * @brief  Set the gain
  * @param  pObj  pointer to component object
  * @param  Gain Gain in mdB
  * @retval Component status
  */
int32_t CMW_VD66GY_SetGain(void *io_ctx, int32_t gain)
{
  int32_t ret;
  uint8_t again_regmin, again_regmax;
  uint16_t dgain_regmin, dgain_regmax;
  uint32_t again_min_mdB, again_max_mdB;
  uint32_t dgain_min_mdB, dgain_max_mdB;
  double analog_linear_gain, digital_linear_gain;

  ret = VD6G_GetAnalogGainRegRange(&((CMW_VD66GY_t *)io_ctx)->ctx_driver, &again_regmin, &again_regmax);
  if (ret)
    return ret;

  ret = VD6G_GetDigitalGainRegRange(&((CMW_VD66GY_t *)io_ctx)->ctx_driver, &dgain_regmin, &dgain_regmax);
  if (ret)
    return ret;

  again_min_mdB = (uint32_t) LINEAR_TO_MDECIBEL(32 / (32 - again_regmin));
  again_max_mdB = (uint32_t) LINEAR_TO_MDECIBEL(32 / (32 - again_regmax));
  dgain_min_mdB = (uint32_t) LINEAR_TO_MDECIBEL(FP58_TO_FLOAT(dgain_regmin));
  dgain_max_mdB = (uint32_t) LINEAR_TO_MDECIBEL(FP58_TO_FLOAT(dgain_regmax));

  if ((gain < dgain_min_mdB + again_min_mdB)
      || (gain > dgain_max_mdB + again_max_mdB))
    return -1;

  if (gain <= again_max_mdB)
  {
    /* Use analog gain only and set digital gain to its minimum */
    analog_linear_gain = MDECIBEL_TO_LINEAR((double)(gain - dgain_min_mdB));
    digital_linear_gain = MDECIBEL_TO_LINEAR((double)dgain_min_mdB);
  }
  else
  {
    /* For higher gain values, add digital gain */
    analog_linear_gain = MDECIBEL_TO_LINEAR((double)again_max_mdB);
    digital_linear_gain = MDECIBEL_TO_LINEAR((double)(gain - again_max_mdB));
  }

  ret = VD6G_SetAnalogGain(&((CMW_VD66GY_t *)io_ctx)->ctx_driver, (int) (32 - (32 / analog_linear_gain)));
  if (ret)
    return ret;

  ret = VD6G_SetDigitalGain(&((CMW_VD66GY_t *)io_ctx)->ctx_driver, FLOAT_TO_FP58(digital_linear_gain));
  if (ret)
    return ret;

  return 0;
}

/**
  * @brief  Set the exposure
  * @param  pObj  pointer to component object
  * @param  Exposure Exposure in micro seconds
  * @retval Component status
  */
int32_t CMW_VD66GY_SetExposure(void *io_ctx, int32_t exposure)
{
  return VD6G_SetExposureTime(&((CMW_VD66GY_t *)io_ctx)->ctx_driver, exposure);
}

/**
  * @brief  Set the exposure mode
  * @param  pObj  pointer to component object
  * @param  Exposure Exposure mode
  * @retval Component status
  */
int32_t CMW_VD66GY_SetExposureMode(void *io_ctx, int32_t mode)
{
  int ret = -1;

  switch (mode)
  {
    case CMW_EXPOSUREMODE_MANUAL:
      ret = VD6G_SetExposureMode(&((CMW_VD66GY_t *)io_ctx)->ctx_driver, VD6G_EXPOSURE_MANUAL);
      break;
    case CMW_EXPOSUREMODE_AUTOFREEZE:
      ret = VD6G_SetExposureMode(&((CMW_VD66GY_t *)io_ctx)->ctx_driver, VD6G_EXPOSURE_FREEZE_AEALGO);
      break;
    case CMW_EXPOSUREMODE_AUTO:
    default:
      ret = VD6G_SetExposureMode(&((CMW_VD66GY_t *)io_ctx)->ctx_driver, VD6G_EXPOSURE_AUTO);
      break;
  }

  return (ret == 0) ? CMW_ERROR_NONE : CMW_ERROR_UNKNOWN_FAILURE;
}

/**
  * @brief  Get the sensor info
  * @param  pObj  pointer to component object
  * @param  pInfo pointer to sensor info structure
  * @retval Component status
  */
int32_t CMW_VD66GY_GetSensorInfo(void *io_ctx, ISP_SensorInfoTypeDef *info)
{
  uint8_t again_regmin, again_regmax;
  uint16_t dgain_regmin, dgain_regmax;
  uint32_t again_min_mdB, again_max_mdB;
  uint32_t dgain_min_mdB, dgain_max_mdB;

  int ret;

  if ((!io_ctx) || (info == NULL))
    return CMW_ERROR_WRONG_PARAM;

  /* Get sensor name */
  if (sizeof(info->name) >= strlen(VD66GY_NAME) + 1)
  {
    strcpy(info->name, VD66GY_NAME);
  }
  else
  {
    return CMW_ERROR_WRONG_PARAM;
  }

  /* Get isp bayer pattern info */
  info->bayer_pattern = ((CMW_VD66GY_t *)io_ctx)->ctx_driver.bayer - 1;

  /* Get color depth */
  info->color_depth = VD6G_COLOR_DEPTH_RAW8;

  /* Get resolution info */
  info->width = VD6G_MAX_WIDTH;
  info->height = VD6G_MAX_HEIGHT;

  /* Get gain range */
  ret = VD6G_GetAnalogGainRegRange(&((CMW_VD66GY_t *)io_ctx)->ctx_driver, &again_regmin, &again_regmax);
  if (ret)
    return ret;

  ret = VD6G_GetDigitalGainRegRange(&((CMW_VD66GY_t *)io_ctx)->ctx_driver, &dgain_regmin, &dgain_regmax);
  if (ret)
    return ret;

  again_min_mdB = (uint32_t) LINEAR_TO_MDECIBEL(32 / (32 - again_regmin));
  again_max_mdB = (uint32_t) LINEAR_TO_MDECIBEL(32 / (32 - again_regmax));
  dgain_min_mdB = (uint32_t) LINEAR_TO_MDECIBEL(FP58_TO_FLOAT(dgain_regmin));
  dgain_max_mdB = (uint32_t) LINEAR_TO_MDECIBEL(FP58_TO_FLOAT(dgain_regmax));

  info->gain_min = again_min_mdB + dgain_min_mdB;
  info->gain_max = again_max_mdB + dgain_max_mdB;

  /* Get exposure range */
  ret = VD6G_GetExposureRegRange(&((CMW_VD66GY_t *)io_ctx)->ctx_driver, &info->exposure_min, &info->exposure_max);
  if (ret)
    return ret;

  return CMW_ERROR_NONE;
}

static void CMW_VD66GY_VsyncEventCallback(void *io_ctx, uint32_t pipe)
{
#ifndef ISP_MW_TUNING_TOOL_SUPPORT
  /* Update the ISP frame counter and call its statistics handler */
  switch (pipe)
  {
    case DCMIPP_PIPE0 :
      ISP_IncDumpFrameId(&((CMW_VD66GY_t *)io_ctx)->hIsp);
      break;
    case DCMIPP_PIPE1 :
      ISP_IncMainFrameId(&((CMW_VD66GY_t *)io_ctx)->hIsp);
      ISP_GatherStatistics(&((CMW_VD66GY_t *)io_ctx)->hIsp);
      break;
    case DCMIPP_PIPE2 :
      ISP_IncAncillaryFrameId(&((CMW_VD66GY_t *)io_ctx)->hIsp);
      break;
  }
#endif
}

static void CMW_VD66GY_FrameEventCallback(void *io_ctx, uint32_t pipe)
{
}

int32_t VD66GY_RegisterBusIO(CMW_VD66GY_t *io_ctx)
{
  int ret;

  if (!io_ctx)
    return CMW_ERROR_COMPONENT_FAILURE;

  if (!io_ctx->Init)
    return CMW_ERROR_COMPONENT_FAILURE;

  ret = io_ctx->Init();

  return ret;
}

int32_t VD66GY_ReadID(CMW_VD66GY_t *io_ctx, uint32_t *Id)
{
  uint16_t reg16;
  int32_t ret;

  ret = CMW_VD66GY_Read16(io_ctx, VD66GY_REG_MODEL_ID, &reg16);
  if (ret)
    return ret;

  *Id = reg16;

  return CMW_ERROR_NONE;
}

static void CMW_VD66GY_PowerOn(CMW_VD66GY_t *io_ctx)
{
  /* Camera sensor Power-On sequence */
  /* Assert the camera  NRST pins */
  io_ctx->EnablePin(1);
  io_ctx->ShutdownPin(0);  /* Disable MB1723 2V8 signal  */
  HAL_Delay(200);   /* NRST signals asserted during 200ms */
  /* De-assert the camera STANDBY pin (active high) */
  io_ctx->ShutdownPin(1);  /* Disable MB1723 2V8 signal  */
  HAL_Delay(20);     /* NRST de-asserted during 20ms */
}

int CMW_VD66GY_Probe(CMW_VD66GY_t *io_ctx, CMW_Sensor_if_t *vd6g_if)
{
  int ret = CMW_ERROR_NONE;
  uint32_t id;

  io_ctx->ctx_driver.shutdown_pin = VD6G_ShutdownPin;
  io_ctx->ctx_driver.read8 = VD6G_Read8;
  io_ctx->ctx_driver.read16 =VD6G_Read16;
  io_ctx->ctx_driver.read32 = VD6G_Read32;
  io_ctx->ctx_driver.write8 = VD6G_Write8;
  io_ctx->ctx_driver.write16 = VD6G_Write16;
  io_ctx->ctx_driver.write32 = VD6G_Write32;
  io_ctx->ctx_driver.write_array = VD6G_WriteArray;
  io_ctx->ctx_driver.delay = VD6G_Delay;
  io_ctx->ctx_driver.log = VD6G_Log;

  CMW_VD66GY_PowerOn(io_ctx);

  ret = VD66GY_RegisterBusIO(io_ctx);
  if (ret != CMW_ERROR_NONE)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  ret = VD66GY_ReadID(io_ctx, &id);
  if (ret != CMW_ERROR_NONE)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }
  if (id != VD66GY_CHIP_ID)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  memset(vd6g_if, 0, sizeof(*vd6g_if));
  vd6g_if->Init = CMW_VD66GY_Init;
  vd6g_if->DeInit = CMW_VD66GY_DeInit;
  vd6g_if->Run = CMW_VD66GY_Run;
  vd6g_if->VsyncEventCallback = CMW_VD66GY_VsyncEventCallback;
  vd6g_if->FrameEventCallback = CMW_VD66GY_FrameEventCallback;
  vd6g_if->Start = CMW_VD66GY_Start;
  vd6g_if->Stop = CMW_VD66GY_Stop;
  vd6g_if->SetGain = CMW_VD66GY_SetGain;
  vd6g_if->SetExposure = CMW_VD66GY_SetExposure;
  vd6g_if->SetExposureMode = CMW_VD66GY_SetExposureMode;
  vd6g_if->GetSensorInfo = CMW_VD66GY_GetSensorInfo;
  return ret;
}
