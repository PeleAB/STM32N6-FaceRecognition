/**
 ******************************************************************************
 * @file    isp_algo.c
 * @author  AIS Application Team
 * @brief   ISP algorithm
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
#include "isp_algo.h"
#include "isp_services.h"
#include "evision-api-st-ae.h"
#include "evision-api-awb.h"
#include "evision-api-utils.h"
#include <limits.h>
#include <math.h>
#include <inttypes.h>

/* Private types -------------------------------------------------------------*/
/* ISP algorithms identifier */
typedef enum
{
  ISP_ALGO_ID_BADPIXEL = 0U,
  ISP_ALGO_ID_AEC,
  ISP_ALGO_ID_AWB,
  ISP_ALGO_ID_SENSOR_DELAY = 255U,
} ISP_AlgoIDTypeDef;

/* Private constants ---------------------------------------------------------*/
/* Delay (in number of VSYNC) between the time an ISP control (e.g. ColorConv)
 * is updated and the time the frame is actually updated. Typical user = AWB algo. */
#define ALGO_ISP_LATENCY                             2
/* Additional delay to let things getting stable after an AWB update */
#define ALGO_AWB_ADDITIONAL_LATENCY                  3
#define ALGO_AWB_STAT_CHECK_SKIP_AFTER_INIT          10
#define ALGO_AWB_STAT_CHECK_SKIP_AFTER_CT_ESTIMATION 6

/* Debug logs control */
//#define ALGO_AWB_DBG_LOGS
//#define ALGO_AEC_DBG_LOGS
//#define ALGO_PERF_DBG_LOGS

/* Max acceptable sensor delay */
#define ALGO_DELAY_MAX               10
/* Number of delay test configurations */
#define ALGO_DELAY_NB_CONFIG         12
/* Minimum Luminance update during delay test measurements */
#define ALGO_DELAY_L_MARGIN          3

/* Private macro -------------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
ISP_StatusTypeDef ISP_Algo_BadPixel_Init(void *hIsp, void *pAlgo);
ISP_StatusTypeDef ISP_Algo_BadPixel_DeInit(void *hIsp, void *pAlgo);
ISP_StatusTypeDef ISP_Algo_BadPixel_Process(void *hIsp, void *pAlgo);
ISP_StatusTypeDef ISP_Algo_AEC_Init(void *hIsp, void *pAlgo);
ISP_StatusTypeDef ISP_Algo_AEC_DeInit(void *hIsp, void *pAlgo);
ISP_StatusTypeDef ISP_Algo_AEC_Process(void *hIsp, void *pAlgo);
ISP_StatusTypeDef ISP_Algo_AWB_Init(void *hIsp, void *pAlgo);
ISP_StatusTypeDef ISP_Algo_AWB_DeInit(void *hIsp, void *pAlgo);
ISP_StatusTypeDef ISP_Algo_AWB_Process(void *hIsp, void *pAlgo);
#ifdef ISP_MW_TUNING_TOOL_SUPPORT
ISP_StatusTypeDef ISP_Algo_SensorDelay_Init(void *hIsp, void *pAlgo);
ISP_StatusTypeDef ISP_Algo_SensorDelay_DeInit(void *hIsp, void *pAlgo);
ISP_StatusTypeDef ISP_Algo_SensorDelay_Process(void *hIsp, void *pAlgo);
#endif

/* Private variables ---------------------------------------------------------*/
/* Bad Pixel algorithm handle */
ISP_AlgoTypeDef ISP_Algo_BadPixel = {
    .id = ISP_ALGO_ID_BADPIXEL,
    .Init = ISP_Algo_BadPixel_Init,
    .DeInit = ISP_Algo_BadPixel_DeInit,
    .Process = ISP_Algo_BadPixel_Process,
};

#ifdef ISP_MW_SW_AEC_ALGO_SUPPORT
/* AEC algorithm handle */
ISP_AlgoTypeDef ISP_Algo_AEC = {
    .id = ISP_ALGO_ID_AEC,
    .Init = ISP_Algo_AEC_Init,
    .DeInit = ISP_Algo_AEC_DeInit,
    .Process = ISP_Algo_AEC_Process,
};

/* Default hyper parameter of the AEC algorithm */
#define HYPERPARAM_AEC_TOLERANCE                10      /* Max delta between lum stat and target in convergence region */
#define HYPERPARAM_AEC_GAIN_INCREMENT_COEFF     100     /* Factor applied to increment gain update */
#define HYPERPARAM_AEC_GAIN_LOW_DELTA           45      /* Max delta value between lum stat and target in low delta region */
#define HYPERPARAM_AEC_GAIN_HIGH_DELTA          120     /* Min delta value between lum stat and target in high delta region */
#define HYPERPARAM_AEC_GAIN_LOW_INC_MAX         1500    /* Maximum gain update value in luminance low delta region */
#define HYPERPARAM_AEC_GAIN_MEDIUM_INC_MAX      6000    /* Maximum gain update value in luminance medium delta region */
#define HYPERPARAM_AEC_GAIN_HIGH_INC_MAX        12000   /* Maximum gain update value in luminance high delta region */
#define HYPERPARAM_AEC_EXPOSURE_UP_RATIO        0.020F  /* Factor applied to increment exposure */
#define HYPERPARAM_AEC_EXPOSURE_DOWN_RATIO      0.004F  /* Factor applied to decrement exposure */
#define HYPERPARAM_AEC_DARKZONE_LUM_LIMIT       5       /* Default value for dark zone luminance limit */
#endif /* ISP_MW_SW_AEC_ALGO_SUPPORT */

#ifdef ISP_MW_SW_AWB_ALGO_SUPPORT
/* AWB algorithm handle */
ISP_AlgoTypeDef ISP_Algo_AWB = {
    .id = ISP_ALGO_ID_AWB,
    .Init = ISP_Algo_AWB_Init,
    .DeInit = ISP_Algo_AWB_DeInit,
    .Process = ISP_Algo_AWB_Process,
};
#endif /* ISP_MW_SW_AWB_ALGO_SUPPORT */

#ifdef ISP_MW_TUNING_TOOL_SUPPORT
/* Sensor Delay measurement algorithm handle */
ISP_AlgoTypeDef ISP_Algo_SensorDelay = {
    .id = ISP_ALGO_ID_SENSOR_DELAY,
    .Init = ISP_Algo_SensorDelay_Init,
    .DeInit = ISP_Algo_SensorDelay_DeInit,
    .Process = ISP_Algo_SensorDelay_Process,
};
#endif

#ifdef ALGO_PERF_DBG_LOGS
#define MEAS_ITERATION 10
uint32_t tickstart;
uint32_t duration[MEAS_ITERATION];
uint32_t iter = 0;
#endif

/* Registered algorithm list */
ISP_AlgoTypeDef *ISP_Algo_List[] = {
    &ISP_Algo_BadPixel,
#ifdef ISP_MW_SW_AEC_ALGO_SUPPORT
    &ISP_Algo_AEC,
#endif /* ISP_MW_SW_AEC_ALGO_SUPPORT */
#ifdef ISP_MW_SW_AWB_ALGO_SUPPORT
    &ISP_Algo_AWB,
#endif /* ISP_MW_SW_AWB_ALGO_SUPPORT */
#ifdef ISP_MW_TUNING_TOOL_SUPPORT
    &ISP_Algo_SensorDelay,
#endif /* ISP_MW_TUNING_TOOL_SUPPORT */
};

