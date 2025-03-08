/**
 ******************************************************************************
 * @file    isp_core.h
 * @author  AIS Application Team
 * @brief   Header file of ISP middleware CORE.
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
#ifndef __ISP_CORE__H
#define __ISP_CORE__H

/* Includes ------------------------------------------------------------------*/
#include "isp_conf.h"
#include "isp_platform.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/
/* ISP operation status */
typedef enum
{
  ISP_OK                        =   0,
  ISP_ERR_EINVAL                =   1,
  ISP_ERR_DEMOSAICING_EINVAL    =  10,
  ISP_ERR_DEMOSAICING_HAL       =  11,
  ISP_ERR_STATREMOVAL_EINVAL    =  20,
  ISP_ERR_STATREMOVAL_HAL       =  21,
  ISP_ERR_DECIMATION_EINVAL     =  30,
  ISP_ERR_DECIMATION_HAL        =  31,
  ISP_ERR_CONTRAST_EINVAL       =  40,
  ISP_ERR_CONTRAST_HAL          =  41,
  ISP_ERR_STATAREA_EINVAL       =  50,
  ISP_ERR_STATAREA_HAL          =  51,
  ISP_ERR_BADPIXEL_EINVAL       =  60,
  ISP_ERR_BADPIXEL_HAL          =  61,
  ISP_ERR_BLACKLEVEL_EINVAL     =  70,
  ISP_ERR_BLACKLEVEL_HAL        =  71,
  ISP_ERR_ISPGAIN_EINVAL        =  80,
  ISP_ERR_ISPGAIN_HAL           =  81,
  ISP_ERR_COLORCONV_EINVAL      =  90,
  ISP_ERR_COLORCONV_HAL         =  91,
  ISP_ERR_IQPARAM_EINVAL        = 100,
  ISP_ERR_IQPARAM_HAL           = 101,
  ISP_ERR_IQPARAM_MISSING       = 102,
  ISP_ERR_SENSORINFO_EINVAL     = 110,
  ISP_ERR_SENSORINFO            = 111,
  ISP_ERR_SENSORGAIN_EINVAL     = 120,
  ISP_ERR_SENSORGAIN            = 121,
  ISP_ERR_SENSOREXPOSURE_EINVAL = 130,
  ISP_ERR_SENSOREXPOSURE        = 131,
  ISP_ERR_DCMIPP_STATE          = 140,
  ISP_ERR_DCMIPP_STOP           = 141,
  ISP_ERR_DCMIPP_START          = 142,
  ISP_ERR_DCMIPP_DOWNSIZE       = 143,
  ISP_ERR_DCMIPP_CONFIGPIPE     = 144,
  ISP_ERR_DCMIPP_DUMPTIMEOUT    = 145,
  ISP_ERR_DCMIPP_PIXELFORMAT    = 146,
  ISP_ERR_DCMIPP_FRAMESIZE      = 147,
  ISP_ERR_DCMIPP_NOMEM          = 148,
  ISP_ERR_DCMIPP_GAMMA          = 149,
  ISP_ERR_WB_COLORTEMP          = 150,
  ISP_ERR_CMDPARSER_COMMAND     = 160,
  ISP_ERR_CMDPARSER_OPERATION   = 161,
  ISP_ERR_STAT_EINVAL           = 170,
  ISP_ERR_STAT_HAL              = 171,
  ISP_ERR_STAT_MAXCLIENTS       = 172,
  ISP_ERR_APP_HELPER_UNDEFINED  = 180,
  ISP_ERR_ALGO                  = 190,
  ISP_ERR_SENSORTESTPATTERN     = 200,
} ISP_StatusTypeDef;

/* ISP statistic area */
typedef struct
{
  uint32_t X0;
  uint32_t Y0;
  uint32_t XSize;
  uint32_t YSize;
} ISP_StatAreaTypeDef;

