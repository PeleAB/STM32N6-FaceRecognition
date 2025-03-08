/**
 ******************************************************************************
 * @file    isp_services.h
 * @author  AIS Application Team
 * @brief   Header file of ISP middleware services.
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
#ifndef __ISP_SERVICES__H
#define __ISP_SERVICES__H

/* Includes ------------------------------------------------------------------*/
#include "isp_core.h"

/* Exported types ------------------------------------------------------------*/
typedef enum {
  ISP_STAT_LOC_NONE          = 0x00U,
  ISP_STAT_LOC_UP            = 0x01U << 0,
  ISP_STAT_LOC_DOWN          = 0x01U << 1,
  ISP_STAT_LOC_UP_AND_DOWN   = (ISP_STAT_LOC_UP | ISP_STAT_LOC_DOWN),
} ISP_SVC_StatLocation;

typedef enum {
  ISP_STAT_TYPE_NONE         = 0x00U,
  ISP_STAT_TYPE_AVG          = 0x01U << 0,
  ISP_STAT_TYPE_BINS         = 0x01U << 1,
  ISP_STAT_TYPE_AVG_AND_BINS = (ISP_STAT_TYPE_AVG | ISP_STAT_TYPE_BINS),
  ISP_STAT_TYPE_ALL_TMP      = 0x01U << 2, /* special value for IQTuningTool usage */
} ISP_SVC_StatType;

typedef struct {
  ISP_StatisticsTypeDef up;   /* Statistics collected at the up side of the ISP pipeline */
  ISP_StatisticsTypeDef down; /* Statistics collected at the down side of the ISP pipeline */
  uint32_t upFrameIdStart;    /* Frame id of the first frame of the gather cycle at up side */
  uint32_t upFrameIdEnd;      /* Frame id of the last frame of the gather cycle at up side */
  uint32_t downFrameIdStart;  /* Frame id of the first frame of the gather cycle at down side */
  uint32_t downFrameIdEnd;    /* Frame id of the last frame of the gather cycle at down side */
} ISP_SVC_StatStateTypeDef;

typedef ISP_StatusTypeDef (*ISP_stat_ready_cb)(ISP_AlgoTypeDef *pAlgo);

/* Exported constants --------------------------------------------------------*/
/* Use a large precision factor to keep maximum precision on the ColorConv coeff and ISP gain values */
#define ISP_CCM_PRECISION_FACTOR  100000000
#define ISP_GAIN_PRECISION_FACTOR 100000000

/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
/* ISP services */
ISP_StatusTypeDef ISP_SVC_ISP_SetDemosaicing(ISP_HandleTypeDef *hIsp, ISP_DemosaicingTypeDef *pConfig);
ISP_StatusTypeDef ISP_SVC_ISP_SetStatRemoval(ISP_HandleTypeDef *hIsp, ISP_StatRemovalTypeDef *pConfig);
ISP_StatusTypeDef ISP_SVC_ISP_SetDecimation(ISP_HandleTypeDef *hIsp, ISP_DecimationTypeDef *pConfig);
ISP_StatusTypeDef ISP_SVC_ISP_GetDecimation(ISP_HandleTypeDef *hIsp, ISP_DecimationTypeDef *pConfig);
ISP_StatusTypeDef ISP_SVC_ISP_SetContrast(ISP_HandleTypeDef *hIsp, ISP_ContrastTypeDef *pConfig);
ISP_StatusTypeDef ISP_SVC_ISP_SetStatArea(ISP_HandleTypeDef *hIsp, ISP_StatAreaTypeDef *pConfig);
ISP_StatusTypeDef ISP_SVC_ISP_GetStatArea(ISP_HandleTypeDef *hIsp, ISP_StatAreaTypeDef *pConfig);
ISP_StatusTypeDef ISP_SVC_ISP_SetBadPixel(ISP_HandleTypeDef *hIsp, ISP_BadPixelTypeDef *pConfig);
ISP_StatusTypeDef ISP_SVC_ISP_GetBadPixel(ISP_HandleTypeDef *hIsp, ISP_BadPixelTypeDef *pConfig);
ISP_StatusTypeDef ISP_SVC_ISP_SetBlackLevel(ISP_HandleTypeDef *hIsp, ISP_BlackLevelTypeDef *pConfig);
ISP_StatusTypeDef ISP_SVC_ISP_GetBlackLevel(ISP_HandleTypeDef *hIsp, ISP_BlackLevelTypeDef *pConfig);
ISP_StatusTypeDef ISP_SVC_ISP_SetGain(ISP_HandleTypeDef *hIsp, ISP_ISPGainTypeDef *pConfig);
ISP_StatusTypeDef ISP_SVC_ISP_GetGain(ISP_HandleTypeDef *hIsp, ISP_ISPGainTypeDef *pConfig);
ISP_StatusTypeDef ISP_SVC_ISP_SetColorConv(ISP_HandleTypeDef *hIsp, ISP_ColorConvTypeDef *pConfig);
ISP_StatusTypeDef ISP_SVC_ISP_GetColorConv(ISP_HandleTypeDef *hIsp, ISP_ColorConvTypeDef *pConfig);

