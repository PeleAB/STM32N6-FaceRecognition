/**
 ******************************************************************************
 * @file    isp_core.c
 * @author  AIS Application Team
 * @brief   API and CORE parts of the ISP middleware
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

/* Includes ------------------------------------------------------------------*/
#include "isp_core.h"
#include "isp_api.h"
#include "isp_services.h"
#include "isp_algo.h"
#ifdef ISP_MW_TUNING_TOOL_SUPPORT
#include "isp_tool_com.h"
#include "isp_cmd_parser.h"
#endif
#include <math.h>

/* Private types -------------------------------------------------------------*/
/* Private constants ---------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
/**
  * @brief  ISP_Init
  *         Initializes the ISP device
  * @param  hIsp: ISP device handle
  * @param  hDcmipp: HAL DCMIPP device handle
  * @param  CameraInstance: BSP camera instance
  * @param  pAppliHelpers: pointer to application helpers. The application helpers are used
  *         by the ISP to interact with the application. get/set sensor exp/gain/ and get sensor info are mandatory,
  *         while the other helpers are optional and can be set to NULL
  * @param  pStatArea: pointer to statistic area used by the IQ algorithms.
  * @param  ISP_IQParamCacheInit: pointer to ISP sensors specific params.
  *         If NULL, then the statistic area IQ static parameter is used.
  * @retval ISP status
  */
ISP_StatusTypeDef ISP_Init(ISP_HandleTypeDef *hIsp,
                           void *hDcmipp,
                           uint32_t CameraInstance,
                           ISP_AppliHelpersTypeDef *pAppliHelpers,
                           ISP_StatAreaTypeDef *pStatArea,
                           const ISP_IQParamTypeDef *ISP_IQParamCacheInit)
{
  ISP_StatusTypeDef ret;

  if ((hIsp == NULL) || (hDcmipp == NULL) || (pAppliHelpers == NULL))
  {
    return ISP_ERR_EINVAL;
  }

  memset(hIsp, 0, sizeof(*hIsp));

#ifdef ISP_MW_TUNING_TOOL_SUPPORT
  ISP_ToolCom_Init();
#endif

  hIsp->hDcmipp = hDcmipp;
  hIsp->cameraInstance = CameraInstance;
  hIsp->MainPipe_FrameCount = 0;
  hIsp->AncillaryPipe_FrameCount = 0;
  hIsp->DumpPipe_FrameCount = 0;

  hIsp->appliHelpers = *pAppliHelpers;
  /* Appli CB is mandatory for the sensor get/set exp/gain function */
  if (hIsp->appliHelpers.GetSensorInfo == NULL)
  {
     printf("INFO: GetSensorInfo helper function is not implemented\r\n");
     return ISP_ERR_APP_HELPER_UNDEFINED;
   }
  if (hIsp->appliHelpers.SetSensorGain == NULL)
  {
      printf("INFO: SetSensorGain helper function is not implemented\r\n");
      return ISP_ERR_APP_HELPER_UNDEFINED;
  }
  if (hIsp->appliHelpers.GetSensorGain == NULL)
  {
     printf("INFO: GetSensorGain helper function is not implemented\r\n");
     return ISP_ERR_APP_HELPER_UNDEFINED;
   }
  if (hIsp->appliHelpers.SetSensorExposure == NULL)
  {
      printf("INFO: SetSensorExposure helper function is not implemented\r\n");
      return ISP_ERR_APP_HELPER_UNDEFINED;
  }
  if (hIsp->appliHelpers.GetSensorExposure == NULL)
  {
     printf("INFO: GetSensorExposure helper function is not implemented\r\n");
     return ISP_ERR_APP_HELPER_UNDEFINED;
   }

  if (pStatArea != NULL)
  {
    hIsp->statArea = *pStatArea;
  }

  /* Initialize IQ param (read from non volatile memory) */
  ret = ISP_SVC_IQParam_Init(hIsp, ISP_IQParamCacheInit);
  if (ret != ISP_OK)
  {
    return ret;
  }

  /* Set decimation configuration */
  /* Get Sensor Info */
  ret = ISP_SVC_Sensor_GetInfo(hIsp, &hIsp->sensorInfo);
  if (ret != ISP_OK)
  {
    return ret;
  }
  /* Compute the ISP decimation value according to the sensor resolution and the maximum ISP resolution */
  /* It is mandatory to ensure that RAW frame size does not exceed 2688 width prior to demosaicing */
  ISP_DecimationTypeDef decimation;
  if ((hIsp->sensorInfo.width / ISP_DECIM_FACTOR_1) < ISP_RAW_MAX_WIDTH)
  {
    decimation.factor = ISP_DECIM_FACTOR_1;
  }
  else if ((hIsp->sensorInfo.width  / ISP_DECIM_FACTOR_2) < ISP_RAW_MAX_WIDTH)
  {
    decimation.factor = ISP_DECIM_FACTOR_2;
  }
  else if ((hIsp->sensorInfo.width  / ISP_DECIM_FACTOR_4) < ISP_RAW_MAX_WIDTH)
  {
    decimation.factor = ISP_DECIM_FACTOR_4;
  }
  else if ((hIsp->sensorInfo.width  / ISP_DECIM_FACTOR_8) < ISP_RAW_MAX_WIDTH)
  {
    decimation.factor = ISP_DECIM_FACTOR_8;
  }
  else
  {
    printf("ERROR: No possible decimation factor to target ISP RAW width constraint\r\n");
    return ISP_ERR_DECIMATION_EINVAL;
  }
  ret = ISP_SVC_ISP_SetDecimation(hIsp, &decimation);
  if (ret != ISP_OK)
  {
    return ret;
  }

  /* Initialize algorithms */
  ret = ISP_Algo_Init(hIsp);
  if (ret != ISP_OK)
  {
    return ret;
  }

  /* Initialize the statistic engine */
  ISP_SVC_Stats_Init(hIsp);

  return ISP_OK;
}

