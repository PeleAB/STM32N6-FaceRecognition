 /**
 ******************************************************************************
 * @file    cmw_utils.h
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

#ifndef CMW_UTILS
#define CMW_UTILS

#include "stm32n6xx_hal_dcmipp.h"
#include "cmw_camera.h"



void CMW_UTILS_GetPipeConfig(uint32_t cam_width, uint32_t cam_height, CMW_DCMIPP_Conf_t *p_conf,
                                    DCMIPP_CropConfTypeDef *crop, DCMIPP_DecimationConfTypeDef *dec,
                                    DCMIPP_DownsizeTypeDef *down);

#endif