#ifdef ISP_MW_SW_AEC_ALGO_SUPPORT
/* Algo internal */
static evision_st_ae_process_t *pIspAEprocess;
#endif /* ISP_MW_SW_AEC_ALGO_SUPPORT */

#ifdef ISP_MW_SW_AWB_ALGO_SUPPORT
/* Algo internal */
static evision_awb_estimator_t* pIspAWBestimator;
#endif /* ISP_MW_SW_AWB_ALGO_SUPPORT */

/* Global variables ----------------------------------------------------------*/
uint32_t current_awb_profId = 0;
ISP_MetaTypeDef Meta = {0};

/* Private functions ---------------------------------------------------------*/
/**
  * @brief  ISP_Algo_BadPixel_Init
  *         Initialize the BadPixel algorithm
  * @param  hIsp:  ISP device handle. To cast in (ISP_HandleTypeDef *).
  * @param  pAlgo: ISP algorithm handle. To cast in (ISP_AlgoTypeDef *).
  * @retval operation result
  */
ISP_StatusTypeDef ISP_Algo_BadPixel_Init(void *hIsp, void *pAlgo)
{
  (void)hIsp; /* unused */

  ((ISP_AlgoTypeDef *)pAlgo)->state = ISP_ALGO_STATE_INIT;

  return ISP_OK;
}

/**
  * @brief  ISP_Algo_BadPixel_DeInit
  *         Deinitialize the BadPixel algorithm
  * @param  hIsp:  ISP device handle. To cast in (ISP_HandleTypeDef *).
  * @param  pAlgo: ISP algorithm handle. To cast in (ISP_AlgoTypeDef *).
  * @retval operation result
  */
ISP_StatusTypeDef ISP_Algo_BadPixel_DeInit(void *hIsp, void *pAlgo)
{
  (void)hIsp; /* unused */
  (void)pAlgo; /* unused */

  return ISP_OK;
}

/**
  * @brief  ISP_Algo_BadPixel_Process
  *         Process the BadPixel algorithm
  * @param  hIsp:  ISP device handle. To cast in (ISP_HandleTypeDef *).
  * @param  pAlgo: ISP algorithm handle. To cast in (ISP_AlgoTypeDef *).
  * @retval operation result
  */
ISP_StatusTypeDef ISP_Algo_BadPixel_Process(void *hIsp, void *pAlgo)
{
  (void)hIsp; /* unused */
  (void)pAlgo; /* unused */
  static uint32_t BadPixelCount, LastFrameId;
  static int8_t Step;
  uint32_t CurrentFrameId;
  ISP_BadPixelTypeDef BadPixelConfig;
  ISP_IQParamTypeDef *IQParamConfig;
  ISP_StatusTypeDef ret;

  IQParamConfig = ISP_SVC_IQParam_Get(hIsp);

  if (IQParamConfig->badPixelAlgo.enable == false)
  {
    return ISP_OK;
  }

  /* Wait for a new frame */
  CurrentFrameId = ISP_SVC_Misc_GetMainFrameId(hIsp);
  if (CurrentFrameId == LastFrameId)
  {
    return ISP_OK;
  }
  LastFrameId = CurrentFrameId;

  if (Step++ >= 0)
  {
    /* Measure the number of bad pixels */
    ret  = ISP_SVC_ISP_GetBadPixel(hIsp, &BadPixelConfig);
    if (ret != ISP_OK)
    {
      return ret;
    }
    BadPixelCount += BadPixelConfig.count;
  }

  if (Step == 10)
  {
    /* All measures done : make an average and compare with threshold */
    BadPixelCount /= 10;

    if ((BadPixelCount > IQParamConfig->badPixelAlgo.threshold) && (BadPixelConfig.strength > 0))
    {
      /* Bad pixel is above target : decrease strength */
      BadPixelConfig.strength--;
    }
    else if ((BadPixelCount < IQParamConfig->badPixelAlgo.threshold) && (BadPixelConfig.strength < ISP_BADPIXEL_STRENGTH_MAX - 1))
    {
      /* Bad pixel is below target : increase strength. (exclude ISP_BADPIXEL_STRENGTH_MAX which gives weird results) */
      BadPixelConfig.strength++;
    }

    /* Set updated Strength */
    BadPixelConfig.enable = 1;
    ret = ISP_SVC_ISP_SetBadPixel(hIsp, &BadPixelConfig);
    if (ret != ISP_OK)
    {
      return ret;
    }

    /* Set Step to -1 to wait for an extra frame before a new measurement (the ISP HW needs one frame to update after reconfig) */
    Step = -1;
    BadPixelCount = 0;
  }

  return ISP_OK;
}

#if defined(ISP_MW_SW_AEC_ALGO_SUPPORT) || defined(ISP_MW_SW_AWB_ALGO_SUPPORT)
static void log_cb(const char *const msg)
{
  printf("%s", msg);
  printf("\r\n");
}
#endif

#ifdef ISP_MW_SW_AEC_ALGO_SUPPORT
/**
  * @brief  ISP_Algo_AEC_Init
  *         Initialize the AEC algorithm
  * @param  hIsp:  ISP device handle. To cast in (ISP_HandleTypeDef *).
  * @param  pAlgo: ISP algorithm handle. To cast in (ISP_AlgoTypeDef *).
  * @retval operation result
  */
