/**
 ******************************************************************************
 * @file    isp_cmd_parser.c
 * @author  AIS Application Team
 * @brief   ISP parser for the commands exchanged with the remote IQ tuning tool
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
#ifdef ISP_MW_TUNING_TOOL_SUPPORT

/* Includes ------------------------------------------------------------------*/
#include "isp_core.h"
#include "isp_api.h"
#include "isp_cmd_parser.h"
#include "isp_tool_com.h"
#include "isp_services.h"

/* Private types -------------------------------------------------------------*/
typedef enum {
  ISP_CMD_OP_SET               = 0x00,
  ISP_CMD_OP_GET               = 0x01,
  ISP_CMD_OP_SET_OK            = 0x80,
  ISP_CMD_OP_SET_FAILURE       = 0x81,
  ISP_CMD_OP_GET_OK            = 0x82,
  ISP_CMD_OP_GET_FAILURE       = 0x83,
} ISP_CMD_Operation_TypeDef;

typedef enum {
  ISP_CMD_STATREMOVAL          = 0x00,
  ISP_CMD_DECIMATION           = 0x01,
  ISP_CMD_DEMOSAICING          = 0x02,
  ISP_CMD_CONTRAST             = 0x03,
  ISP_CMD_STATISTICAREA        = 0x04,
  ISP_CMD_SENSORGAIN           = 0x05,
  ISP_CMD_SENSOREXPOSURE       = 0x06,
  ISP_CMD_BADPIXELALGO         = 0x07,
  ISP_CMD_BADPIXELSTATIC       = 0x08,
  ISP_CMD_BLACKLEVELSTATIC     = 0x09,
  ISP_CMD_AECALGO              = 0x0A,
  ISP_CMD_AWBALGO              = 0x0B,
  ISP_CMD_AWBPROFILE           = 0x0C,
  ISP_CMD_ISPGAINSTATIC        = 0x0D,
  ISP_CMD_COLORCONVSTATIC      = 0x0E,
  ISP_CMD_STATISTICUP          = 0x0F,
  ISP_CMD_STATISTICDOWN        = 0x10,
  ISP_CMD_DUMP_PREVIEW_FRAME   = 0x11,
  ISP_CMD_DUMP_ISP_FRAME       = 0x12,
  ISP_CMD_DUMP_RAW_FRAME       = 0x13,
  ISP_CMD_STOPPREVIEW          = 0x14,
  ISP_CMD_STARTPREVIEW         = 0x15,
  ISP_CMD_DCMIPPVERSION        = 0x16,
  ISP_CMD_GAMMA                = 0x17,
  ISP_CMD_SENSORINFO           = 0x18,
  ISP_CMD_SENSORTESTPATTERN    = 0x19,
  /* Application API commands */
  ISP_CMD_USER_EXPOSURETARGET  = 0x80,
  ISP_CMD_USER_LISTWBREFMODES  = 0x81,
  ISP_CMD_USER_WBREFMODE       = 0x82,
  ISP_CMD_USER_GETDECIMATION   = 0x83,
  ISP_CMD_USER_STATISTICAREA   = 0x84,
} ISP_CMD_ID_TypeDef;

typedef struct
{
  uint8_t operation;
  uint8_t id;
  uint8_t dummy[2]; /* Get that structure 32 bits aligned */
} ISP_CMD_HeaderTypeDef;

typedef struct
{
  ISP_CMD_HeaderTypeDef header;
} ISP_CMD_BaseTypeDef;

typedef struct
{
  ISP_CMD_HeaderTypeDef header;
  ISP_StatRemovalTypeDef data;
} ISP_CMD_StatRemovalTypeDef;

typedef struct
{
  ISP_CMD_HeaderTypeDef header;
  ISP_DecimationTypeDef data;
} ISP_CMD_DecimationTypeDef;

typedef struct
{
  ISP_CMD_HeaderTypeDef header;
  ISP_DemosaicingTypeDef data;
} ISP_CMD_DemosaicingTypeDef;

typedef struct
{
  ISP_CMD_HeaderTypeDef header;
  ISP_ContrastTypeDef data;
} ISP_CMD_ContrastTypeDef;

