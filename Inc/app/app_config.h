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
#include "sysobj_params.h"

/* ---------------------------------------------------------------------------
 * Persistent parameter store configuration
 * 3 × 4 KB sectors at the very end of the 128 MB NOR flash (MX66UW1G45G).
 *   Bank 0: 0x07FFD000   Bank 1: 0x07FFE000   Bank 2: 0x07FFF000
 * ------------------------------------------------------------------------- */
#define PARAM_FLASH_BASE  0x07FFD000UL
#define PARAM_XSPI_INST   0U

/**
 * @brief Application parameter IDs.
 *
 * Each entry corresponds to one row in s_param_table (defined in
 * app_pipeline.c).  Add new IDs here and a matching row in the table.
 * Maximum: PARAMS_MAX_ENTRIES (15).
 */
typedef enum {
  PARAM_CONF_THRESHOLD = 0, /*!< uint32, default=50,  range [0,  100] – detection confidence % */
  PARAM_BRIGHTNESS     = 1, /*!< uint32, default=80,  range [0,  100] – display brightness %   */
  PARAM_TARGET_FPS     = 2, /*!< uint32, default=30,  range [1,   60] – target frame rate       */
  PARAM_ID_COUNT       = 3,
} app_param_id_t;

#ifndef USE_DCACHE
#define USE_DCACHE
#endif

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

/* ReID tracking configuration */
#define APP_MAX_OBJECT_DETECT             20
#define APP_MAX_OBJECT_TRACKING           10
#define APP_SCORE_THRESHOLD               0.5
#define APP_LOST_TIME_IN_MS               60000
#define APP_START_TRACKING_CONF_THRESHOLD 0.9

#endif