/* ISP algo state */
typedef enum
{
  ISP_ALGO_STATE_INIT = 0U,
  ISP_ALGO_STATE_NEED_STAT,
  ISP_ALGO_STATE_WAITING_STAT,
  ISP_ALGO_STATE_STAT_READY,
} ISP_AlgoStateTypeDef;

/* ISP algorithm handle structure */
/* The Init(), Deinit() and Process() functions have the following (void*) parameters:
 *   hIsp:  ISP device handle. To cast in (ISP_HandleTypeDef *).
 *   pAlgo: ISP algorithm handle. To cast in (ISP_AlgoTypeDef *).
 */
#define NB_PERF_MEASURES 10
typedef struct
{
  uint8_t id;
  ISP_AlgoStateTypeDef state;
  ISP_StatusTypeDef (*Init)(void *hIsp, void *pAlgo);
  ISP_StatusTypeDef (*DeInit)(void *hIsp, void *pAlgo);
  ISP_StatusTypeDef (*Process)(void *hIsp, void *pAlgo);
  /* Use for performance measurement */
  uint32_t perf_meas[NB_PERF_MEASURES];
  uint32_t iter;
} ISP_AlgoTypeDef;

/* ISP frame format */
typedef enum
{
  ISP_FORMAT_RGB888   = 0x00U,
  ISP_FORMAT_RAW8     = 0x01U,
  ISP_FORMAT_RAW10    = 0x02U,
  ISP_FORMAT_RAW12    = 0x03U,
  ISP_FORMAT_RAW14    = 0x04U,
} ISP_FormatTypeDef;

/* ISP frame meta data */
typedef struct
{
  uint32_t size;              /* Frame buffer size in bytes */
  uint32_t width;             /* Frame width */
  uint32_t height;            /* Frame height */
  uint32_t pitch;             /* Pitch in bytes */
  ISP_FormatTypeDef format;   /* Frame format */
} ISP_DumpFrameMetaTypeDef;

/* ISP dump config */
typedef enum
{
  ISP_DUMP_CFG_DEFAULT           = 0x00U,
  ISP_DUMP_CFG_FULLSIZE_RGB888   = 0x01U,
  ISP_DUMP_CFG_DUMP_PIPE_SENSOR  = 0x02U,
} ISP_DumpCfgTypeDef;

#define ISP_SENSOR_INFO_MAX_LENGTH      (32U)

typedef struct
{
  char name[ISP_SENSOR_INFO_MAX_LENGTH];
  uint8_t bayer_pattern;
  uint8_t color_depth;
  uint32_t width;
  uint32_t height;
  uint32_t gain_min;
  uint32_t gain_max;
  uint32_t exposure_min;
  uint32_t exposure_max;
} ISP_SensorInfoTypeDef;

/* ISP application helpers providing control of non ISP features */
typedef struct
{
  /* [OPTIONAL] Enable the camera preview on the LCD display */
  ISP_StatusTypeDef (*StartPreview)(void *pHdcmipp);
  /* [OPTIONAL] Disable the camera preview on the LCD display */
  ISP_StatusTypeDef (*StopPreview)(void *pHdcmipp);
  /* [OPTIONAL] Dump a frame of the camera pipeline. The parameters are:
  *    pHdcmipp:  DCMIPP device handle.
  *    Pipe:      Pipe where to perform the dump ('DUMP'(0) or 'ANCILLARY'(2))
  *    Config:    Dump with the current pipe config, or without downsizing with
  *               a specific pixel format.
  *    pBuffer:   Pointer to the address of the dumped buffer (output parameter)
  *    pMeta:     Pointer to buffer meta data (output parameter)
  */
  ISP_StatusTypeDef (*DumpFrame)(void *pHdcmipp,
                                 uint32_t Pipe,
                                 ISP_DumpCfgTypeDef Config,
                                 uint32_t **pBuffer,
                                 ISP_DumpFrameMetaTypeDef *pMeta);
  /* [MANDATORY] Get sensor info */
  ISP_StatusTypeDef (*GetSensorInfo)(uint32_t Instance, ISP_SensorInfoTypeDef *Info);
  /* [MANDATORY] Set sensor gain */
  ISP_StatusTypeDef (*SetSensorGain)(uint32_t Instance, int32_t Gain);
  /* [MANDATORY] Get sensor gain */
  ISP_StatusTypeDef (*GetSensorGain)(uint32_t Instance, int32_t *Gain);
  /* [MANDATORY] Set sensor exposure */
  ISP_StatusTypeDef (*SetSensorExposure)(uint32_t Instance, int32_t Exposure);
  /* [MANDATORY] Get sensor exposure */
  ISP_StatusTypeDef (*GetSensorExposure)(uint32_t Instance, int32_t *Exposure);
  /* [OPTIONAL] Set sensor test pattern */
  ISP_StatusTypeDef (*SetSensorTestPattern)(uint32_t Instance, int32_t mode);
} ISP_AppliHelpersTypeDef;