typedef struct
{
  ISP_CMD_HeaderTypeDef header;
  ISP_StatAreaTypeDef data;
} ISP_CMD_StatAreaTypeDef;

typedef struct
{
  ISP_CMD_HeaderTypeDef header;
  ISP_SensorGainTypeDef data;
} ISP_CMD_SensorGainTypeDef;

typedef struct
{
  ISP_CMD_HeaderTypeDef header;
  ISP_SensorExposureTypeDef data;
} ISP_CMD_SensorExposureTypeDef;

typedef struct
{
  ISP_CMD_HeaderTypeDef header;
  ISP_BadPixelAlgoTypeDef data;
} ISP_CMD_BadPixelAlgoTypeDef;

typedef struct
{
  ISP_CMD_HeaderTypeDef header;
  ISP_BadPixelTypeDef data;
} ISP_CMD_BadPixelStaticTypeDef;

typedef struct
{
  ISP_CMD_HeaderTypeDef header;
  ISP_BlackLevelTypeDef data;
} ISP_CMD_BlackLevelStaticTypeDef;

typedef struct
{
  ISP_CMD_HeaderTypeDef header;
  ISP_AECAlgoTypeDef data;
} ISP_CMD_AECAlgoTypeDef;

typedef struct
{
  ISP_CMD_HeaderTypeDef header;
  ISP_AWBAlgoTypeDef data;
} ISP_CMD_AWBAlgoTypeDef;

typedef struct
{
  ISP_CMD_HeaderTypeDef header;
  ISP_AWBProfileTypeDef data;
} ISP_CMD_AWBProfileTypeDef;

typedef struct
{
  ISP_CMD_HeaderTypeDef header;
  ISP_ISPGainTypeDef data;
} ISP_CMD_ISPGainStaticTypeDef;

typedef struct
{
  ISP_CMD_HeaderTypeDef header;
  ISP_ColorConvTypeDef data;
} ISP_CMD_ColorConvStaticTypeDef;

typedef struct
{
  ISP_CMD_HeaderTypeDef header;
  ISP_StatisticsTypeDef data;
} ISP_CMD_StatisticsUpTypeDef;

typedef struct
{
  ISP_CMD_HeaderTypeDef header;
  ISP_StatisticsTypeDef data;
} ISP_CMD_StatisticsDownTypeDef;

typedef struct
{
  ISP_CMD_HeaderTypeDef header;
  ISP_DumpFrameMetaTypeDef data;
} ISP_CMD_DumpFrameMetaTypeDef;

typedef struct
{
  ISP_CMD_HeaderTypeDef header;
  ISP_ExposureCompTypeDef exposureCompensation;
  uint32_t exposureTarget;
} ISP_CMD_ExposureTargetTypeDef;

typedef struct
{
  ISP_CMD_HeaderTypeDef header;
  uint32_t RefColorTemp[ISP_AWB_COLORTEMP_REF];
} ISP_CMD_ListWBRefModesTypeDef;

typedef struct
{
  ISP_CMD_HeaderTypeDef header;
  uint8_t automatic;
  uint32_t refColorTemp;
} ISP_CMD_WBRefModeTypeDef;

typedef struct
{
  ISP_CMD_HeaderTypeDef header;
  ISP_SensorTestPatternTypeDef data;
} ISP_CMD_SensorTestPatternTypeDef;

typedef struct
{
  ISP_CMD_HeaderTypeDef header;
  uint32_t majrev;
  uint32_t minrev;
} ISP_CMD_DCMIPPVersionTypeDef;

typedef struct
{
  ISP_CMD_HeaderTypeDef header;
  ISP_GammaTypeDef data;
} ISP_CMD_GammaTypeDef;

typedef struct
{
  ISP_CMD_HeaderTypeDef header;
  ISP_SensorInfoTypeDef data;
} ISP_CMD_SensorInfoTypeDef;