ISP_StatusTypeDef ISP_Algo_AEC_Init(void *hIsp, void *pAlgo)
{
  ISP_HandleTypeDef *pIsp_handle = (ISP_HandleTypeDef*) hIsp;
  ISP_AlgoTypeDef *algo = (ISP_AlgoTypeDef *)pAlgo;
  ISP_SensorExposureTypeDef exposureConfig;
  ISP_SensorGainTypeDef gainConfig;
  ISP_IQParamTypeDef *IQParamConfig;
  evision_return_t e_ret;

  IQParamConfig = ISP_SVC_IQParam_Get(hIsp);

  if (IQParamConfig->sensorDelay.delay == 0)
  {
    /* A value of 0 is invalid, it would break the AEC algo */
    IQParamConfig->sensorDelay.delay = 1;
  }

  /* Create st_ae_process instance */
  pIspAEprocess = evision_api_st_ae_new(log_cb);
  if (pIspAEprocess == NULL)
  {
    return ISP_ERR_ALGO;
  }

  /* Initialize st_ae_process instance */
  e_ret = evision_api_st_ae_init(pIspAEprocess);
  if (e_ret != EVISION_RET_SUCCESS)
  {
    evision_api_st_ae_delete(pIspAEprocess);
    return ISP_ERR_ALGO;
  }

  /* Configure algo (AEC target and anti-flicker setting) */
  pIspAEprocess->hyper_params.target = IQParamConfig->AECAlgo.exposureTarget;
  pIspAEprocess->hyper_params.compat_freq = IQParamConfig->AECAlgo.antiFlickerFreq;

  /* Configure algo (sensor config) */
  pIspAEprocess->hyper_params.exposure_min = pIsp_handle->sensorInfo.exposure_min;
  pIspAEprocess->hyper_params.exposure_max = pIsp_handle->sensorInfo.exposure_max;
  pIspAEprocess->hyper_params.gain_min = pIsp_handle->sensorInfo.gain_min;
  pIspAEprocess->hyper_params.gain_max = pIsp_handle->sensorInfo.gain_max;

  /* Force hyper parameters with configuration defined in evision-api-st-ae.h */
  pIspAEprocess->hyper_params.tolerance = HYPERPARAM_AEC_TOLERANCE;
  pIspAEprocess->hyper_params.gain_increment_coeff = HYPERPARAM_AEC_GAIN_INCREMENT_COEFF;
  pIspAEprocess->hyper_params.gain_low_delta = HYPERPARAM_AEC_GAIN_LOW_DELTA;
  pIspAEprocess->hyper_params.gain_high_delta = HYPERPARAM_AEC_GAIN_HIGH_DELTA;
  pIspAEprocess->hyper_params.gain_low_increment_max = HYPERPARAM_AEC_GAIN_LOW_INC_MAX;
  pIspAEprocess->hyper_params.gain_medium_increment_max = HYPERPARAM_AEC_GAIN_MEDIUM_INC_MAX;
  pIspAEprocess->hyper_params.gain_high_increment_max = HYPERPARAM_AEC_GAIN_HIGH_INC_MAX;
  pIspAEprocess->hyper_params.exposure_up_ratio = HYPERPARAM_AEC_EXPOSURE_UP_RATIO;
  pIspAEprocess->hyper_params.exposure_down_ratio = HYPERPARAM_AEC_EXPOSURE_DOWN_RATIO;
  pIspAEprocess->hyper_params.dark_zone_lum_limit = HYPERPARAM_AEC_DARKZONE_LUM_LIMIT;

  /* Initialize exposure and gain at min value */
  if (IQParamConfig->AECAlgo.enable == true)
  {
    exposureConfig.exposure = pIsp_handle->sensorInfo.exposure_min;
    gainConfig.gain = pIsp_handle->sensorInfo.gain_min;
    if ((ISP_SVC_Sensor_SetExposure(hIsp, &exposureConfig) != ISP_OK) || (ISP_SVC_Sensor_SetGain(hIsp, &gainConfig)!= ISP_OK))
    {
      return ISP_ERR_ALGO;
    }
  }

  /* Update State */
  algo->state = ISP_ALGO_STATE_INIT;

  return ISP_OK;
}

/**
  * @brief  ISP_Algo_AEC_DeInit
  *         Deinitialize the AEC algorithm
  * @param  hIsp:  ISP device handle. To cast in (ISP_HandleTypeDef *).
  * @param  pAlgo: ISP algorithm handle. To cast in (ISP_AlgoTypeDef *).
  * @retval operation result
  */
ISP_StatusTypeDef ISP_Algo_AEC_DeInit(void *hIsp, void *pAlgo)
{
  (void)hIsp; /* unused */
  (void)pAlgo; /* unused */

  if (pIspAEprocess != NULL)
  {
    evision_api_st_ae_delete(pIspAEprocess);
  }
  return ISP_OK;
}

/**
  * @brief  ISP_Algo_AEC_StatCb
  *         Callback informing that statistics are available
  * @param  pAlgo: ISP algorithm handle.
  * @retval operation result
  */
ISP_StatusTypeDef ISP_Algo_AEC_StatCb(ISP_AlgoTypeDef *pAlgo)
{
  /* Update State */
  pAlgo->state = ISP_ALGO_STATE_STAT_READY;

  return ISP_OK;
}

/**
  * @brief  ISP_Algo_AEC_Process
  *         Process the AEC algorithm. This basic algorithm controls the sensor gain
  *         in order to reach an average luminance of exposureTarget.
  * @param  hIsp:  ISP device handle. To cast in (ISP_HandleTypeDef *).
  * @param  pAlgo: ISP algorithm handle. To cast in (ISP_AlgoTypeDef *).
  * @retval operation result
  */
ISP_StatusTypeDef ISP_Algo_AEC_Process(void *hIsp, void *pAlgo)
{
  static ISP_SVC_StatStateTypeDef stats;
  ISP_AlgoTypeDef *algo = (ISP_AlgoTypeDef *)pAlgo;
  ISP_IQParamTypeDef *IQParamConfig;
  ISP_StatusTypeDef ret = ISP_OK;
  ISP_SensorGainTypeDef gainConfig;
  ISP_SensorExposureTypeDef exposureConfig;
  uint32_t avgL;
#ifdef ALGO_AEC_DBG_LOGS
  static uint32_t currentL;
#endif
  evision_return_t e_ret;

  IQParamConfig = ISP_SVC_IQParam_Get(hIsp);
  if (IQParamConfig->AECAlgo.enable == false)
  {
    return ISP_OK;
  }

  switch(algo->state)
  {
  case ISP_ALGO_STATE_INIT:
  case ISP_ALGO_STATE_NEED_STAT:
    /* Ask for stats */
    ret = ISP_SVC_Stats_GetNext(hIsp, &ISP_Algo_AEC_StatCb, pAlgo, &stats, ISP_STAT_LOC_DOWN,
                                ISP_STAT_TYPE_AVG, IQParamConfig->sensorDelay.delay);
    if (ret != ISP_OK)
    {
      return ret;
    }

    /* Wait for stats to be ready */
    algo->state = ISP_ALGO_STATE_WAITING_STAT;
    break;

  case ISP_ALGO_STATE_WAITING_STAT:
    /* Do nothing */
    break;

  case ISP_ALGO_STATE_STAT_READY:
    /* Align on the target update (may have been updated with ISP_SetExposureTarget()) */
    pIspAEprocess->hyper_params.target = IQParamConfig->AECAlgo.exposureTarget;

    /* Align on the anti-flicker frequency (may have been updated by IQTune)*/
    pIspAEprocess->hyper_params.compat_freq = IQParamConfig->AECAlgo.antiFlickerFreq;

    avgL = stats.down.averageL;
#ifdef ALGO_AEC_DBG_LOGS
    if (avgL != currentL)
    {
      printf("L = %"PRIu32"\r\n", avgL);
      currentL = avgL;
    }
#endif
    /* Read the current sensor gain */
    ret = ISP_SVC_Sensor_GetGain(hIsp, &gainConfig);
    if (ret != ISP_OK)
    {
      return ret;
    }

    ret = ISP_SVC_Sensor_GetExposure(hIsp, &exposureConfig);
    if (ret != ISP_OK)
    {
      return ret;
    }

    /* Store meta data */
    Meta.averageL = (uint8_t)avgL;
    Meta.exposureTarget = IQParamConfig->AECAlgo.exposureTarget;

    /* Run algo to calculate new gain and exposure */
    e_ret = evision_api_st_ae_process(pIspAEprocess, gainConfig.gain, exposureConfig.exposure, (uint8_t)avgL);
    if (e_ret == EVISION_RET_SUCCESS)
    {
      if (gainConfig.gain != pIspAEprocess->new_gain)
      {
        /* Set new gain */
        gainConfig.gain = pIspAEprocess->new_gain;

        ret = ISP_SVC_Sensor_SetGain(hIsp, &gainConfig);
        if (ret != ISP_OK)
        {
          return ret;
        }

#ifdef ALGO_AEC_DBG_LOGS
        printf("New gain = %"PRIu32"\r\n", gainConfig.gain);
#endif
      }

      if (exposureConfig.exposure != pIspAEprocess->new_exposure)
      {
        /* Set new exposure */
        exposureConfig.exposure = pIspAEprocess->new_exposure;

        ret = ISP_SVC_Sensor_SetExposure(hIsp, &exposureConfig);
        if (ret != ISP_OK)
        {
          return ret;
        }

#ifdef ALGO_AEC_DBG_LOGS
        printf("New exposure = %"PRIu32"\r\n", exposureConfig.exposure);
#endif
      }
    }

    /* Ask for stats */
    ret = ISP_SVC_Stats_GetNext(hIsp, &ISP_Algo_AEC_StatCb, pAlgo, &stats, ISP_STAT_LOC_DOWN,
                                ISP_STAT_TYPE_AVG, IQParamConfig->sensorDelay.delay);

    /* Wait for stats to be ready */
    algo->state = ISP_ALGO_STATE_WAITING_STAT;
    break;

  default:
    printf("WARNING: Unknown AE algo state\r\n");
    /* Reset state to ISP_ALGO_STATE_INIT */
    algo->state = ISP_ALGO_STATE_INIT;
    break;

  }

  return ret;
}
#endif /* ISP_MW_SW_AEC_ALGO_SUPPORT */