/* ISP Device handle structure */
typedef struct
{
  void *hDcmipp;
  uint32_t cameraInstance;
  ISP_StatAreaTypeDef statArea;
  ISP_AlgoTypeDef **algorithm;
  ISP_AppliHelpersTypeDef appliHelpers;
  uint32_t MainPipe_FrameCount;
  uint32_t AncillaryPipe_FrameCount;
  uint32_t DumpPipe_FrameCount;
  ISP_SensorInfoTypeDef sensorInfo;
} ISP_HandleTypeDef;

/* ISP Demosaicing type */
typedef enum
{
  ISP_DEMOS_TYPE_RGGB = 0x00U,
  ISP_DEMOS_TYPE_GRBG = 0x01U,
  ISP_DEMOS_TYPE_GBRG = 0x02U,
  ISP_DEMOS_TYPE_BGGR = 0x03U,
  ISP_DEMOS_TYPE_MONO = 0x04U,
} ISP_DemosTypeTypeDef;

/* ISP Decimation factor type */
typedef enum
{
  ISP_DECIM_FACTOR_1 = 0x01U,
  ISP_DECIM_FACTOR_2 = 0x02U,
  ISP_DECIM_FACTOR_4 = 0x04U,
  ISP_DECIM_FACTOR_8 = 0x08U,
} ISP_DecimFactorTypeDef;

/* ISP Contrast Luminance table */
typedef struct
{
  uint32_t LUM_0;           /* Unit = 100 for "x1.0", 150 for "x1.5" */
  uint32_t LUM_32;
  uint32_t LUM_64;
  uint32_t LUM_96;
  uint32_t LUM_128;
  uint32_t LUM_160;
  uint32_t LUM_192;
  uint32_t LUM_224;
  uint32_t LUM_256;
} ISP_ContrastLumCoeffTypeDef;

/* ISP demosaicing control */
typedef struct
{
  uint8_t enable;             /* Enable or disable */
  ISP_DemosTypeTypeDef type;  /* Bayer pattern type */
  uint8_t peak;               /* Peak detection relative algorithm strength */
  uint8_t lineV;              /* Vertical line detection relative algorithm strength */
  uint8_t lineH;              /* Horizontal line detection relative algorithm strength */
  uint8_t edge;               /* Edge detection relative algorithm strength */
} ISP_DemosaicingTypeDef;

/* ISP stat removal control */
typedef struct
{
  uint8_t enable;             /* Enable or disable */
  uint32_t nbHeadLines;       /* Number of headlines to remove */
  uint32_t nbValidLines;      /* Number of valid lines to keep */
} ISP_StatRemovalTypeDef;

typedef struct
{
  ISP_DecimFactorTypeDef factor;  /* Horizontal and vertical decimation factor */
} ISP_DecimationTypeDef;

typedef struct
{
  uint8_t enable;             /* Enable or disable */
  ISP_ContrastLumCoeffTypeDef coeff; /* Luminance amplification coefficients */
} ISP_ContrastTypeDef;

typedef struct
{
  uint32_t gain;              /* Gain in mdB */
} ISP_SensorGainTypeDef;

