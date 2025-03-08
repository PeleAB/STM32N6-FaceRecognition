/**
 ******************************************************************************
 * @file    isp_api.h
 * @author  AIS Application Team
 * @brief   Header file of ISP middleware API.
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
#ifndef __ISP_API__H
#define __ISP_API__H


/* Includes ------------------------------------------------------------------*/
#include "isp_core.h"

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
ISP_StatusTypeDef ISP_Init(ISP_HandleTypeDef *hIsp, void *hDcmipp, uint32_t CameraInstance, ISP_AppliHelpersTypeDef *pAppliHelpers, ISP_StatAreaTypeDef *pStatArea, const ISP_IQParamTypeDef *ISP_IQParamCacheInit);
ISP_StatusTypeDef ISP_DeInit(ISP_HandleTypeDef *hIsp);
ISP_StatusTypeDef ISP_Start(ISP_HandleTypeDef *hIsp);
ISP_StatusTypeDef ISP_BackgroundProcess(ISP_HandleTypeDef *hIsp);

ISP_StatusTypeDef ISP_SetExposureTarget(ISP_HandleTypeDef *hIsp, ISP_ExposureCompTypeDef ExposureCompensation);
ISP_StatusTypeDef ISP_GetExposureTarget(ISP_HandleTypeDef *hIsp, ISP_ExposureCompTypeDef *pExposureCompensation, uint32_t *pExposureTarget);
ISP_StatusTypeDef ISP_ListWBRefModes(ISP_HandleTypeDef *hIsp, uint32_t RefColorTemp[]);
ISP_StatusTypeDef ISP_SetAECState(ISP_HandleTypeDef *hIsp, uint8_t enable);
ISP_StatusTypeDef ISP_GetAECState(ISP_HandleTypeDef *hIsp, uint8_t *pEnable);
ISP_StatusTypeDef ISP_SetWBRefMode(ISP_HandleTypeDef *hIsp, uint8_t Automatic, uint32_t RefColorTemp);
ISP_StatusTypeDef ISP_GetWBRefMode(ISP_HandleTypeDef *hIsp, uint8_t *pAutomatic, uint32_t *pRefColorTemp);
ISP_StatusTypeDef ISP_GetDecimationFactor(ISP_HandleTypeDef *hIsp, ISP_DecimationTypeDef *pDecimation);
ISP_StatusTypeDef ISP_SetStatArea(ISP_HandleTypeDef *hIsp, ISP_StatAreaTypeDef *pStatArea);
ISP_StatusTypeDef ISP_GetStatArea(ISP_HandleTypeDef *hIsp, ISP_StatAreaTypeDef *pStatArea);

void ISP_GatherStatistics(ISP_HandleTypeDef *hIsp);
void ISP_IncMainFrameId(ISP_HandleTypeDef *hIsp);
uint32_t ISP_GetMainFrameId(ISP_HandleTypeDef *hIsp);
void ISP_IncAncillaryFrameId(ISP_HandleTypeDef *hIsp);
uint32_t ISP_GetAncillaryFrameId(ISP_HandleTypeDef *hIsp);
void ISP_IncDumpFrameId(ISP_HandleTypeDef *hIsp);
uint32_t ISP_GetDumpFrameId(ISP_HandleTypeDef *hIsp);

#endif /* __ISP_API__H */