/**
  * @brief  ISP_DeInit
  *         Deinitializes the ISP device
  * @param  hIsp: ISP device handle
  * @retval ISP status
  */
ISP_StatusTypeDef ISP_DeInit(ISP_HandleTypeDef *hIsp)
{
  ISP_StatusTypeDef ret;

  if (hIsp == NULL)
  {
    return ISP_ERR_EINVAL;
  }

  /* DeInitialize algorithms */
  ret = ISP_Algo_DeInit(hIsp);
  if (ret != ISP_OK)
  {
    return ret;
  }

  /* Re-initialized the hIsp structure */
  memset(hIsp, 0, sizeof(*hIsp));

  return ISP_OK;
}

/**
  * @brief  ISP_Start
  *         Configures the ISP device
  * @param  hIsp: ISP device handle
  * @retval ISP status
  */
ISP_StatusTypeDef ISP_Start(ISP_HandleTypeDef *hIsp)
{
  ISP_IQParamTypeDef *IQParamConfig;
  ISP_StatusTypeDef ret;

  if (hIsp == NULL)
  {
    return ISP_ERR_EINVAL;
  }

  ret = ISP_SVC_Misc_IsDCMIPPReady(hIsp);
  if (ret != ISP_OK)
  {
    return ret;
  }

  /* Get IQ Param config */
  IQParamConfig = ISP_SVC_IQParam_Get(hIsp);

  /* Set static configurations */
  ret = ISP_SVC_ISP_SetDemosaicing(hIsp, &IQParamConfig->demosaicing);
  if (ret != ISP_OK)
  {
    return ret;
  }

  ret = ISP_SVC_ISP_SetStatRemoval(hIsp, &IQParamConfig->statRemoval);
  if (ret != ISP_OK)
  {
    return ret;
  }

  ret = ISP_SVC_ISP_SetContrast(hIsp, &IQParamConfig->contrast);
  if (ret != ISP_OK)
  {
    return ret;
  }

  /* Set optional static configurations */
  if ((IQParamConfig->sensorGainStatic.gain != 0) && (!IQParamConfig->AECAlgo.enable))
  {
    ret = ISP_SVC_Sensor_SetGain(hIsp, &IQParamConfig->sensorGainStatic);
    if (ret != ISP_OK)
    {
      return ret;
    }
  }

  if ((IQParamConfig->sensorExposureStatic.exposure != 0) && (!IQParamConfig->AECAlgo.enable))
  {
    ret = ISP_SVC_Sensor_SetExposure(hIsp, &IQParamConfig->sensorExposureStatic);
    if (ret != ISP_OK)
    {
      return ret;
    }
  }

  if (IQParamConfig->badPixelStatic.enable != 0)
  {
    ret = ISP_SVC_ISP_SetBadPixel(hIsp, &IQParamConfig->badPixelStatic);
    if (ret != ISP_OK)
    {
      return ret;
    }
  }

  if (IQParamConfig->blackLevelStatic.enable != 0)
  {
    ret = ISP_SVC_ISP_SetBlackLevel(hIsp, &IQParamConfig->blackLevelStatic);
    if (ret != ISP_OK)
    {
      return ret;
    }
  }

  if ((IQParamConfig->ispGainStatic.enable != 0) && (!IQParamConfig->AWBAlgo.enable))
  {
    ret = ISP_SVC_ISP_SetGain(hIsp, &IQParamConfig->ispGainStatic);
    if (ret != ISP_OK)
    {
      return ret;
    }
  }

  if ((IQParamConfig->colorConvStatic.enable != 0) && (!IQParamConfig->AWBAlgo.enable))
  {
    ret = ISP_SVC_ISP_SetColorConv(hIsp, &IQParamConfig->colorConvStatic);
    if (ret != ISP_OK)
    {
      return ret;
    }
  }

  /* Configure statistic area (defined by the application or by an optional static configuration) */
  /* Get its config from IQ params if it was not provided by the application at ISP_Init() */
  if ((hIsp->statArea.XSize == 0) || (hIsp->statArea.YSize == 0))
  {
    hIsp->statArea = IQParamConfig->statAreaStatic;
  }
  ret = ISP_SVC_ISP_SetStatArea(hIsp, &hIsp->statArea);
  if (ret != ISP_OK)
  {
    return ret;
  }

  ret = ISP_SVC_ISP_SetGamma(hIsp, &IQParamConfig->gamma);
  if (ret != ISP_OK)
  {
    return ret;
  }

  /* Initialize the exposure target based on the selected exposure compensation */
  IQParamConfig->AECAlgo.exposureTarget = (uint32_t) (ISP_IDEAL_TARGET_EXPOSURE * pow(2, (float)IQParamConfig->AECAlgo.exposureCompensation / 2));

  return ISP_OK;
}