/* Sensor services */
ISP_StatusTypeDef ISP_SVC_Sensor_GetInfo(ISP_HandleTypeDef *hIsp, ISP_SensorInfoTypeDef *pConfig);
ISP_StatusTypeDef ISP_SVC_Sensor_SetGain(ISP_HandleTypeDef *hIsp, ISP_SensorGainTypeDef *pConfig);
ISP_StatusTypeDef ISP_SVC_Sensor_GetGain(ISP_HandleTypeDef *hIsp, ISP_SensorGainTypeDef *pConfig);
ISP_StatusTypeDef ISP_SVC_Sensor_SetExposure(ISP_HandleTypeDef *hIsp, ISP_SensorExposureTypeDef *pConfig);
ISP_StatusTypeDef ISP_SVC_Sensor_GetExposure(ISP_HandleTypeDef *hIsp, ISP_SensorExposureTypeDef *pConfig);
ISP_StatusTypeDef ISP_SVC_Sensor_SetTestPattern(ISP_HandleTypeDef *hIsp, ISP_SensorTestPatternTypeDef *pConfig);

/* Misc services */
ISP_StatusTypeDef ISP_SVC_Misc_IsDCMIPPReady(ISP_HandleTypeDef *hIsp);
ISP_StatusTypeDef ISP_SVC_Misc_GetDCMIPPVersion(ISP_HandleTypeDef *hIsp, uint32_t *pMajRev, uint32_t *pMinRev);
void ISP_SVC_Misc_IncMainFrameId(ISP_HandleTypeDef *hIsp);
uint32_t ISP_SVC_Misc_GetMainFrameId(ISP_HandleTypeDef *hIsp);
void ISP_SVC_Misc_IncAncillaryFrameId(ISP_HandleTypeDef *hIsp);
uint32_t ISP_SVC_Misc_GetAncillaryFrameId(ISP_HandleTypeDef *hIsp);
void ISP_SVC_Misc_IncDumpFrameId(ISP_HandleTypeDef *hIsp);
uint32_t ISP_SVC_Misc_GetDumpFrameId(ISP_HandleTypeDef *hIsp);
ISP_StatusTypeDef ISP_SVC_Misc_SetWBRefMode(ISP_HandleTypeDef *hIsp, uint32_t RefColorTemp);
ISP_StatusTypeDef ISP_SVC_Misc_GetWBRefMode(ISP_HandleTypeDef *hIsp, uint32_t *pRefColorTemp);
ISP_StatusTypeDef ISP_SVC_Misc_StopPreview(ISP_HandleTypeDef *hIsp);
ISP_StatusTypeDef ISP_SVC_Misc_StartPreview(ISP_HandleTypeDef *hIsp);
bool ISP_SVC_Misc_IsGammaEnabled(ISP_HandleTypeDef *hIsp, uint32_t Pipe);
ISP_StatusTypeDef ISP_SVC_ISP_SetGamma(ISP_HandleTypeDef *hIsp, ISP_GammaTypeDef *pConfig);

/* Dump services */
ISP_StatusTypeDef ISP_SVC_Dump_GetFrame(ISP_HandleTypeDef *hIsp, uint32_t **pBuffer, ISP_DumpCfgTypeDef DumpConfig, ISP_DumpFrameMetaTypeDef *pMeta);

/* IQ param services */
ISP_StatusTypeDef ISP_SVC_IQParam_Init(ISP_HandleTypeDef *hIsp, const ISP_IQParamTypeDef *ISP_IQParamCacheInit);
ISP_IQParamTypeDef *ISP_SVC_IQParam_Get(ISP_HandleTypeDef *hIsp);

/* Statistics services */
void ISP_SVC_Stats_Init(ISP_HandleTypeDef *hIsp);
ISP_StatusTypeDef ISP_SVC_Stats_GetLatest(ISP_HandleTypeDef *hIsp, ISP_SVC_StatStateTypeDef *pStats);
ISP_StatusTypeDef ISP_SVC_Stats_GetNext(ISP_HandleTypeDef *hIsp, ISP_stat_ready_cb callback, ISP_AlgoTypeDef *pAlgo, ISP_SVC_StatStateTypeDef *pStats,
                                        ISP_SVC_StatLocation location, ISP_SVC_StatType type, uint32_t frameDelay);
ISP_StatusTypeDef ISP_SVC_Stats_ProcessCallbacks(ISP_HandleTypeDef *hIsp);
void ISP_SVC_Stats_Gather(ISP_HandleTypeDef *hIsp);

#endif /* __ISP_SERVICES__H */