typedef struct
{
  uint32_t exposure;          /* Exposure time in micro seconds */
} ISP_SensorExposureTypeDef;


typedef struct
{
  uint8_t enablePipe1;        /* Enable or disable gamma on pipe 1 */
  uint8_t enablePipe2;        /* Enable or disable gamma on pipe 2 */
} ISP_GammaTypeDef;

typedef struct
{
  uint32_t mode;              /* Test pattern mode */
} ISP_SensorTestPatternTypeDef;

typedef struct
{
  uint8_t enable;
  uint32_t threshold;         /* Max number of detected bad pixels */
} ISP_BadPixelAlgoTypeDef;

typedef struct
{
  uint8_t enable;             /* Enable or disable */
  uint8_t strength;           /* Strength of the bad pixel removal algorithm */
  uint32_t count;             /* Reported number of bad pixels */
} ISP_BadPixelTypeDef;

typedef struct
{
  uint8_t enable;             /* Enable or disable */
  uint8_t BLCR;               /* Level offset for the red component */
  uint8_t BLCG;               /* Level offset for the green component */
  uint8_t BLCB;               /* Level offset for the blue component */
} ISP_BlackLevelTypeDef;

typedef enum
{
  EXPOSURE_TARGET_MINUS_2_0_EV = -4,
  EXPOSURE_TARGET_MINUS_1_5_EV = -3,
  EXPOSURE_TARGET_MINUS_1_0_EV = -2,
  EXPOSURE_TARGET_MINUS_0_5_EV = -1,
  EXPOSURE_TARGET_0_0_EV       =  0,
  EXPOSURE_TARGET_PLUS_0_5_EV  =  1,
  EXPOSURE_TARGET_PLUS_1_0_EV  =  2,
  EXPOSURE_TARGET_PLUS_1_5_EV  =  3,
  EXPOSURE_TARGET_PLUS_2_0_EV  =  4,
} ISP_ExposureCompTypeDef;

typedef struct
{
  uint8_t enable;                               /* Enable or disable */
  ISP_ExposureCompTypeDef exposureCompensation; /* Exposure Compensation (in EV) */
  uint32_t exposureTarget;                      /* Exposure Target */
} ISP_AECAlgoTypeDef;

#define ISP_AWB_COLORTEMP_REF               (5U)
#define ISP_AWB_PROFILE_ID_MAX_LENGTH      (32U)

typedef struct
{
  uint8_t enable;             /* Enable or disable */
  char id[ISP_AWB_COLORTEMP_REF][ISP_AWB_PROFILE_ID_MAX_LENGTH]; /* Array of profile names for identification */
  uint32_t referenceColorTemp[ISP_AWB_COLORTEMP_REF]; /* Array of reference color temperatures */
  uint32_t ispGainR[ISP_AWB_COLORTEMP_REF];   /* Array of gains of the red component. Unit = 100000000 for "x1.0", 150000000 for "x1.5". Max gain is "x255" */
  uint32_t ispGainG[ISP_AWB_COLORTEMP_REF];   /* Array of gains of the green component */
  uint32_t ispGainB[ISP_AWB_COLORTEMP_REF];   /* Array of gains of the blue component */
  int32_t coeff[ISP_AWB_COLORTEMP_REF][3][3]; /* Array of 3x3 RGB to RGB matrix coefficients. Unit = 100000000 for "x1.0", -150000000 for "x-1.5". Range is "x-4.0" to "x4.0" */
} ISP_AWBAlgoTypeDef;

typedef struct
{
  char id[ISP_AWB_PROFILE_ID_MAX_LENGTH]; /* profile name for identification */
  uint32_t referenceColorTemp;            /* reference color temperature */
} ISP_AWBProfileTypeDef;

typedef struct
{
  uint8_t enable;             /* Enable or disable */
  uint32_t ispGainR;          /* Gain of the red component. Unit = 100000000 for "x1.0", 150000000 for "x1.5". Max gain is "x16" */
  uint32_t ispGainG;          /* Gain of the green component */
  uint32_t ispGainB;          /* Gain of the blue component */
} ISP_ISPGainTypeDef;