/**
  * @brief  ISP_BackgroundProcess
  *         Run the background process of the ISP device
  * @param  hIsp: ISP device handle
  * @retval ISP status
  */
ISP_StatusTypeDef ISP_BackgroundProcess(ISP_HandleTypeDef *hIsp)
{
  ISP_StatusTypeDef retAlgo, retStats;
#ifdef ISP_MW_TUNING_TOOL_SUPPORT
  ISP_StatusTypeDef retCmdParser = ISP_OK;
  uint8_t *cmd;
#endif

  if (hIsp == NULL)
  {
    return ISP_ERR_EINVAL;
  }

#ifdef ISP_MW_TUNING_TOOL_SUPPORT
  if (ISP_ToolCom_CheckCommandReceived(&cmd) > 0)
  {
    retCmdParser = ISP_CmdParser_ProcessCommand(hIsp, cmd);
  }
#endif

  /* Check if a statistics gathering cycle has been completed to call the statistic callbacks */
  retStats = ISP_SVC_Stats_ProcessCallbacks(hIsp);

  /* Process the algorithms */
  retAlgo = ISP_Algo_Process(hIsp);

#ifdef ISP_MW_TUNING_TOOL_SUPPORT
  if (retCmdParser != ISP_OK)
  {
    return retCmdParser;
  }
#endif
  if (retStats != ISP_OK)
  {
    return retStats;
  }
  else if (retAlgo != ISP_OK)
  {
    return retAlgo;
  }

  return ISP_OK;
}

