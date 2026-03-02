/**
 ******************************************************************************
 * @file    isp_param_conf.h
 * @author  AIS Application Team
 * @brief   Header file for IQT parameters of the ISP middleware.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under SLA0044 terms that can be found here:
 * https://www.st.com/resource/en/license_agreement/SLA0044.txt
 *
 ******************************************************************************
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __ISP_PARAM_CONF__H
#define __ISP_PARAM_CONF__H

#include "app_config.h"
#include "cmw_camera.h"

#include "imx335_isp_param_conf.h"
#include "vd66gy_isp_param_conf.h"

static const ISP_IQParamTypeDef* ISP_IQParamCacheInit[] = {
    &ISP_IQParamCacheInit_IMX335,
    &ISP_IQParamCacheInit_VD66GY
};

#endif /* __ISP_PARAM_CONF__H */