typedef union {
  ISP_CMD_BaseTypeDef              base;
  ISP_CMD_StatRemovalTypeDef       statRemoval;
  ISP_CMD_DecimationTypeDef        decimation;
  ISP_CMD_DemosaicingTypeDef       demosaicing;
  ISP_CMD_ContrastTypeDef          contrast;
  ISP_CMD_StatAreaTypeDef          statArea;
  ISP_CMD_SensorGainTypeDef        sensorGain;
  ISP_CMD_SensorExposureTypeDef    sensorExposure;
  ISP_CMD_BadPixelAlgoTypeDef      badPixelAlgo;
  ISP_CMD_BadPixelStaticTypeDef    badPixelStatic;
  ISP_CMD_BlackLevelStaticTypeDef  blackLevelStatic;
  ISP_CMD_AECAlgoTypeDef           AECAlgo;
  ISP_CMD_AWBAlgoTypeDef           AWBAlgo;
  ISP_CMD_AWBProfileTypeDef        AWBProfile;
  ISP_CMD_ISPGainStaticTypeDef     ISPGainStatic;
  ISP_CMD_ColorConvStaticTypeDef   colorConvStatic;
  ISP_CMD_StatisticsUpTypeDef      statisticsUp;
  ISP_CMD_StatisticsDownTypeDef    statisticsDown;
  ISP_CMD_DumpFrameMetaTypeDef     dumpFrameMeta;
  ISP_CMD_ExposureTargetTypeDef    exposureTarget;
  ISP_CMD_ListWBRefModesTypeDef    listWBRefModes;
  ISP_CMD_WBRefModeTypeDef         WBRefMode;
  ISP_CMD_DCMIPPVersionTypeDef     DCMIPPVersion;
  ISP_CMD_GammaTypeDef             gamma;
  ISP_CMD_SensorInfoTypeDef        sensorInfo;
  ISP_CMD_SensorTestPatternTypeDef sensorTestPattern;
} ISP_CMD_TypeDef;

/* Private constants ---------------------------------------------------------*/
#define ISP_MAX_DUMP_SIZE         4096
#define ISP_DUMP_DATA_STR         "DUMP DATA"

/* Private macro -------------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static ISP_StatusTypeDef ISP_CmdParser_SetConfig(ISP_HandleTypeDef *hIsp, uint8_t *cmd);
static ISP_StatusTypeDef ISP_CmdParser_GetConfig(ISP_HandleTypeDef *hIsp, uint8_t *cmd);
static void ISP_CmdParser_SendDumpData(uint8_t* pFrame, uint32_t size);
static ISP_StatusTypeDef ISP_CmdParser_StatUpCb(ISP_AlgoTypeDef *pAlgo);
static ISP_StatusTypeDef ISP_CmdParser_StatDownCb(ISP_AlgoTypeDef *pAlgo);

/* Private variables ---------------------------------------------------------*/
static ISP_SVC_StatStateTypeDef ISP_CmdParser_stats;

extern uint32_t current_awb_profId;

/* Private functions ---------------------------------------------------------*/
/**
  * @brief  ISP_CmdParser_ProcessCommand
  *         Parse and process a command received from the remote IQ tuning tool
  * @param  hIsp: ISP device handle
  * @param  cmd: Pointer to a received command
  * @retval operation result
  */
ISP_StatusTypeDef ISP_CmdParser_ProcessCommand(ISP_HandleTypeDef *hIsp, uint8_t *cmd)
{
  ISP_StatusTypeDef ret = ISP_OK;
  ISP_CMD_TypeDef *c = (ISP_CMD_TypeDef *)cmd;

  switch(c->base.header.operation)
  {
  case ISP_CMD_OP_SET:
    ret = ISP_CmdParser_SetConfig(hIsp, cmd);
    break;
  case ISP_CMD_OP_GET:
    ret = ISP_CmdParser_GetConfig(hIsp, cmd);
    break;
  default:
    ret = ISP_ERR_CMDPARSER_OPERATION;
    break;
  }

  return ret;
}

/**
  * @brief  ISP_CmdParser_SetConfig
  *         Parse and process a "Set configuration" command
  * @param  hIsp: ISP device handle
  * @param  cmd: Pointer to a received command
  * @retval operation result
  */