#ifdef ISP_MW_SW_AWB_ALGO_SUPPORT
/**
  * @brief  ISP_Algo_ApplyGammaInverse
  *         Apply Gamma 1/2.2 correction to a component value
  * @param  hIsp:  ISP device handle.
  * @param  comp: component value
  * @retval gamma corrected value
  */
double ISP_Algo_ApplyGammaInverse(ISP_HandleTypeDef *hIsp, uint32_t comp)
{
  double out;

  /* Check if gamma is enabled */
  if (ISP_SVC_Misc_IsGammaEnabled(hIsp, 1 /*main pipe*/) != 0) {
    out = 255 * pow((float)comp / 255, 1.0 / 2.2);
  }
  else
  {
    out = (double) comp;
  }
  return out;
}

/**
  * @brief  ISP_Algo_GetUpStat
  *         Recalculate average up statistics from collected down statistics
  * @param  hIsp:  ISP device handle.
  * @param  pStats: pointer to the statistics
  */
void ISP_Algo_GetUpStat(ISP_HandleTypeDef *hIsp, ISP_SVC_StatStateTypeDef *pStats)
{
  ISP_ISPGainTypeDef ISPGain;
  ISP_BlackLevelTypeDef BlackLevel;
  int64_t upR, upG, upB;

  if ((ISP_SVC_ISP_GetGain(hIsp, &ISPGain) == ISP_OK) && (ISPGain.enable == 1))
  {

    /* reverse gain */
    upR = (int64_t) pStats->down.averageR * ISP_GAIN_PRECISION_FACTOR / ISPGain.ispGainR;
    upG = (int64_t) pStats->down.averageG * ISP_GAIN_PRECISION_FACTOR / ISPGain.ispGainG;
    upB = (int64_t) pStats->down.averageB * ISP_GAIN_PRECISION_FACTOR / ISPGain.ispGainB;

    pStats->up.averageR = (uint8_t) upR;
    pStats->up.averageG = (uint8_t) upG;
    pStats->up.averageB = (uint8_t) upB;

    if ((ISP_SVC_ISP_GetBlackLevel(hIsp, &BlackLevel) == ISP_OK) && (BlackLevel.enable == 1))
    {
        /* reverse black level */
    pStats->up.averageR += BlackLevel.BLCR;
    pStats->up.averageG += BlackLevel.BLCG;
    pStats->up.averageB += BlackLevel.BLCB;
    }
  }
  else
  {
    pStats->up.averageR = pStats->down.averageR;
    pStats->up.averageG = pStats->down.averageG;
    pStats->up.averageB = pStats->down.averageB;
  }
}

/**
  * @brief  ISP_Algo_ApplyCConv
  *         Apply Color Conversion matrix to RGB components, clamping output values to [0-255]
  * @param  hIsp:  ISP device handle.
  * @param  inR: Red component value
  * @param  inG: Green component value
  * @param  inB: Blue component value
  * @param  outR: pointer to Red component value after color conversion
  * @param  outG: pointer to Green component value after color conversion
  * @param  outB: pointer to Blue component value after color conversion
  * @retval None
  */
void ISP_Algo_ApplyCConv(ISP_HandleTypeDef *hIsp, uint32_t inR, uint32_t inG, uint32_t inB, uint32_t *outR, uint32_t *outG, uint32_t *outB)
{
  ISP_ColorConvTypeDef colorConv;
  int64_t ccR, ccG, ccB;

  if ((ISP_SVC_ISP_GetColorConv(hIsp, &colorConv) == ISP_OK) && (colorConv.enable == 1))
  {
    /* Apply ColorConversion matrix to the input components */
    ccR = (int64_t) inR * colorConv.coeff[0][0] + (int64_t) inG * colorConv.coeff[0][1] + (int64_t) inB * colorConv.coeff[0][2];
    ccG = (int64_t) inR * colorConv.coeff[1][0] + (int64_t) inG * colorConv.coeff[1][1] + (int64_t) inB * colorConv.coeff[1][2];
    ccB = (int64_t) inR * colorConv.coeff[2][0] + (int64_t) inG * colorConv.coeff[2][1] + (int64_t) inB * colorConv.coeff[2][2];

    ccR /= ISP_CCM_PRECISION_FACTOR;
    ccG /= ISP_CCM_PRECISION_FACTOR;
    ccB /= ISP_CCM_PRECISION_FACTOR;

    /* Clamp values to 0-255 */
    ccR = (ccR < 0) ? 0 : (ccR > 255) ? 255 : ccR;
    ccG = (ccG < 0) ? 0 : (ccG > 255) ? 255 : ccG;
    ccB = (ccB < 0) ? 0 : (ccB > 255) ? 255 : ccB;

    *outR = (uint32_t) ccR;
    *outG = (uint32_t) ccG;
    *outB = (uint32_t) ccB;
  }
  else
  {
    *outR = inR;
    *outG = inG;
    *outB = inB;
  }
}

/**
  * @brief  ISP_Algo_AWB_Init
  *         Initialize the AWB algorithm
  * @param  hIsp:  ISP device handle. To cast in (ISP_HandleTypeDef *).
  * @param  pAlgo: ISP algorithm handle. To cast in (ISP_AlgoTypeDef *).
  * @retval operation result
  */
ISP_StatusTypeDef ISP_Algo_AWB_Init(void *hIsp, void *pAlgo)
{
  (void)hIsp; /* unused */
  ISP_AlgoTypeDef *algo = (ISP_AlgoTypeDef *)pAlgo;

  /* Create estimator */
  pIspAWBestimator = evision_api_awb_new(log_cb);
  if (pIspAWBestimator == NULL)
  {
    return ISP_ERR_ALGO;
  }

  /* Continue the initialization in ISP_Algo_AWB_Process() function when state is ISP_ALGO_STATE_INIT.
   * This allows to read the IQ params after an algo stop/start cycle */
  algo->state = ISP_ALGO_STATE_INIT;

  return ISP_OK;
}

