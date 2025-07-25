/**
 ******************************************************************************
 * @file    isp_algo.h
 * @author  AIS Application Team
 * @brief   Header file of ISP algorithm
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
#ifndef __ISP_ALGO__H
#define __ISP_ALGO__H

/* Includes ------------------------------------------------------------------*/
#include "isp_conf.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
ISP_StatusTypeDef ISP_Algo_Init(ISP_HandleTypeDef *hIsp);
ISP_StatusTypeDef ISP_Algo_DeInit(ISP_HandleTypeDef *hIsp);
ISP_StatusTypeDef ISP_Algo_Process(ISP_HandleTypeDef *hIsp);

/* Exported variables --------------------------------------------------------*/

#endif /* __ISP_ALGO__H */