static ISP_StatusTypeDef ISP_CmdParser_SetConfig(ISP_HandleTypeDef *hIsp, uint8_t *cmd)
{
  ISP_StatusTypeDef ret = ISP_OK;
  ISP_CMD_TypeDef base_cmd, c;
  ISP_IQParamTypeDef *IQParamConfig;
  uint8_t cmd_id;

  IQParamConfig = ISP_SVC_IQParam_Get(hIsp);

  /* Warning: the command received from the remote may be NOT 32bit memory-aligned.
   * As a consequence, direct structure copy is forbidden (hard fault error).
   * Copy the command in the "c" struct which is memory-aligned.
   */
  memcpy(&c, cmd, sizeof(c));

  cmd_id = c.base.header.id;
  switch(cmd_id)
  {
  case ISP_CMD_STATREMOVAL:
    /* Update both ISP and IQ params */
    ret = ISP_SVC_ISP_SetStatRemoval(hIsp, &c.statRemoval.data);
    if (ret == ISP_OK)
    {
      IQParamConfig->statRemoval.enable = c.statRemoval.data.enable;
      if (IQParamConfig->statRemoval.enable)
      {
        IQParamConfig->statRemoval = c.statRemoval.data;
      }
    }
    break;

  case ISP_CMD_DEMOSAICING:
    /* Update both ISP and IQ params */
    ret = ISP_SVC_ISP_SetDemosaicing(hIsp, &c.demosaicing.data);
    if (ret == ISP_OK)
    {
      IQParamConfig->demosaicing.enable = c.demosaicing.data.enable;
      if (IQParamConfig->demosaicing.enable)
      {
        IQParamConfig->demosaicing = c.demosaicing.data;
      }
    }
    break;

  case ISP_CMD_CONTRAST:
    /* Update both ISP and IQ params */
    ret = ISP_SVC_ISP_SetContrast(hIsp, &c.contrast.data);
    if (ret == ISP_OK)
    {
      IQParamConfig->contrast.enable = c.contrast.data.enable;
      if (IQParamConfig->contrast.enable)
      {
        IQParamConfig->contrast = c.contrast.data;
      }
    }
    break;

  case ISP_CMD_STATISTICAREA:
    /* Update both ISP and IQ params */
    ret = ISP_SVC_ISP_SetStatArea(hIsp, &c.statArea.data);
    if (ret == ISP_OK)
    {
      IQParamConfig->statAreaStatic = c.statArea.data;
      hIsp->statArea = c.statArea.data;
    }
    break;

  case ISP_CMD_SENSORGAIN:
    /* Update both ISP and IQ params */
    ret = ISP_SVC_Sensor_SetGain(hIsp, &c.sensorGain.data);
    if (ret == ISP_OK)
    {
      IQParamConfig->sensorGainStatic = c.sensorGain.data;
    }
    break;

  case ISP_CMD_SENSOREXPOSURE:
    /* Update both ISP and IQ params */
    ret = ISP_SVC_Sensor_SetExposure(hIsp, &c.sensorExposure.data);
    if (ret == ISP_OK)
    {
      IQParamConfig->sensorExposureStatic = c.sensorExposure.data;
    }
    break;

  case ISP_CMD_SENSORTESTPATTERN:
    ret = ISP_SVC_Sensor_SetTestPattern(hIsp, &c.sensorTestPattern.data);
    break;

  case ISP_CMD_BADPIXELALGO:
    /* Update only IQ params, the algo will consider this update at its next process call */
    IQParamConfig->badPixelAlgo.enable = c.badPixelAlgo.data.enable;
    if (IQParamConfig->badPixelAlgo.enable)
    {
      IQParamConfig->badPixelAlgo = c.badPixelAlgo.data;
    }
    break;

  case ISP_CMD_BADPIXELSTATIC:
    /* Update both ISP and IQ params */
    ret = ISP_SVC_ISP_SetBadPixel(hIsp, &c.badPixelStatic.data);
    if (ret == ISP_OK)
    {
      IQParamConfig->badPixelStatic.enable = c.badPixelStatic.data.enable;
      if (IQParamConfig->badPixelStatic.enable)
      {
        IQParamConfig->badPixelStatic = c.badPixelStatic.data;
      }
    }
    break;

  case ISP_CMD_BLACKLEVELSTATIC:
    /* Update both ISP and IQ params */
    ret = ISP_SVC_ISP_SetBlackLevel(hIsp, &c.blackLevelStatic.data);
    if (ret == ISP_OK)
    {
      IQParamConfig->blackLevelStatic.enable = c.blackLevelStatic.data.enable;
      if (IQParamConfig->blackLevelStatic.enable)
      {
        IQParamConfig->blackLevelStatic = c.blackLevelStatic.data;
      }
    }
    break;

  case ISP_CMD_AECALGO:
    /* Update IQ params */
    IQParamConfig->AECAlgo.enable = c.AECAlgo.data.enable;
    if (IQParamConfig->AECAlgo.enable)
    {
      IQParamConfig->AECAlgo = c.AECAlgo.data;
    }
    /* Call the application API to set the exposureTarget based on the exposureCompensation
     * so that the algo will consider this update at its next process call
     */
    ret = ISP_SetExposureTarget(hIsp, c.AECAlgo.data.exposureCompensation);
    break;

  case ISP_CMD_AWBALGO:
    /* Update only IQ params, the algo will consider this update at its next process call */
    IQParamConfig->AWBAlgo.enable = c.AWBAlgo.data.enable;
    IQParamConfig->AWBAlgo = c.AWBAlgo.data;
    if (IQParamConfig->AWBAlgo.enable)
    {
      IQParamConfig->AWBAlgo.enable = ISP_AWB_ENABLE_RECONFIGURE;
    }
    break;

  case ISP_CMD_ISPGAINSTATIC:
    /* Update both ISP and IQ params */
    ret = ISP_SVC_ISP_SetGain(hIsp, &c.ISPGainStatic.data);
    if (ret == ISP_OK)
    {
      IQParamConfig->ispGainStatic.enable = c.ISPGainStatic.data.enable;
      if (IQParamConfig->ispGainStatic.enable)
      {
        IQParamConfig->ispGainStatic = c.ISPGainStatic.data;
      }
    }
    break;

  case ISP_CMD_COLORCONVSTATIC:
    /* Update both ISP and IQ params */
    ret = ISP_SVC_ISP_SetColorConv(hIsp, &c.colorConvStatic.data);
    if (ret == ISP_OK)
    {
      IQParamConfig->colorConvStatic.enable = c.colorConvStatic.data.enable;
      if (IQParamConfig->colorConvStatic.enable)
      {
        IQParamConfig->colorConvStatic = c.colorConvStatic.data;
      }
    }
    break;

  case ISP_CMD_USER_EXPOSURETARGET:
    /* Call the application API */
    ret = ISP_SetExposureTarget(hIsp, c.exposureTarget.exposureCompensation);
    break;

  case ISP_CMD_USER_WBREFMODE:
    /* Call the application API */
    ret = ISP_SetWBRefMode(hIsp, c.WBRefMode.automatic, c.WBRefMode.refColorTemp);
    break;

  case ISP_CMD_USER_STATISTICAREA:
    /* Update both ISP and IQ params */
    ret = ISP_SetStatArea(hIsp, &c.statArea.data);
    if (ret == ISP_OK)
    {
      IQParamConfig->statAreaStatic = c.statArea.data;
      hIsp->statArea = c.statArea.data;
    }
    break;

  case ISP_CMD_STOPPREVIEW:
    ret = ISP_SVC_Misc_StopPreview(hIsp);
    break;

  case ISP_CMD_STARTPREVIEW:
    ret = ISP_SVC_Misc_StartPreview(hIsp);
    break;

  case ISP_CMD_GAMMA:
    /* Update both ISP and IQ params */
    ret = ISP_SVC_ISP_SetGamma(hIsp, &c.gamma.data);
    if (ret == ISP_OK)
    {
      IQParamConfig->gamma = c.gamma.data;
    }
    break;

  default:
    ret = ISP_ERR_CMDPARSER_COMMAND;
  }

  /* Free the received message just before sending the answer message */
  ISP_ToolCom_PrepareNextCommand();

  base_cmd.base.header.id = cmd_id;
  if (ret == ISP_OK)
  {
    base_cmd.base.header.operation = ISP_CMD_OP_SET_OK;
  }
  else
  {
    base_cmd.base.header.operation = ISP_CMD_OP_SET_FAILURE;
    base_cmd.base.header.dummy[0] = ret;
  }

  ISP_ToolCom_SendData((uint8_t*)&base_cmd, sizeof(base_cmd), NULL, NULL);

  return ret;
}