/**
  * @brief  ISP_Algo_AWB_DeInit
  *         Deinitialize the AWB algorithm
  * @param  hIsp:  ISP device handle. To cast in (ISP_HandleTypeDef *).
  * @param  pAlgo: ISP algorithm handle. To cast in (ISP_AlgoTypeDef *).
  * @retval operation result
  */
ISP_StatusTypeDef ISP_Algo_AWB_DeInit(void *hIsp, void *pAlgo)
{
  (void)hIsp; /* unused */
  (void)pAlgo; /* unused */

  if (pIspAWBestimator != NULL)
  {
    evision_api_awb_delete(pIspAWBestimator);
  }

  return ISP_OK;
}

/**
  * @brief  ISP_Algo_AWB_StatCb
  *         Callback informing that statistics are available
  * @param  pAlgo: ISP algorithm handle.
  * @retval operation result
  */
ISP_StatusTypeDef ISP_Algo_AWB_StatCb(ISP_AlgoTypeDef *pAlgo)
{
  /* Update State */
  if (pAlgo->state != ISP_ALGO_STATE_INIT)
  {
    pAlgo->state = ISP_ALGO_STATE_STAT_READY;
  }

  return ISP_OK;
}

/**
  * @brief  ISP_Algo_AWB_Process
  *         Process the AWB algorithm. This algorithm controls the ISP gain and color conversion
  *         in order to output realistic colors (white balance).
  * @param  hIsp:  ISP device handle. To cast in (ISP_HandleTypeDef *).
  * @param  pAlgo: ISP algorithm handle. To cast in (ISP_AlgoTypeDef *).
  * @retval operation result
  */