typedef struct
{
  uint8_t enable;             /* Enable or disable */
  int32_t coeff[3][3];        /* 3x3 RGB to RGB matrix coefficients. Unit = 100000000 for "x1.0", -150000000 for "x-1.5". Range is "x-4.0" to "x4.0" */
} ISP_ColorConvTypeDef;

typedef struct
{
  uint8_t averageR;           /* Average of the red component */
  uint8_t averageG;           /* Average of the green component */
  uint8_t averageB;           /* Average of the blue component */
  uint8_t averageL;           /* Average of the Luminance */
  uint32_t histogram[12];     /* Histogram of the L, R, G or B component */
} ISP_StatisticsTypeDef;

/* IQ parameter */
typedef struct
{
  ISP_StatRemovalTypeDef statRemoval;
  ISP_DemosaicingTypeDef demosaicing;
  ISP_ContrastTypeDef contrast;
  ISP_StatAreaTypeDef statAreaStatic;
  ISP_SensorGainTypeDef sensorGainStatic;
  ISP_SensorExposureTypeDef sensorExposureStatic;
  ISP_BadPixelAlgoTypeDef badPixelAlgo;
  ISP_BadPixelTypeDef badPixelStatic;
  ISP_BlackLevelTypeDef blackLevelStatic;
  ISP_AECAlgoTypeDef AECAlgo;
  ISP_AWBAlgoTypeDef AWBAlgo;
  ISP_ISPGainTypeDef ispGainStatic;
  ISP_ColorConvTypeDef colorConvStatic;
  ISP_GammaTypeDef gamma;
} ISP_IQParamTypeDef;

/* Exported constants --------------------------------------------------------*/
#define ISP_DEMOS_STRENGTH_MAX              (7U)
#define ISP_STATREMOVAL_HEADLINES_MAX       (7U)
#define ISP_STATREMOVAL_VALIDLINES_MAX      (4094U)
#define ISP_CONTAST_LUMCOEFF_MAX            (394U)
#define ISP_BADPIXEL_STRENGTH_MAX           (7U)
#define ISP_EXPOSURE_GAIN_MAX               (1600000000U)
#define ISP_COLORCONV_MAX                   (399000000)
#define ISP_STATWINDOW_MAX                  (4094U)
#define ISP_STATWINDOW_MIN                  (4U)
#define ISP_AWB_ENABLE_RECONFIGURE          (255U)

/* A well exposed picture (taking into account the gamma correction) has its
 * Luminance average at the middle of the luminance range.
 * Here luminance range is [0, 255] so a well expose picture should have a
 * luminance equal to 128 gamma applied. In our case, the statistics are recovered
 * before the gamma correction so the target luminance for a well exposed picture
 * is 56 (((56/255)^(1/2.2))*255 = 128)
 */
#define ISP_IDEAL_TARGET_EXPOSURE 56

/* ISP pipeline is able to work on RAW line with maximum of 2688 pixels width */
#define ISP_RAW_MAX_WIDTH 2688