/**
  * @brief  ISP_CmdParser_GetConfig
  *         Parse and process a "Get configuration" command
  * @param  hIsp: ISP device handle
  * @param  cmd: Pointer to a received command
  * @retval operation result
  */
static ISP_StatusTypeDef ISP_CmdParser_GetConfig(ISP_HandleTypeDef *hIsp, uint8_t *cmd)
{
  ISP_StatusTypeDef ret = ISP_OK;
  ISP_IQParamTypeDef *IQParamConfig;
  ISP_CMD_TypeDef c = { 0 };
  uint8_t cmd_id;
  uint32_t *pFrame = NULL;

  IQParamConfig = ISP_SVC_IQParam_Get(hIsp);

  cmd_id = ((ISP_CMD_TypeDef *)cmd)->base.header.id;
  switch(cmd_id)
  {
  case ISP_CMD_STATREMOVAL:
    c.statRemoval.data = IQParamConfig->statRemoval;
    break;

  case ISP_CMD_DECIMATION:
    /* Get actual value from ISP, which may be defined at the ISP init according to the sensor resolution */
    ret = ISP_SVC_ISP_GetDecimation(hIsp, &c.decimation.data);
    break;

  case ISP_CMD_DEMOSAICING:
    c.demosaicing.data = IQParamConfig->demosaicing;
    break;

  case ISP_CMD_CONTRAST:
    c.contrast.data = IQParamConfig->contrast;
    break;

  case ISP_CMD_STATISTICAREA:
    /* Get actual value from ISP, which may be defined by a static configuration or by the running application */
    ret = ISP_SVC_ISP_GetStatArea(hIsp, &c.statArea.data);
    break;

  case ISP_CMD_SENSORGAIN:
    /* Get actual value from sensor, which may be defined by a static configuration or updated by an algorithm */
    ret = ISP_SVC_Sensor_GetGain(hIsp, &c.sensorGain.data);
    break;

  case ISP_CMD_SENSOREXPOSURE:
    /* Get actual value from sensor, which may be defined by a static configuration or updated by an algorithm */
    ret = ISP_SVC_Sensor_GetExposure(hIsp, &c.sensorExposure.data);
    break;

  case ISP_CMD_BADPIXELALGO:
    c.badPixelAlgo.data = IQParamConfig->badPixelAlgo;
    break;

  case ISP_CMD_BADPIXELSTATIC:
    /* Get actual value from ISP, which may be defined by a static configuration or updated by an algorithm */
    ret = ISP_SVC_ISP_GetBadPixel(hIsp, &c.badPixelStatic.data);
    break;

  case ISP_CMD_BLACKLEVELSTATIC:
    /* Get actual value from ISP, which may be defined by a static configuration or updated by an algorithm */
    ret = ISP_SVC_ISP_GetBlackLevel(hIsp, &c.blackLevelStatic.data);
    break;

  case ISP_CMD_AECALGO:
    c.AECAlgo.data = IQParamConfig->AECAlgo;
    break;

  case ISP_CMD_AWBALGO:
    c.AWBAlgo.data = IQParamConfig->AWBAlgo;
    break;

  case ISP_CMD_AWBPROFILE:
    strcpy(c.AWBProfile.data.id, IQParamConfig->AWBAlgo.id[current_awb_profId]);
    c.AWBProfile.data.referenceColorTemp = IQParamConfig->AWBAlgo.referenceColorTemp[current_awb_profId];
    break;

  case ISP_CMD_ISPGAINSTATIC:
    /* Get actual value from ISP, which may be defined by a static configuration or updated by an algorithm */
    ret = ISP_SVC_ISP_GetGain(hIsp, &c.ISPGainStatic.data);
    break;

  case ISP_CMD_COLORCONVSTATIC:
    /* Get actual value from ISP, which may be defined by a static configuration or updated by an algorithm */
    ret = ISP_SVC_ISP_GetColorConv(hIsp, &c.colorConvStatic.data);
    break;

  case ISP_CMD_STATISTICUP:
    ret = ISP_SVC_Stats_GetNext(hIsp, &ISP_CmdParser_StatUpCb, NULL, &ISP_CmdParser_stats, ISP_STAT_LOC_UP, ISP_STAT_TYPE_ALL_TMP, 2);
    break;

  case ISP_CMD_STATISTICDOWN:
    ret = ISP_SVC_Stats_GetNext(hIsp, &ISP_CmdParser_StatDownCb, NULL, &ISP_CmdParser_stats, ISP_STAT_LOC_DOWN, ISP_STAT_TYPE_ALL_TMP, 2);
    break;

  case ISP_CMD_DUMP_PREVIEW_FRAME:
    ret = ISP_SVC_Dump_GetFrame(hIsp, &pFrame, ISP_DUMP_CFG_DEFAULT, &c.dumpFrameMeta.data);
    break;

  case ISP_CMD_DUMP_ISP_FRAME:
    ret = ISP_SVC_Dump_GetFrame(hIsp, &pFrame, ISP_DUMP_CFG_FULLSIZE_RGB888, &c.dumpFrameMeta.data);
    break;

  case ISP_CMD_DUMP_RAW_FRAME:
    ret = ISP_SVC_Dump_GetFrame(hIsp, &pFrame, ISP_DUMP_CFG_DUMP_PIPE_SENSOR, &c.dumpFrameMeta.data);
    break;

  case ISP_CMD_DCMIPPVERSION:
    ret = ISP_SVC_Misc_GetDCMIPPVersion(hIsp, &c.DCMIPPVersion.majrev, &c.DCMIPPVersion.minrev);
    break;

  case ISP_CMD_USER_EXPOSURETARGET:
    /* Call the application API */
    ret = ISP_GetExposureTarget(hIsp, &c.exposureTarget.exposureCompensation, &c.exposureTarget.exposureTarget);
    break;

  case ISP_CMD_USER_LISTWBREFMODES:
    /* Call the application API */
    ret = ISP_ListWBRefModes(hIsp, c.listWBRefModes.RefColorTemp);
    break;

  case ISP_CMD_USER_WBREFMODE:
    /* Call the application API */
    ret = ISP_GetWBRefMode(hIsp, &c.WBRefMode.automatic, &c.WBRefMode.refColorTemp);
    break;

  case ISP_CMD_USER_GETDECIMATION:
    /* Call the application API */
    ret = ISP_GetDecimationFactor(hIsp, &c.decimation.data);
    break;

  case ISP_CMD_USER_STATISTICAREA:
    /* Get actual value from ISP, which may be defined by a static configuration or by the running application */
    ret = ISP_GetStatArea(hIsp, &c.statArea.data);
    break;

  case ISP_CMD_GAMMA:
    c.gamma.data = IQParamConfig->gamma;
    break;

  case ISP_CMD_SENSORINFO:
    c.sensorInfo.data = hIsp->sensorInfo;
    break;

  default:
    c.base.header.operation = ISP_CMD_OP_GET_FAILURE;
    ret = ISP_ERR_CMDPARSER_COMMAND;
  }

  c.base.header.id = cmd_id;
  if (ret == ISP_OK)
  {
    c.base.header.operation = ISP_CMD_OP_GET_OK;
  }
  else
  {
    c.base.header.operation = ISP_CMD_OP_GET_FAILURE;
    c.base.header.dummy[0] = ret;
  }

  /* Free the received message just before sending the answer message */
  ISP_ToolCom_PrepareNextCommand();

  if (!((cmd_id == ISP_CMD_STATISTICUP || cmd_id == ISP_CMD_STATISTICDOWN) && (ret == ISP_OK)))
  {
    /* Send command answer (except for statistic where the answer is sent upon callback call */
    ISP_ToolCom_SendData((uint8_t*)&c, sizeof(c), NULL, NULL);
  }

  /* Send dump buffer if requested  */
  if (((cmd_id == ISP_CMD_DUMP_PREVIEW_FRAME) || (cmd_id == ISP_CMD_DUMP_ISP_FRAME) || (cmd_id == ISP_CMD_DUMP_RAW_FRAME)) && (ret == ISP_OK))
  {
    ISP_CmdParser_SendDumpData((uint8_t*)pFrame, c.dumpFrameMeta.data.size);
  }

  return ret;
}