ISP_StatusTypeDef ISP_Algo_AWB_Process(void *hIsp, void *pAlgo)
{
  static ISP_SVC_StatStateTypeDef stats;
  static uint8_t enableCurrent = false;
  static uint8_t reconfigureRequest = false;
  static uint32_t currentColorTemp = 0;
  static evision_awb_profile_t awbProfiles[ISP_AWB_COLORTEMP_REF];
  static float colorTempThresholds[ISP_AWB_COLORTEMP_REF - 1];
  ISP_IQParamTypeDef *IQParamConfig;
  ISP_ColorConvTypeDef ColorConvConfig;
  ISP_ISPGainTypeDef ISPGainConfig;
  ISP_AlgoTypeDef *algo = (ISP_AlgoTypeDef *)pAlgo;
  ISP_StatusTypeDef ret_stat, ret = ISP_OK;
  evision_return_t e_ret;
  uint32_t ccAvgR, ccAvgG, ccAvgB, colorTemp, i, j, profId, profNb;
  float cfaGains[4], ccmCoeffs[3][3], ccmOffsets[3] = { 0 };
  double meas[3];
  static uint32_t statsHistory[3][3] = { 0 };
  static uint32_t colorTempHistory[2] = { 0 };
  static uint8_t skip_stat_check_count = ALGO_AWB_STAT_CHECK_SKIP_AFTER_INIT;
  uint8_t stat_has_changed = false;

  IQParamConfig = ISP_SVC_IQParam_Get(hIsp);

  if (IQParamConfig->AWBAlgo.enable == false)
  {
    enableCurrent = false;
    return ISP_OK;
  }
  else if ((enableCurrent == false) || (IQParamConfig->AWBAlgo.enable == ISP_AWB_ENABLE_RECONFIGURE))
  {
    /* Start or resume algo : set state to INIT in order to read the IQ params */
    algo->state = ISP_ALGO_STATE_INIT;
    IQParamConfig->AWBAlgo.enable = true;
    reconfigureRequest = true;
    enableCurrent = true;
    skip_stat_check_count = ALGO_AWB_STAT_CHECK_SKIP_AFTER_CT_ESTIMATION;
  }

  switch(algo->state)
  {
  case ISP_ALGO_STATE_INIT:
    profNb = 0;

    /* Reset color temperature history */
    for (i = 0; i < 2; i++)
    {
      colorTempHistory[i] = 0;
    }

    /* Set profiles (color temperature, gains, color conv matrix) */
    for (profId = 0; profId < ISP_AWB_COLORTEMP_REF; profId++)
    {
      colorTemp = IQParamConfig->AWBAlgo.referenceColorTemp[profId];
      if (colorTemp == 0)
        break;

      if (profNb > 0)
      {
        /* Profile decision threshold = lowest ref. temperature + 1/4 of the distance between two reference temperatures */
        colorTempThresholds[profNb - 1] = (float) ((colorTemp + 3 * IQParamConfig->AWBAlgo.referenceColorTemp[profId - 1]) /4 );
      }

      /* Set cfa gains (RGGB) */
      cfaGains[0] = (float) IQParamConfig->AWBAlgo.ispGainR[profId] / ISP_GAIN_PRECISION_FACTOR;
      cfaGains[1] = (float) IQParamConfig->AWBAlgo.ispGainG[profId] / ISP_GAIN_PRECISION_FACTOR;
      cfaGains[2] = cfaGains[1];
      cfaGains[3] = (float) IQParamConfig->AWBAlgo.ispGainB[profId] / ISP_GAIN_PRECISION_FACTOR;

      /* Set CCM Coeff */
      for (i = 0; i < 3; i++)
      {
        for (j = 0; j < 3; j++)
        {
          ccmCoeffs[i][j] = (float) IQParamConfig->AWBAlgo.coeff[profId][i][j] / ISP_CCM_PRECISION_FACTOR;
        }
      }

      /* Set profile */
      evision_api_awb_set_profile(&awbProfiles[profId], (float) colorTemp, cfaGains, ccmCoeffs, ccmOffsets);
      profNb++;
    }

    if (profNb == 0)
    {
      return ISP_ERR_WB_COLORTEMP;
    }

    /* Register profiles */
    e_ret = evision_api_awb_init_profiles(pIspAWBestimator, (double) IQParamConfig->AWBAlgo.referenceColorTemp[0],
                                          (double) IQParamConfig->AWBAlgo.referenceColorTemp[profNb - 1], (uint16_t)profNb,
                                          colorTempThresholds, awbProfiles);
    if (e_ret != EVISION_RET_SUCCESS)
    {
      return ISP_ERR_ALGO;
    }

    /* Configure algo */
    pIspAWBestimator->hyper_params.speed_p_min = 1.35;
    pIspAWBestimator->hyper_params.speed_p_max = (profNb < 4)? 1.8 : 2.0;
    pIspAWBestimator->hyper_params.gm_tolerance = 1;
    pIspAWBestimator->hyper_params.conv_criterion = 3;

    /* Ask for stats */
    ret = ISP_SVC_Stats_GetNext(hIsp, &ISP_Algo_AWB_StatCb, pAlgo, &stats, ISP_STAT_LOC_DOWN,
                                ISP_STAT_TYPE_AVG, ALGO_ISP_LATENCY + ALGO_AWB_ADDITIONAL_LATENCY);
    if (ret != ISP_OK)
    {
      return ret;
    }

    /* Wait for stats to be ready */
    algo->state = ISP_ALGO_STATE_WAITING_STAT;
    break;

  case ISP_ALGO_STATE_NEED_STAT:
    ret = ISP_SVC_Stats_GetNext(hIsp, &ISP_Algo_AWB_StatCb, pAlgo, &stats, ISP_STAT_LOC_DOWN,
                                ISP_STAT_TYPE_AVG, ALGO_ISP_LATENCY + ALGO_AWB_ADDITIONAL_LATENCY);
    if (ret != ISP_OK)
    {
      return ret;
    }

    /* Wait for stats to be ready */
    algo->state = ISP_ALGO_STATE_WAITING_STAT;
    break;

  case ISP_ALGO_STATE_WAITING_STAT:
    /* Do nothing */
    break;

  case ISP_ALGO_STATE_STAT_READY:
    ISP_Algo_GetUpStat(hIsp, &stats);

    for (i = 0; i < 3; i++)
    {
      if ((abs(stats.up.averageR - (int32_t)statsHistory[i][0]) > 2)
          || (abs(stats.up.averageG - (int32_t)statsHistory[i][1]) > 2)
          || (abs(stats.up.averageB - (int32_t)statsHistory[i][2]) > 2))
      {
        stat_has_changed = true;
      }
    }

    if (skip_stat_check_count || stat_has_changed || reconfigureRequest)
    {
        statsHistory[2][0] = stats.up.averageR;
        statsHistory[2][1] = stats.up.averageG;
        statsHistory[2][2] = stats.up.averageB;

        /* Get stats after color conversion */
        ISP_Algo_ApplyCConv(hIsp, stats.down.averageR, stats.down.averageG, stats.down.averageB, &ccAvgR, &ccAvgG, &ccAvgB);

        /* Apply gamma */
        meas[0] = ISP_Algo_ApplyGammaInverse(hIsp, ccAvgR);
        meas[1] = ISP_Algo_ApplyGammaInverse(hIsp, ccAvgG);
        meas[2] = ISP_Algo_ApplyGammaInverse(hIsp, ccAvgB);

        /* Run algo to estimate gain and color conversion to apply */
        e_ret = evision_api_awb_run_average(pIspAWBestimator, NULL, 1, meas);
        if (e_ret == EVISION_RET_SUCCESS)
        {
#ifdef ALGO_AWB_DBG_LOGS
          static int nb_meas, nb_changes;
          static int nb_colortemp_change[ISP_AWB_COLORTEMP_REF];

          nb_meas++;
          if (pIspAWBestimator->out_temp != currentColorTemp)
            nb_changes++;
          for (int i = 0; i < ISP_AWB_COLORTEMP_REF; i++) {
            if (pIspAWBestimator->out_temp == IQParamConfig->AWBAlgo.referenceColorTemp[i])
            {
              nb_colortemp_change[i]++;
              continue;
            }
          }

          if (nb_meas == 100)
          {
            printf("Last 100 measures:\r\n");
            for (int i = 0; i < ISP_AWB_COLORTEMP_REF; i++) {
              printf("\t%"PRIu32": %"PRIu16"\r\n",
                     IQParamConfig->AWBAlgo.referenceColorTemp[i],
                     nb_colortemp_change[i]);
            }
            printf("\nChanges: %d\r\n", nb_changes);

            nb_meas = 0;
            nb_changes = 0;
            for (int i = 0; i < ISP_AWB_COLORTEMP_REF; i++) {
              nb_colortemp_change[i] = 0;
            }
          }
#endif
          if (pIspAWBestimator->out_temp != currentColorTemp || reconfigureRequest == true)
          {
#ifdef ALGO_AWB_DBG_LOGS
            printf("Color temperature = %"PRIu32"\r\n", (uint32_t) pIspAWBestimator->out_temp);
#endif
            if ((pIspAWBestimator->out_temp == colorTempHistory[1]) && (reconfigureRequest != true))
            {
              skip_stat_check_count = 0; //oscillation detected
            }
            else
            {
              if (skip_stat_check_count <= ALGO_AWB_STAT_CHECK_SKIP_AFTER_CT_ESTIMATION) skip_stat_check_count = ALGO_AWB_STAT_CHECK_SKIP_AFTER_CT_ESTIMATION;

              /* Store meta data */
              Meta.colorTemp = (uint32_t) pIspAWBestimator->out_temp;

              /* Find the index profile for this referenceColorTemp */
              for (profId = 0; profId < ISP_AWB_COLORTEMP_REF; profId++)
              {
                if (pIspAWBestimator->out_temp == IQParamConfig->AWBAlgo.referenceColorTemp[profId])
                  break;
              }

              if (profId == ISP_AWB_COLORTEMP_REF)
              {
                /* Unknown profile */
                ret  = ISP_ERR_WB_COLORTEMP;
              }
              else
              {
                /* Apply Color Conversion */
                ColorConvConfig.enable = 1;
                memcpy(ColorConvConfig.coeff, IQParamConfig->AWBAlgo.coeff[profId], sizeof(ColorConvConfig.coeff));
                ret = ISP_SVC_ISP_SetColorConv(hIsp, &ColorConvConfig);

                /* Apply gain */
                if (ret == ISP_OK)
                {
                  ISPGainConfig.enable = 1;
                  ISPGainConfig.ispGainR = IQParamConfig->AWBAlgo.ispGainR[profId];
                  ISPGainConfig.ispGainG = IQParamConfig->AWBAlgo.ispGainG[profId];
                  ISPGainConfig.ispGainB = IQParamConfig->AWBAlgo.ispGainB[profId];
                  ret = ISP_SVC_ISP_SetGain(hIsp, &ISPGainConfig);
                  if (ret == ISP_OK)
                  {
                    currentColorTemp = (uint32_t) pIspAWBestimator->out_temp ;
                    current_awb_profId = profId;
                  }
                }
              }
            }
          }
        }
        else
        {
          ret = ISP_ERR_ALGO;
        }
    }

    /* Reset reconfigureRequest */
    reconfigureRequest = false;

    /* Decrease counter to limit the number of estimations before reaching convergence */
    if (skip_stat_check_count > 0) skip_stat_check_count--;

    /* Store history to be able to detect variations*/
    statsHistory[1][0] = statsHistory[0][0];
    statsHistory[1][1] = statsHistory[0][1];
    statsHistory[1][2] = statsHistory[0][2];
    statsHistory[0][0] = stats.up.averageR;
    statsHistory[0][1] = stats.up.averageG;
    statsHistory[0][2] = stats.up.averageB;
    colorTempHistory[1] = colorTempHistory[0];
    colorTempHistory[0] = currentColorTemp;

    /* Ask for stats */
    ret_stat = ISP_SVC_Stats_GetNext(hIsp, &ISP_Algo_AWB_StatCb, pAlgo, &stats, ISP_STAT_LOC_DOWN,
                                     ISP_STAT_TYPE_AVG, ALGO_ISP_LATENCY + ALGO_AWB_ADDITIONAL_LATENCY);
    ret = (ret != ISP_OK) ? ret : ret_stat;

    /* Wait for stats to be ready */
    algo->state = ISP_ALGO_STATE_WAITING_STAT;
    break;

  default:
    printf("WARNING: Unknown AWB algo state\r\n");
    /* Reset state to ISP_ALGO_STATE_INIT */
    algo->state = ISP_ALGO_STATE_INIT;
    break;
  }

  return ret;
}
#endif /* ISP_MW_SW_AWB_ALGO_SUPPORT */