/**
  * @brief  ISP_SetExposureTarget
  *         Update the exposure target used by the AEC algorithm
  * @param  hIsp: ISP device handle
  * @param  ExposureTarget: Exposure target
  * @retval Operation status
  */
ISP_StatusTypeDef ISP_SetExposureTarget(ISP_HandleTypeDef *hIsp, ISP_ExposureCompTypeDef ExposureCompensation)
{
  ISP_IQParamTypeDef *IQParamConfig;

  IQParamConfig = ISP_SVC_IQParam_Get(hIsp);
  IQParamConfig->AECAlgo.exposureCompensation = ExposureCompensation;
  IQParamConfig->AECAlgo.exposureTarget = (uint32_t) (ISP_IDEAL_TARGET_EXPOSURE * pow(2, (float)ExposureCompensation / 2));

  return ISP_OK;
}

/**
  * @brief  ISP_GetExposureTarget
  *         Get the exposure target used by the AEC algorithm
  * @param  hIsp: ISP device handle
  * @param  pExposureTarget: Pointer to exposure target
  * @retval Operation status
  */
ISP_StatusTypeDef ISP_GetExposureTarget(ISP_HandleTypeDef *hIsp, ISP_ExposureCompTypeDef *pExposureCompensation, uint32_t *pExposureTarget)
{
  ISP_IQParamTypeDef *IQParamConfig;

  IQParamConfig = ISP_SVC_IQParam_Get(hIsp);
  *pExposureTarget = IQParamConfig->AECAlgo.exposureTarget;
  *pExposureCompensation = IQParamConfig->AECAlgo.exposureCompensation;

  return ISP_OK;
}

/**
  * @brief  ISP_SetAECState
  *         Set AEC algorithm state
  * @param  hIsp: ISP device handle
  * @param  enable: AEC algorithm state ('true' if algo is running, 'false' if algo is stopped)
  * @retval Operation status
  */
ISP_StatusTypeDef ISP_SetAECState(ISP_HandleTypeDef *hIsp, uint8_t enable)
{
  ISP_IQParamTypeDef *IQParamConfig;

  IQParamConfig = ISP_SVC_IQParam_Get(hIsp);
  IQParamConfig->AECAlgo.enable = enable;

  return ISP_OK;
}

/**
  * @brief  ISP_GetAECState
  *         Get AEC algorithm state
  * @param  hIsp: ISP device handle
  * @param  pEnable: AEC algorithm pointer to current state ('true' if algo is running, 'false' if algo is stopped)
  * @retval Operation status
  */
ISP_StatusTypeDef ISP_GetAECState(ISP_HandleTypeDef *hIsp, uint8_t *pEnable)
{
  ISP_IQParamTypeDef *IQParamConfig;

  IQParamConfig = ISP_SVC_IQParam_Get(hIsp);
  *pEnable = IQParamConfig->AECAlgo.enable;

  return ISP_OK;
}

/**
  * @brief  ISP_ListWBRefModes
  *         List the reference modes (color temperature) that define a white balance configuration
  * @param  hIsp: ISP device handle
  * @param  RefColorTemp: Array of reference color temperatures
  * @retval Operation status
  */
ISP_StatusTypeDef ISP_ListWBRefModes(ISP_HandleTypeDef *hIsp, uint32_t RefColorTemp[])
{
  ISP_IQParamTypeDef *IQParamConfig;

  IQParamConfig = ISP_SVC_IQParam_Get(hIsp);

  memcpy(RefColorTemp, IQParamConfig->AWBAlgo.referenceColorTemp, sizeof(IQParamConfig->AWBAlgo.referenceColorTemp));

  return ISP_OK;
}