/* Exported macro ------------------------------------------------------------*/
#define ERROR_MESSAGE(err) \
    ( err == ISP_OK ? "No err" \
      : err == ISP_ERR_EINVAL ? "Invalid arg" \
      : err == ISP_ERR_DEMOSAICING_EINVAL ? "Demosaicing invalid arg" \
      : err == ISP_ERR_DEMOSAICING_HAL ? "Demosaicing HAL err" \
      : err == ISP_ERR_STATREMOVAL_EINVAL ? "Stat Removal invalid arg" \
      : err == ISP_ERR_STATREMOVAL_HAL ? "Stat Removal HAL err" \
      : err == ISP_ERR_DECIMATION_EINVAL ? "Decimation invalid arg" \
      : err == ISP_ERR_DECIMATION_HAL ? "Decimation HAL err" \
      : err == ISP_ERR_CONTRAST_EINVAL ? "Contrast invalid arg" \
      : err == ISP_ERR_CONTRAST_HAL ? "Contrast HAL err" \
      : err == ISP_ERR_STATAREA_EINVAL ? "Stat Area invalid arg" \
      : err == ISP_ERR_STATAREA_HAL ? "Stat Area HAL err" \
      : err == ISP_ERR_BADPIXEL_EINVAL ? "Bad Pixel invalid arg" \
      : err == ISP_ERR_BADPIXEL_HAL ? "Bad Pixel HAL err" \
      : err == ISP_ERR_BLACKLEVEL_EINVAL ? "Black level invalid arg" \
      : err == ISP_ERR_BLACKLEVEL_HAL ? "Black level HAL err" \
      : err == ISP_ERR_ISPGAIN_EINVAL ? "ISP gain invalid arg" \
      : err == ISP_ERR_ISPGAIN_HAL ? "ISP gain HAL err" \
      : err == ISP_ERR_COLORCONV_EINVAL ? "Color conv invalid arg" \
      : err == ISP_ERR_COLORCONV_HAL ? "Color conv HAL err" \
      : err == ISP_ERR_IQPARAM_EINVAL ? "IQ Params invalid arg" \
      : err == ISP_ERR_IQPARAM_HAL ? "IQ Params HAL err" \
      : err == ISP_ERR_IQPARAM_MISSING ? "IQ Params missing" \
      : err == ISP_ERR_SENSORINFO_EINVAL ? "Sensor info invalid arg" \
      : err == ISP_ERR_SENSORINFO ? "Sensor info err" \
      : err == ISP_ERR_SENSORGAIN_EINVAL ? "Sensor gain invalid arg" \
      : err == ISP_ERR_SENSORGAIN ? "Sensor gain err" \
      : err == ISP_ERR_SENSOREXPOSURE_EINVAL ? "Sensor exposure invalid arg" \
      : err == ISP_ERR_SENSOREXPOSURE ? "Sensor exposure err" \
      : err == ISP_ERR_DCMIPP_STATE ? "DCMIPP state err" \
      : err == ISP_ERR_DCMIPP_STOP ? "DCMIPP stop err" \
      : err == ISP_ERR_DCMIPP_START ? "DCMIPP start err" \
      : err == ISP_ERR_DCMIPP_DOWNSIZE ? "DCMIPP downsize err" \
      : err == ISP_ERR_DCMIPP_CONFIGPIPE ? "DCMIPP pipe config err" \
      : err == ISP_ERR_DCMIPP_DUMPTIMEOUT ? "DCMIPP dump timeout" \
      : err == ISP_ERR_DCMIPP_PIXELFORMAT ? "DCMIPP pixel format err" \
      : err == ISP_ERR_DCMIPP_FRAMESIZE ? "DCMIPP frame size err" \
      : err == ISP_ERR_DCMIPP_NOMEM ? "DCMIPP memory err" \
      : err == ISP_ERR_DCMIPP_GAMMA ? "DCMIPP gamma err" \
      : err == ISP_ERR_WB_COLORTEMP ? "WB ColorTemp err" \
      : err == ISP_ERR_CMDPARSER_COMMAND ? "CmdParser CMD err" \
      : err == ISP_ERR_CMDPARSER_OPERATION ? "CmdParser OP err" \
      : err == ISP_ERR_STAT_EINVAL ? "Stat invalid arg" \
      : err == ISP_ERR_STAT_HAL ? "Stat HAL err" \
      : err == ISP_ERR_STAT_MAXCLIENTS ? "Stat Max Clients err" \
      : err == ISP_ERR_APP_HELPER_UNDEFINED ? "App Helper undefined" \
      : err == ISP_ERR_ALGO ? "Algo err" \
      : err == ISP_ERR_SENSORTESTPATTERN ? "Sensor Test Pattern" \
      : "Unknown error" )

/* Exported functions ------------------------------------------------------- */

#endif /* __ISP_CORE__H */
