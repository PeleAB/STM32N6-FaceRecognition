/**
 ******************************************************************************
 * @file    app_config.h
 * @author  GPM Application Team
 *
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
#ifndef APP_CONFIG
#define APP_CONFIG

#include "postprocess_conf.h"

#define USE_DCACHE

/* Define sensor info */
#define SENSOR_IMX335_WIDTH 2592
#define SENSOR_IMX335_HEIGHT 1944
#define SENSOR_IMX335_FLIP CMW_MIRRORFLIP_MIRROR

#define SENSOR_VD66GY_WIDTH 1120
#define SENSOR_VD66GY_HEIGHT 720
#define SENSOR_VD66GY_FLIP CMW_MIRRORFLIP_FLIP

#define SENSOR_VD55G1_WIDTH 800
#define SENSOR_VD55G1_HEIGHT 600
#define SENSOR_VD55G1_FLIP CMW_MIRRORFLIP_FLIP

/* Define venc info per sensor */
#define VENC_IMX335_WIDTH 1280
#define VENC_IMX335_HEIGHT 720

#define VENC_VD66GY_WIDTH 1120
#define VENC_VD66GY_HEIGHT 720

#define VENC_VD55G1_WIDTH 640
#define VENC_VD55G1_HEIGHT 480

#define CAMERA_FPS 30

#define CAPTURE_FORMAT DCMIPP_PIXEL_PACKER_FORMAT_ARGB8888
#define CAPTURE_BPP 4

/* Model Related Info */
#define NN_WIDTH 224
#define NN_HEIGHT 224
#define NN_FORMAT DCMIPP_PIXEL_PACKER_FORMAT_RGB888_YUV444_1
#define NN_BPP 3

/* Delay display by CAPTURE_DELAY frame number */
#define CAPTURE_DELAY 1

#endif