/**
  * @brief  ISP_SetWBRefMode
  *         Set the reference mode that define a white balance configuration.
  *         Can be a manual mode specified by a color temperature, or 'Automatic' relying on the AWB algorithm
  * @param  hIsp: ISP device handle
  * @param  Automatic: 'true' if the AWB automatic mode is requested
  * @param  RefColorTemp: Reference color temperature (manual mode). Not significant if Automatic is set to 'true'
  * @retval Operation status
  */
ISP_StatusTypeDef ISP_SetWBRefMode(ISP_HandleTypeDef *hIsp, uint8_t Automatic, uint32_t RefColorTemp)
{
  ISP_ISPGainTypeDef ISPGain;
  ISP_ColorConvTypeDef ColorConv;
  ISP_IQParamTypeDef *IQParamConfig;
  ISP_StatusTypeDef ret;
  uint32_t index;

  IQParamConfig = ISP_SVC_IQParam_Get(hIsp);

  if (Automatic)
  {
    /* Start the AWB algorithm */
    IQParamConfig->AWBAlgo.enable = 1;
  }
  else
  {
    /* Check the validity of RefColorTemp */
    for (index = 0; index < ISP_AWB_COLORTEMP_REF; index++)
    {
      if (IQParamConfig->AWBAlgo.referenceColorTemp[index] == RefColorTemp)
        break;
    }

    if (index >= ISP_AWB_COLORTEMP_REF)
    {
      /* Unknown reference color temperature */
      return ISP_ERR_WB_COLORTEMP;
    }

    /* Stop the AWB algorithm */
    IQParamConfig->AWBAlgo.enable = 0;

    /* Apply ISP RGB gains and Color Conversion */
    ISPGain.enable = 1;
    ISPGain.ispGainR = IQParamConfig->AWBAlgo.ispGainR[index];
    ISPGain.ispGainG = IQParamConfig->AWBAlgo.ispGainG[index];
    ISPGain.ispGainB = IQParamConfig->AWBAlgo.ispGainB[index];
    ColorConv.enable = 1;
    memcpy(ColorConv.coeff, IQParamConfig->AWBAlgo.coeff[index], sizeof(ColorConv.coeff));

    ret = ISP_SVC_ISP_SetGain(hIsp, &ISPGain);
    if (ret != ISP_OK)
    {
      return ret;
    }

    ret = ISP_SVC_ISP_SetColorConv(hIsp, &ColorConv);
    if (ret != ISP_OK)
    {
      return ret;
    }

    /* Store the selected mode */
    ret = ISP_SVC_Misc_SetWBRefMode(hIsp, RefColorTemp);
    if (ret != ISP_OK)
    {
      return ret;
    }
  }

  return ISP_OK;
}

/**
  * @brief  ISP_GetWBRefMode
  *         Get the reference mode that define a white balance configuration.
  *         Can be a manual mode specified by a color temperature, or 'Automatic' relying on the AWB algorithm
  * @param  hIsp: ISP device handle
  * @param  pAutomatic: Pointer to the mode. 'true' means that AWB automatic mode is running.
  * @param  pRefColorTemp: Pointer to reference color temperature (manual mode). Not significant if pAutomatic is 'true'
  * @retval Operation status
  */
ISP_StatusTypeDef ISP_GetWBRefMode(ISP_HandleTypeDef *hIsp, uint8_t *pAutomatic, uint32_t *pRefColorTemp)
{
  ISP_IQParamTypeDef *IQParamConfig;

  IQParamConfig = ISP_SVC_IQParam_Get(hIsp);
  *pAutomatic = IQParamConfig->AWBAlgo.enable;

  if (!*pAutomatic)
    ISP_SVC_Misc_GetWBRefMode(hIsp, pRefColorTemp);

  return ISP_OK;
}