#ifdef ISP_MW_TUNING_TOOL_SUPPORT
/**
  * @brief  ISP_Algo_SensorDelay_Init
  *         Initialize the SensorDelay algorithm
  * @param  hIsp:  ISP device handle. To cast in (ISP_HandleTypeDef *).
  * @param  pAlgo: ISP algorithm handle. To cast in (ISP_AlgoTypeDef *).
  * @retval operation result
  */
ISP_StatusTypeDef ISP_Algo_SensorDelay_Init(void *hIsp, void *pAlgo)
{
  (void)hIsp; /* unused */

  ((ISP_AlgoTypeDef *)pAlgo)->state = ISP_ALGO_STATE_INIT;

  return ISP_OK;
}

/**
  * @brief  ISP_Algo_SensorDelay_DeInit
  *         Deinitialize the SensorDelay algorithm
  * @param  hIsp:  ISP device handle. To cast in (ISP_HandleTypeDef *).
  * @param  pAlgo: ISP algorithm handle. To cast in (ISP_AlgoTypeDef *).
  * @retval operation result
  */
ISP_StatusTypeDef ISP_Algo_SensorDelay_DeInit(void *hIsp, void *pAlgo)
{
  (void)hIsp; /* unused */

  ((ISP_AlgoTypeDef *)pAlgo)->state = ISP_ALGO_STATE_INIT;

  return ISP_OK;
}

/**
  * @brief  ISP_Algo_SensorDelay_StatCb
  *         Callback informing that statistics are available
  * @param  pAlgo: ISP algorithm handle.
  * @retval operation result
  */
ISP_StatusTypeDef ISP_Algo_SensorDelay_StatCb(ISP_AlgoTypeDef *pAlgo)
{
  /* Update State */
  pAlgo->state = ISP_ALGO_STATE_STAT_READY;

  return ISP_OK;
}

/**
  * @brief  ISP_Algo_SensorDelay_Process
  *         Process the SensorDelay algorithm. Change the sensor gain and exposure and
  *         measure the number of frames it takes to have the frame updated.
  * @param  hIsp:  ISP device handle. To cast in (ISP_HandleTypeDef *).
  * @param  pAlgo: ISP algorithm handle. To cast in (ISP_AlgoTypeDef *).
  * @retval operation result
  */
ISP_StatusTypeDef ISP_Algo_SensorDelay_Process(void *hIsp, void *pAlgo)
{
  ISP_AlgoTypeDef *algo = (ISP_AlgoTypeDef *)pAlgo;
  ISP_SensorInfoTypeDef *pSensorInfo;
  ISP_IQParamTypeDef *IQParamConfig;
  ISP_StatusTypeDef ret = ISP_OK;
  uint8_t sensorDelay;
  int32_t avgL;
  uint32_t i;
  static int32_t refL, delay, delays[ALGO_DELAY_NB_CONFIG - 1], configId;
  static ISP_SensorExposureTypeDef configExposure[ALGO_DELAY_NB_CONFIG];
  static ISP_SensorGainTypeDef configGain[ALGO_DELAY_NB_CONFIG];
  static ISP_SensorExposureTypeDef prevExposureConfig;
  static ISP_SensorGainTypeDef prevGainConfig;
  static uint8_t prevAECStatus, prevAWBStatus;
  static ISP_SVC_StatStateTypeDef stats;

  if (ISP_SVC_Misc_SensorDelayMeasureIsRunning() == false)
  {
    return ISP_OK;
  }

  IQParamConfig = ISP_SVC_IQParam_Get(hIsp);

  switch(algo->state)
  {
  case ISP_ALGO_STATE_INIT:
    /* Get current AEC and AWB algo status and sensor configuration */
    prevAECStatus = IQParamConfig->AECAlgo.enable;
    prevAWBStatus = IQParamConfig->AWBAlgo.enable;
    ret = ISP_SVC_Sensor_GetGain(hIsp, &prevGainConfig);
    if (ret != ISP_OK)
    {
      return ret;
    }
    ret = ISP_SVC_Sensor_GetExposure(hIsp, &prevExposureConfig);
    if (ret != ISP_OK)
    {
      return ret;
    }

    /* Disable AEC and AWB algo to avoid interferences */
    IQParamConfig->AECAlgo.enable = false;
    IQParamConfig->AWBAlgo.enable = false;

    /* Initialize the sensor test configurations */
    pSensorInfo = &((ISP_HandleTypeDef*) hIsp)->sensorInfo;
    /* Config  0: Exposure =   0%  -  Gain =  0%
     *    .................. +20% ...........
     * Config  5: Exposure = 100%  -  Gain =  0%
     *    ................................. +10%
     * Config 11: Exposure = 100%  -  Gain = 60%
     */
    for (i = 0; i < 6; i++)
    {
      configExposure[i].exposure = i ? (pSensorInfo->exposure_max * 20 * i) / 100 : pSensorInfo->exposure_min;
      configGain[i].gain = pSensorInfo->gain_min;
      configExposure[i + 6].exposure = pSensorInfo->exposure_max;
      configGain[i + 6].gain = (pSensorInfo->gain_max * 10 * (i + 1)) / 100;
    }

    /* Apply first test configuration */
    configId = 0;
    ret = ISP_SVC_Sensor_SetGain(hIsp, &configGain[configId]);
    if (ret != ISP_OK)
    {
      return ret;
    }
    ret = ISP_SVC_Sensor_SetExposure(hIsp, &configExposure[configId]);
    if (ret != ISP_OK)
    {
      return ret;
    }

    /* Ask for stats lately (just to define a test starting point) */
    delay = 0;
    refL = 0;
    memset(delays, 0, sizeof(delays));
    ret = ISP_SVC_Stats_GetNext(hIsp, &ISP_Algo_SensorDelay_StatCb, pAlgo, &stats, ISP_STAT_LOC_DOWN, ISP_STAT_TYPE_AVG, ALGO_DELAY_MAX);
    if (ret != ISP_OK)
    {
      return ret;
    }

    /* Wait for stats to be ready */
    algo->state = ISP_ALGO_STATE_WAITING_STAT;
    break;

  case ISP_ALGO_STATE_NEED_STAT:
    break;

  case ISP_ALGO_STATE_WAITING_STAT:
    /* Do nothing */
    break;

  case ISP_ALGO_STATE_STAT_READY:
    avgL = (int32_t)stats.down.averageL;
    if (configId > 0)
    {
      /* New stat available, check if Luminance has changed */
      delay++;

      if (abs(avgL- refL) <= ALGO_DELAY_L_MARGIN && delay != ALGO_DELAY_MAX)
      {
        /* No change, wait for next frame */
        ret = ISP_SVC_Stats_GetNext(hIsp, &ISP_Algo_SensorDelay_StatCb, pAlgo, &stats, ISP_STAT_LOC_DOWN, ISP_STAT_TYPE_AVG, 1);
        algo->state = ISP_ALGO_STATE_WAITING_STAT;
        return ret;
      }

      /* Luminance was updated since we applied a new sensor configuration : store the result for this test.
       * Reaching ALGO_DELAY_MAX happens when we have a totally black or white frame. The measure shall be considered as invalid. */
      delays[configId - 1] = delay;
    }

    /* New delay measure available */
    if (++configId != ALGO_DELAY_NB_CONFIG)
    {
      /* Apply new sensor test configuration  */
      ret = ISP_SVC_Sensor_SetGain(hIsp, &configGain[configId]);
      if (ret != ISP_OK)
      {
        return ret;
      }
      ret = ISP_SVC_Sensor_SetExposure(hIsp, &configExposure[configId]);
      if (ret != ISP_OK)
      {
        return ret;
      }

      /* Ask for stats at next frame */
      delay = 0;
      refL = avgL;
      ret = ISP_SVC_Stats_GetNext(hIsp, &ISP_Algo_SensorDelay_StatCb, pAlgo, &stats, ISP_STAT_LOC_DOWN, ISP_STAT_TYPE_AVG, 1);
      if (ret != ISP_OK)
      {
        return ret;
      }

      /* Wait for stats to be ready */
      algo->state = ISP_ALGO_STATE_WAITING_STAT;
    }
    else
    {
      /* All the configurations have been tested, finalize the test procedure */
      /* Find the max valid delay (ALGO_DELAY_MAX being considered as invalid) */
      sensorDelay = 0;
      for (i = 0; i < ALGO_DELAY_NB_CONFIG - 1; i++)
      {
        if ((delays[i] != ALGO_DELAY_MAX) && (delays[i] > sensorDelay))
        {
          sensorDelay = (uint8_t)delays[i];
        }
      }

      /* Restore initial AEC, AWB and sensor states */
      IQParamConfig->AECAlgo.enable = prevAECStatus;
      IQParamConfig->AWBAlgo.enable = prevAWBStatus;
      ret = ISP_SVC_Sensor_SetGain(hIsp, &prevGainConfig);
      if (ret != ISP_OK)
      {
        return ret;
      }
      ret = ISP_SVC_Sensor_SetExposure(hIsp, &prevExposureConfig);
      if (ret != ISP_OK)
      {
        return ret;
      }

      /* Apply the measure (if valid) and send answer to the remote IQTune */
      if (sensorDelay)
      {
        IQParamConfig->sensorDelay.delay = sensorDelay;
      }
      ret = ISP_SVC_Misc_SendSensorDelayMeasure(hIsp, (ISP_SensorDelayTypeDef *)&sensorDelay);

      /* Stop delay algo */
      ISP_SVC_Misc_SensorDelayMeasureStop();

      algo->state = ISP_ALGO_STATE_INIT;
    }
    break;

  default:
    printf("WARNING: Unknown Sensor Delay algo state\r\n");
    /* Reset state to ISP_ALGO_STATE_INIT */
    algo->state = ISP_ALGO_STATE_INIT;
    break;
  }

  return ret;
}
#endif /* ISP_MW_TUNING_TOOL_SUPPORT */