/**
  * @brief  ISP_CmdParser_SendDumpData
  *         Send dump frame data, splitting it in several parts if too large
  * @param  pFrame: Pointer to data to send
  * @param  size: Size of data to send
  * @retval None
  */
static void ISP_CmdParser_SendDumpData(uint8_t* pFrame, uint32_t size)
{
  uint32_t sizeToSend, remaining = size;
  uint8_t first = true;
  char dump_start_msg[32];
  char dump_stop_msg[32];

  if (size > ISP_MAX_DUMP_SIZE) {
    /* Split the data in several parts */
    do {
      if (first)
      {
        sprintf(dump_start_msg, "%s[", ISP_DUMP_DATA_STR);
        sizeToSend = ISP_MAX_DUMP_SIZE;
        first = false;
        ISP_ToolCom_SendData((uint8_t*)pFrame, sizeToSend, dump_start_msg, NULL);
      }
      else if (remaining <= ISP_MAX_DUMP_SIZE)
      {
        sprintf(dump_stop_msg, "%s]", ISP_DUMP_DATA_STR);
        sizeToSend = remaining;
        ISP_ToolCom_SendData((uint8_t*)pFrame, sizeToSend, NULL, dump_stop_msg);
      }
      else
      {
        sizeToSend = ISP_MAX_DUMP_SIZE;
        ISP_ToolCom_SendData((uint8_t*)pFrame, sizeToSend, NULL, NULL);
      }

      pFrame += sizeToSend;
      remaining -= sizeToSend;
    } while (remaining > 0);
  }
  else
  {
    /* Send all the data in one single part */
    sprintf(dump_start_msg, "%s[", ISP_DUMP_DATA_STR);
    sprintf(dump_stop_msg, "%s]", ISP_DUMP_DATA_STR);
    ISP_ToolCom_SendData((uint8_t*)pFrame, size, dump_start_msg, dump_stop_msg);
  }
}