/**
  * @brief  ISP_GetDecimationFactor
  *         Get the decimation factor applied in the ISP pipeline.
  *         This decimation factor ensures that RAW frame size does not exceed 2688 width prior to demosaicing
  * @param  hIsp: ISP device handle
  * @param  pDecimation: Pointer to the decimation factor
  * @retval Operation status
  */
ISP_StatusTypeDef ISP_GetDecimationFactor(ISP_HandleTypeDef *hIsp, ISP_DecimationTypeDef *pDecimation)
{
  return ISP_SVC_ISP_GetDecimation(hIsp, pDecimation);
}

/**
  * @brief  ISP_SetStatArea
  *         Set statistic area (position and size) defined by the user
  * @param  hIsp: ISP device handle
  * @param  pStatArea: Pointer to the statistic area
  * @retval Operation status
  */
ISP_StatusTypeDef ISP_SetStatArea(ISP_HandleTypeDef *hIsp, ISP_StatAreaTypeDef *pStatArea)
{
  return ISP_SVC_ISP_SetStatArea(hIsp, pStatArea);
}

/**
  * @brief  ISP_GetStatArea
  *         Get current statistic area
  * @param  hIsp: ISP device handle
  * @param  pStatArea: Pointer to the statistic area
  * @retval Operation status
  */
ISP_StatusTypeDef ISP_GetStatArea(ISP_HandleTypeDef *hIsp, ISP_StatAreaTypeDef *pStatArea)
{
  return ISP_SVC_ISP_GetStatArea(hIsp, pStatArea);
}

/**
  * @brief  ISP_GatherStatistics
  *         Gather statistics
  * @param  hIsp: ISP device handle
  * @retval None
  */
void ISP_GatherStatistics(ISP_HandleTypeDef *hIsp)
{
  ISP_SVC_Stats_Gather(hIsp);
}

/**
  * @brief  ISP_IncMainFrameId
  *         Increment the frame ID of the frame output on the main pipe
  * @param  hIsp: ISP device handle
  * @retval None
  */
void ISP_IncMainFrameId(ISP_HandleTypeDef *hIsp)
{
  ISP_SVC_Misc_IncMainFrameId(hIsp);
}

/**
  * @brief  ISP_GetMainFrameId
  *         Return the frame ID of the last frame output on the main pipe
  * @param  hIsp: ISP device handle
  * @retval Id of the last frame output on the main pipe
  */
uint32_t ISP_GetMainFrameId(ISP_HandleTypeDef *hIsp)
{
  return ISP_SVC_Misc_GetMainFrameId(hIsp);
}

/**
  * @brief  ISP_IncAncillaryFrameId
  *         Increment the frame ID of the frame output on the ancillary pipe
  * @param  hIsp: ISP device handle
  * @retval None
  */
void ISP_IncAncillaryFrameId(ISP_HandleTypeDef *hIsp)
{
	ISP_SVC_Misc_IncAncillaryFrameId(hIsp);
}

/**
  * @brief  ISP_GetAncillaryFrameId
  *         Return the frame ID of the last frame output on the ancillary pipe
  * @param  hIsp: ISP device handle
  * @retval Id of the last frame output on the ancillary pipe
  */
uint32_t ISP_GetAncillaryFrameId(ISP_HandleTypeDef *hIsp)
{
  return ISP_SVC_Misc_GetAncillaryFrameId(hIsp);
}

/**
  * @brief  ISP_IncDumpFrameId
  *         Increment the frame ID of the frame output on the dump pipe
  * @param  hIsp: ISP device handle
  * @retval None
  */
void ISP_IncDumpFrameId(ISP_HandleTypeDef *hIsp)
{
  ISP_SVC_Misc_IncDumpFrameId(hIsp);
}

/**
  * @brief  ISP_GetDumpFrameId
  *         Return the frame ID of the last frame output on the dump pipe
  * @param  hIsp: ISP device handle
  * @retval Id of the last frame output on the dump pipe
  */
uint32_t ISP_GetDumpFrameId(ISP_HandleTypeDef *hIsp)
{
  return ISP_SVC_Misc_GetDumpFrameId(hIsp);
}