/* Exported functions --------------------------------------------------------*/
/**
  * @brief  ISP_Algo_Init
  *         Register and initialize all the algorithms
  * @param  hIsp: ISP device handle
  * @retval operation result
  */
ISP_StatusTypeDef ISP_Algo_Init(ISP_HandleTypeDef *hIsp)
{
  ISP_AlgoTypeDef *algo;
  ISP_StatusTypeDef ret;
  uint8_t i;

  hIsp->algorithm = ISP_Algo_List;

  for (i = 0; i < sizeof(ISP_Algo_List) / sizeof(*ISP_Algo_List); i++)
  {
    algo = hIsp->algorithm[i];
    if ((algo != NULL) && (algo->Init != NULL))
    {
      ret = algo->Init((void*)hIsp, (void*)algo);
      if (ret != ISP_OK)
      {
        return ret;
      }
    }
  }

  return ISP_OK;
}

/**
  * @brief  ISP_Algo_DeInit
  *         Deinitialize all the algorithms
  * @param  hIsp: ISP device handle
  * @retval operation result
  */
ISP_StatusTypeDef ISP_Algo_DeInit(ISP_HandleTypeDef *hIsp)
{
  ISP_AlgoTypeDef *algo;
  ISP_StatusTypeDef ret;
  uint8_t i;

  for (i = 0; i < sizeof(ISP_Algo_List) / sizeof(*ISP_Algo_List); i++)
  {
    algo = hIsp->algorithm[i];
    if ((algo != NULL) && (algo->DeInit != NULL))
    {
      ret = algo->DeInit((void*)hIsp, (void*)algo);
      if (ret != ISP_OK)
      {
        return ret;
      }
    }
  }

  return ISP_OK;
}

/**
  * @brief  ISP_Algo_Process
  *         Process all the algorithms
  * @param  hIsp: ISP device handle
  * @retval operation result
  */
ISP_StatusTypeDef ISP_Algo_Process(ISP_HandleTypeDef *hIsp)
{
  ISP_AlgoTypeDef *algo;
  ISP_StatusTypeDef ret;
  uint8_t i;

  for (i = 0; i < sizeof(ISP_Algo_List) / sizeof(*ISP_Algo_List); i++)
  {
    algo = hIsp->algorithm[i];
    if ((algo != NULL) && (algo->Process != NULL))
    {
#ifdef ALGO_PERF_DBG_LOGS
      uint32_t tickstart = HAL_GetTick();
#endif
      ret = algo->Process((void*)hIsp, (void*)algo);
      if (ret != ISP_OK)
      {
        return ret;
      }
#ifdef ALGO_PERF_DBG_LOGS
      algo->perf_meas[iter] = HAL_GetTick() - tickstart;
      algo->iter++;
      if (algo->iter == NB_PERF_MEASURES) {
        uint32_t sum = 0;
        for(uint32_t j = 0; j < NB_PERF_MEASURES; j++)
        {
          sum += algo->perf_meas[j];
        }
        switch (algo->id)
        {
          case ISP_ALGO_ID_BADPIXEL:
            printf("BadPixel algo      ");
            break;
#ifdef ISP_MW_SW_AEC_ALGO_SUPPORT
          case ISP_ALGO_ID_AEC:
            printf("AEC algo           ");
            break;
#endif /* ISP_MW_SW_AEC_ALGO_SUPPORT */
#ifdef ISP_MW_SW_AWB_ALGO_SUPPORT
          case ISP_ALGO_ID_AWB:
            printf("AWB algo           ");
            break;
#endif /* ISP_MW_SW_AWB_ALGO_SUPPORT */
        }
        uint32_t meas = sum / NB_PERF_MEASURES;
        if (meas == 0)
        {
          printf(" <1 ms\r\n");
        }
        else
        {
          printf(" %"PRIu32" ms\r\n", meas);
        }
        algo->iter = 0;
      }
#endif
    }
  }

  return ISP_OK;
}