/**
  * @brief  ISP_CmdParser_StatUpCb
  *         Callback called when statistics at Up are available
  * @param  pAlgo: Pointer to algo (not applicable here)
  * @retval operation result
  */
static ISP_StatusTypeDef ISP_CmdParser_StatUpCb(ISP_AlgoTypeDef *pAlgo)
{
  ISP_CMD_TypeDef cmd = { 0 };

  /* Send the answer command */
  cmd.base.header.id = ISP_CMD_STATISTICUP;
  cmd.base.header.operation = ISP_CMD_OP_GET_OK;
  cmd.statisticsUp.data = ISP_CmdParser_stats.up;

  ISP_ToolCom_SendData((uint8_t*)&cmd, sizeof(cmd), NULL, NULL);

  return ISP_OK;
}

/**
  * @brief  ISP_CmdParser_StatDownCb
  *         Callback called when statistics at Down are available
  * @param  pAlgo: Pointer to algo (not applicable here)
  * @retval operation result
  */
static ISP_StatusTypeDef ISP_CmdParser_StatDownCb(ISP_AlgoTypeDef *pAlgo)
{
  ISP_CMD_TypeDef cmd = { 0 };

  /* Send the answer command */
  cmd.base.header.id = ISP_CMD_STATISTICDOWN;
  cmd.base.header.operation = ISP_CMD_OP_GET_OK;
  cmd.statisticsUp.data = ISP_CmdParser_stats.down;

  ISP_ToolCom_SendData((uint8_t*)&cmd, sizeof(cmd), NULL, NULL);

  return ISP_OK;
}

#endif /* ISP_MW_TUNING_TOOL_SUPPORT */
