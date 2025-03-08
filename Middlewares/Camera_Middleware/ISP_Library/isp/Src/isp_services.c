/**
 ******************************************************************************
 * @file    isp_services.c
 * @author  AIS Application Team
 * @brief   Services of the ISP middleware
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
#include "isp_services.h"

/* Private types -------------------------------------------------------------*/
typedef enum {
  ISP_STAT_CFG_UP_AVG = 0,      /* Configure @Up   for average    */
  ISP_STAT_CFG_UP_BINS_0_2,     /* Configure @Up   for bins[0:2]  */
  ISP_STAT_CFG_UP_BINS_3_5,     /* Configure @Up   for bins[3:5]  */
  ISP_STAT_CFG_UP_BINS_6_8,     /* Configure @Up   for bins[6:8]  */
  ISP_STAT_CFG_UP_BINS_9_11,    /* Configure @Up   for bins[9:11] */
  ISP_STAT_CFG_DOWN_AVG,        /* Configure @Down for average    */
  ISP_STAT_CFG_DOWN_BINS_0_2,   /* Configure @Down for bins[0:2]  */
  ISP_STAT_CFG_DOWN_BINS_3_5,   /* Configure @Down for bins[3:5]  */
  ISP_STAT_CFG_DOWN_BINS_6_8,   /* Configure @Down for bins[6:8]  */
  ISP_STAT_CFG_DOWN_BINS_9_11,  /* Configure @Down for bins[9:11] */
  ISP_STAT_CFG_LAST = ISP_STAT_CFG_DOWN_BINS_9_11,
  ISP_STAT_CFG_CYCLE_SIZE,
} ISP_SVC_StatEngineStage;

typedef enum {
  ISP_RED,
  ISP_GREEN,
  ISP_BLUE,
} ISP_SVC_Component;

typedef struct {
  ISP_stat_ready_cb callback;           /* Callback to inform that stats are ready */
  ISP_AlgoTypeDef *pAlgo;               /* Callback context parameter */
  ISP_SVC_StatStateTypeDef *pStats;     /* Output statistics */
  uint32_t refFrameId;                  /* Frame reference for which stats are requested */
  ISP_SVC_StatLocation location;        /* Location where stats are requested */
  ISP_SVC_StatType type;                /* Type of requested stats */
} ISP_SVC_StatRegisteredClient;

#define ISP_SVC_STAT_MAX_CB       (5U)
typedef struct {
  ISP_SVC_StatEngineStage stage;        /* Internal processing stage */
  ISP_SVC_StatStateTypeDef last;        /* Last available statistics */
  ISP_SVC_StatStateTypeDef ongoing;     /* Statistics being updated */
  ISP_SVC_StatRegisteredClient client[ISP_SVC_STAT_MAX_CB]; /* Client waiting for stats */
  ISP_SVC_StatType upRequest;           /* Type of statistics request at Up location */
  ISP_SVC_StatType downRequest;         /* Type of statistics request at Down location */
  uint32_t requestAllCounter;           /* Counter for the temporary "request all stats" mode */
} ISP_SVC_StatEngineTypeDef;

/* Private constants ---------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static void To_Shift_Multiplier(uint32_t Factor, uint8_t *pShift, uint8_t *pMultiplier);
static uint32_t From_Shift_Multiplier(uint8_t Shift, uint8_t Multiplier);
static int16_t To_CConv_Reg(int32_t Coeff);
static int32_t From_CConv_Reg(int16_t Reg);

/* Private variables ---------------------------------------------------------*/
static uint32_t ISP_ManualWBRefColorTemp = 0;
static ISP_DecimationTypeDef ISP_DecimationValue = {ISP_DECIM_FACTOR_1};
static ISP_IQParamTypeDef ISP_IQParamCache;
static ISP_SVC_StatEngineTypeDef ISP_SVC_StatEngine;

static const uint32_t avgRGBUp[] = {
    DCMIPP_STAT_EXT_SOURCE_PRE_BLKLVL_R, DCMIPP_STAT_EXT_SOURCE_PRE_BLKLVL_G, DCMIPP_STAT_EXT_SOURCE_PRE_BLKLVL_B
};

static const uint32_t avgRGBDown[] = {
    DCMIPP_STAT_EXT_SOURCE_POST_DEMOS_R, DCMIPP_STAT_EXT_SOURCE_POST_DEMOS_G, DCMIPP_STAT_EXT_SOURCE_POST_DEMOS_B
};

static const DCMIPP_StatisticExtractionConfTypeDef statConfUpBins_0_2 = {
    .Mode = DCMIPP_STAT_EXT_MODE_BINS,
    .Source = DCMIPP_STAT_EXT_SOURCE_PRE_BLKLVL_L,
    .Bins = DCMIPP_STAT_EXT_BINS_MODE_LOWER_BINS
};

static const DCMIPP_StatisticExtractionConfTypeDef statConfUpBins_3_5 = {
    .Mode = DCMIPP_STAT_EXT_MODE_BINS,
    .Source = DCMIPP_STAT_EXT_SOURCE_PRE_BLKLVL_L,
    .Bins = DCMIPP_STAT_EXT_BINS_MODE_LOWMID_BINS
};

static const DCMIPP_StatisticExtractionConfTypeDef statConfUpBins_6_8 = {
    .Mode = DCMIPP_STAT_EXT_MODE_BINS,
    .Source = DCMIPP_STAT_EXT_SOURCE_PRE_BLKLVL_L,
    .Bins = DCMIPP_STAT_EXT_BINS_MODE_UPMID_BINS
};

static const DCMIPP_StatisticExtractionConfTypeDef statConfUpBins_9_11 = {
    .Mode = DCMIPP_STAT_EXT_MODE_BINS,
    .Source = DCMIPP_STAT_EXT_SOURCE_PRE_BLKLVL_L,
    .Bins = DCMIPP_STAT_EXT_BINS_MODE_UP_BINS
};

static const DCMIPP_StatisticExtractionConfTypeDef statConfDownBins_0_2 = {
    .Mode = DCMIPP_STAT_EXT_MODE_BINS,
    .Source = DCMIPP_STAT_EXT_SOURCE_POST_DEMOS_L,
    .Bins = DCMIPP_STAT_EXT_BINS_MODE_LOWER_BINS
};

static const DCMIPP_StatisticExtractionConfTypeDef statConfDownBins_3_5 = {
    .Mode = DCMIPP_STAT_EXT_MODE_BINS,
    .Source = DCMIPP_STAT_EXT_SOURCE_POST_DEMOS_L,
    .Bins = DCMIPP_STAT_EXT_BINS_MODE_LOWMID_BINS
};

static const DCMIPP_StatisticExtractionConfTypeDef statConfDownBins_6_8 = {
    .Mode = DCMIPP_STAT_EXT_MODE_BINS,
    .Source = DCMIPP_STAT_EXT_SOURCE_POST_DEMOS_L,
    .Bins = DCMIPP_STAT_EXT_BINS_MODE_UPMID_BINS
};

static const DCMIPP_StatisticExtractionConfTypeDef statConfDownBins_9_11 = {
    .Mode = DCMIPP_STAT_EXT_MODE_BINS,
    .Source = DCMIPP_STAT_EXT_SOURCE_POST_DEMOS_L,
    .Bins = DCMIPP_STAT_EXT_BINS_MODE_UP_BINS
};

/* Exported variables --------------------------------------------------------*/

/* Private functions ---------------------------------------------------------*/
static void To_Shift_Multiplier(uint32_t Factor, uint8_t *pShift, uint8_t *pMultiplier)
{
  /* Convert Factor (Unit = 100000000 for "x1.0") to Multiplier (where 128 means "x1.0") */
  uint64_t Val = Factor;
  Val = (Val * 128) / ISP_GAIN_PRECISION_FACTOR;

  /* Get Shift + Multiplier where Multiplier < 256 */
  *pShift = 0;
  while (Val >= 256)
  {
    Val /= 2;
    (*pShift)++;
  }

  *pMultiplier = Val;
}

static uint32_t From_Shift_Multiplier(uint8_t Shift, uint8_t Multiplier)
{
  /* Convert Shift + Multiplier to Factor (Unit = 100000000 for "x1.0") */
  uint64_t Val = (1 << Shift);
  Val = (Val * Multiplier * ISP_GAIN_PRECISION_FACTOR) / 128;
  return (uint32_t) Val;
}

static int16_t To_CConv_Reg(int32_t Coeff)
{
  /* Convert Coefficient (Unit = 100000000 for "x1.0") to register format */
  int64_t Val = Coeff;

  Val = (Val * 256) / ISP_CCM_PRECISION_FACTOR;

  return (int16_t) Val;
}

static int32_t From_CConv_Reg(int16_t Reg)
{
  /* Convert from register format to Coefficient (Unit = 100000000 for "x1.0") */
  int64_t Val = Reg;

  Val = (Val * ISP_CCM_PRECISION_FACTOR) / 256;

  return (int32_t) Val;
}

static uint8_t GetAvgStats(ISP_HandleTypeDef *hIsp, ISP_SVC_StatLocation location, ISP_SVC_Component component, uint32_t accu)
{
  uint32_t nb_comp_pix, comp_divider;

  /* Number of pixels computed from Stat Area and considering decimation */
  nb_comp_pix = hIsp->statArea.XSize * hIsp->statArea.YSize;
  nb_comp_pix /= ISP_DecimationValue.factor * ISP_DecimationValue.factor;

  if (location == ISP_STAT_LOC_DOWN)
  {
    /* RGB format after demosaicing : 1 component per pixel */
    comp_divider = 1;
  }
  else
  {
    /* Only raw bayer sensor expected */
    /* raw bayer: RGB component not present for all pixels */
    comp_divider = (component == ISP_GREEN) ? 2 : 4;
  }

  /* Number of pixels per component */
  nb_comp_pix /= comp_divider;

  /* Compute average (rounding to closest integer) */
  if (nb_comp_pix == 0)
  {
    return 0;
  }

  return ((accu * 256) + (nb_comp_pix / 2)) / nb_comp_pix;
}

static void ReadStatHistogram(ISP_HandleTypeDef *hIsp, uint32_t *histogram)
{
  for (uint32_t i = DCMIPP_STATEXT_MODULE1; i <= DCMIPP_STATEXT_MODULE3; i++)
  {
    HAL_DCMIPP_PIPE_GetISPAccumulatedStatisticsCounter(hIsp->hDcmipp, DCMIPP_PIPE1, i, &(histogram[i - DCMIPP_STATEXT_MODULE1]));
  }
}

static void SetStatConfig(DCMIPP_StatisticExtractionConfTypeDef *statConf, const DCMIPP_StatisticExtractionConfTypeDef *refConfig)
{
  for (int i = 0; i < 3; i++)
  {
    statConf[i] = *refConfig;
  }
}

static ISP_SVC_StatEngineStage GetNextStatStage(ISP_SVC_StatEngineStage current)
{
  ISP_SVC_StatEngineStage next = ISP_STAT_CFG_LAST;

  /* Special mode for IQ tuning tool asking for all stats : go the the next step, no skip */
  if ((ISP_SVC_StatEngine.upRequest & ISP_STAT_TYPE_ALL_TMP) ||
      (ISP_SVC_StatEngine.downRequest & ISP_STAT_TYPE_ALL_TMP))
  {
    next = (ISP_SVC_StatEngineStage) ((current < ISP_STAT_CFG_LAST) ? current + 1 : ISP_STAT_CFG_UP_AVG);
    return next;
  }

  /* Follow the below stage cycle, skipping steps where stats are not requested:
   * - ISP_STAT_CFG_UP_AVG
   * - ISP_STAT_CFG_UP_BINS_0_2 + BINS_3_5 + BINS_6_8 + BINS_9_11
   * - ISP_STAT_CFG_DOWN_AVG
   * - ISP_STAT_CFG_DOWN_BINS_0_2 + BINS_3_5 + BINS_6_8 + BINS_9_11
  */
  switch (current)
  {
  case ISP_STAT_CFG_UP_AVG:
    /* Try Up Bins */
    if (ISP_SVC_StatEngine.upRequest & ISP_STAT_TYPE_BINS)
    {
      next = ISP_STAT_CFG_UP_BINS_0_2;
    }
    /* Skip Up Bins : try Down Avg */
    else if (ISP_SVC_StatEngine.downRequest &  ISP_STAT_TYPE_AVG)
    {
      next = ISP_STAT_CFG_DOWN_AVG;
    }
    /* Skip Down Avg : try Down Bins */
    else if (ISP_SVC_StatEngine.downRequest & ISP_STAT_TYPE_BINS)
    {
      next = ISP_STAT_CFG_DOWN_BINS_0_2;
    }
    /* Skip Down Bins : try Up Avg */
    else if (ISP_SVC_StatEngine.upRequest & ISP_STAT_TYPE_AVG)
    {
      next = ISP_STAT_CFG_UP_AVG;
    }
    break;

  case ISP_STAT_CFG_UP_BINS_9_11:
    /* Try Down Avg */
    if (ISP_SVC_StatEngine.downRequest & ISP_STAT_TYPE_AVG)
    {
      next = ISP_STAT_CFG_DOWN_AVG;
    }
    /* Skip Down Avg : try Down Bins */
    else if (ISP_SVC_StatEngine.downRequest & ISP_STAT_TYPE_BINS)
    {
      next = ISP_STAT_CFG_DOWN_BINS_0_2;
    }
    /* Skip Down Bins : try Up Avg */
    else if (ISP_SVC_StatEngine.upRequest & ISP_STAT_TYPE_AVG)
    {
      next = ISP_STAT_CFG_UP_AVG;
    }
    /* Skip Up Avg : try on Up Bins */
    else if (ISP_SVC_StatEngine.upRequest & ISP_STAT_TYPE_BINS)
    {
      next = ISP_STAT_CFG_UP_BINS_0_2;
    }
    break;

  case ISP_STAT_CFG_DOWN_AVG:
    /* Try Down Bins */
    if (ISP_SVC_StatEngine.downRequest & ISP_STAT_TYPE_BINS)
    {
      next = ISP_STAT_CFG_DOWN_BINS_0_2;
    }
    /* Skip Down Bins : try Up Avg */
    else if (ISP_SVC_StatEngine.upRequest & ISP_STAT_TYPE_AVG)
    {
      next = ISP_STAT_CFG_UP_AVG;
    }
    /* Skip Up Avg : try Up Bins */
    else if (ISP_SVC_StatEngine.upRequest & ISP_STAT_TYPE_BINS)
    {
      next = ISP_STAT_CFG_UP_BINS_0_2;
    }
    /* Skip Up Bins : try Down Avg */
    else if (ISP_SVC_StatEngine.downRequest & ISP_STAT_TYPE_AVG)
    {
      next = ISP_STAT_CFG_DOWN_AVG;
    }
    break;

  case ISP_STAT_CFG_DOWN_BINS_9_11:
    /* Try Up Avg */
    if (ISP_SVC_StatEngine.upRequest & ISP_STAT_TYPE_AVG)
    {
      next = ISP_STAT_CFG_UP_AVG;
    }
    /* Skip Up Avg : try Up Bins */
    else if (ISP_SVC_StatEngine.upRequest & ISP_STAT_TYPE_BINS)
    {
      next = ISP_STAT_CFG_UP_BINS_0_2;
    }
    /* Skip Up Bins : try Down Avg */
    else if (ISP_SVC_StatEngine.downRequest & ISP_STAT_TYPE_AVG)
    {
      next = ISP_STAT_CFG_DOWN_AVG;
    }
    /* Skip Down Avg : try Down Bins */
    else if (ISP_SVC_StatEngine.downRequest & ISP_STAT_TYPE_BINS)
    {
      next = ISP_STAT_CFG_DOWN_BINS_0_2;
    }
    break;

  default:
    /* In the middle of the bins measurement: continue with the next bins part */
    next = (ISP_SVC_StatEngineStage) (current + 1);
    break;
  }

  return next;
}

static ISP_SVC_StatEngineStage GetStatCycleStart(ISP_SVC_StatLocation location)
{
  ISP_SVC_StatEngineStage stage;

  if (location == ISP_STAT_LOC_UP)
  {
    if ((ISP_SVC_StatEngine.upRequest & ISP_STAT_TYPE_AVG) ||
        (ISP_SVC_StatEngine.upRequest & ISP_STAT_TYPE_ALL_TMP))
    {
      /* Stat up cycle starts with AVG measurement */
      stage = ISP_STAT_CFG_UP_AVG;
    }
    else
    {
      /* Stat up cycle starts with 1st BIN measurement */
      stage = ISP_STAT_CFG_UP_BINS_0_2;
    }
  }
  else
  {
    if ((ISP_SVC_StatEngine.downRequest & ISP_STAT_TYPE_AVG) ||
        (ISP_SVC_StatEngine.downRequest & ISP_STAT_TYPE_ALL_TMP))
    {
      /* Stat down cycle starts with AVG measurement */
      stage = ISP_STAT_CFG_DOWN_AVG;
    }
    else
    {
      /* Stat down cycle starts with 1st BIN measurement */
      stage = ISP_STAT_CFG_DOWN_BINS_0_2;
    }
  }
  return stage;
}

static ISP_SVC_StatEngineStage GetStatCycleEnd(ISP_SVC_StatLocation location)
{
  ISP_SVC_StatEngineStage stage;

  if (location == ISP_STAT_LOC_UP)
  {
    if ((ISP_SVC_StatEngine.upRequest & ISP_STAT_TYPE_BINS) ||
        (ISP_SVC_StatEngine.upRequest & ISP_STAT_TYPE_ALL_TMP))
    {
      /* Stat up cycle ends with last BINS measurement */
      stage = ISP_STAT_CFG_UP_BINS_9_11;
    }
    else
    {
      /* Stat up cycle ends with AVG measurement */
      stage = ISP_STAT_CFG_UP_AVG;
    }
  }
  else
  {
    if ((ISP_SVC_StatEngine.downRequest & ISP_STAT_TYPE_BINS) ||
        (ISP_SVC_StatEngine.downRequest & ISP_STAT_TYPE_ALL_TMP))
    {
      /* Stat down cycle ends with last BINS measurement */
      stage = ISP_STAT_CFG_DOWN_BINS_9_11;
    }
    else
    {
      /* Stat down cycle ends with AVG measurement */
      stage = ISP_STAT_CFG_DOWN_AVG;
    }
  }
  return stage;
}

uint8_t LuminanceFromRGB(uint8_t r, uint8_t g, uint8_t b)
{
  /* Compute luminance from RGB components (BT.601) */
  return (uint8_t) (r * 0.299 + g * 0.587 + b * 0.114);
}

uint8_t LuminanceFromRGBMono(uint8_t r, uint8_t g, uint8_t b)
{
  /* Compute luminance from RGB components
   * by adding together R, G, B components for monochrome sensor */
  return r + g + b;
}

/* Exported functions --------------------------------------------------------*/
/**
  * @brief  ISP_SVC_ISP_SetDemosaicing
  *         Set the ISP demosaicing configuration
  * @param  hIsp: ISP device handle
  * @param  pConfig: Pointer to the demosaicing configuration
  * @retval operation result
  */
ISP_StatusTypeDef ISP_SVC_ISP_SetDemosaicing(ISP_HandleTypeDef *hIsp, ISP_DemosaicingTypeDef *pConfig)
{
  HAL_StatusTypeDef halStatus;
  DCMIPP_RawBayer2RGBConfTypeDef rawBayerCfg;

  if ((hIsp == NULL) || (pConfig == NULL) ||
      (pConfig->peak > ISP_DEMOS_STRENGTH_MAX) || (pConfig->lineV > ISP_DEMOS_STRENGTH_MAX) ||
      (pConfig->lineH > ISP_DEMOS_STRENGTH_MAX) || (pConfig->edge > ISP_DEMOS_STRENGTH_MAX))
  {
    return ISP_ERR_DEMOSAICING_EINVAL;
  }

  /* Do not enable demosaicing if the camera sensor is a monochrome sensor */
  if ((pConfig->enable == 0) || (pConfig->type == ISP_DEMOS_TYPE_MONO))
  {
    halStatus = HAL_DCMIPP_PIPE_DisableISPRawBayer2RGB(hIsp->hDcmipp, DCMIPP_PIPE1);
  }
  else
  {
    switch(pConfig->type)
    {
      case ISP_DEMOS_TYPE_RGGB:
        rawBayerCfg.RawBayerType = DCMIPP_RAWBAYER_RGGB;
        break;
      case ISP_DEMOS_TYPE_GRBG:
        rawBayerCfg.RawBayerType = DCMIPP_RAWBAYER_GRBG;
        break;
      case ISP_DEMOS_TYPE_GBRG:
        rawBayerCfg.RawBayerType = DCMIPP_RAWBAYER_GBRG;
        break;
      case ISP_DEMOS_TYPE_BGGR:
        rawBayerCfg.RawBayerType = DCMIPP_RAWBAYER_BGGR;
        break;
      default:
        rawBayerCfg.RawBayerType = DCMIPP_RAWBAYER_RGGB;
        break;
    }

    rawBayerCfg.PeakStrength = (uint32_t) pConfig->peak;
    rawBayerCfg.VLineStrength = (uint32_t) pConfig->lineV;
    rawBayerCfg.HLineStrength = (uint32_t) pConfig->lineH;
    rawBayerCfg.EdgeStrength = (uint32_t) pConfig->edge;
    halStatus = HAL_DCMIPP_PIPE_SetISPRawBayer2RGBConfig(hIsp->hDcmipp, DCMIPP_PIPE1, &rawBayerCfg);

    if (halStatus == HAL_OK)
    {
      halStatus = HAL_DCMIPP_PIPE_EnableISPRawBayer2RGB(hIsp->hDcmipp, DCMIPP_PIPE1);
    }
  }

  if (halStatus != HAL_OK)
  {
    return ISP_ERR_DEMOSAICING_HAL;
  }

  return ISP_OK;
}

/**
  * @brief  ISP_SVC_ISP_SetStatRemoval
  *         Set the ISP Stat Removal configuration
  * @param  hIsp: ISP device handle
  * @param  pConfig: Pointer to the Stat Removal configuration
  * @retval operation result
  */
ISP_StatusTypeDef ISP_SVC_ISP_SetStatRemoval(ISP_HandleTypeDef *hIsp, ISP_StatRemovalTypeDef *pConfig)
{
  HAL_StatusTypeDef halStatus;

  if ((hIsp == NULL) || (pConfig == NULL) ||
      (pConfig->nbHeadLines > ISP_STATREMOVAL_HEADLINES_MAX) || (pConfig->nbValidLines > ISP_STATREMOVAL_VALIDLINES_MAX))
  {
    return ISP_ERR_STATREMOVAL_EINVAL;
  }

  if (pConfig->enable == 0)
  {
    halStatus = HAL_DCMIPP_PIPE_DisableISPRemovalStatistic(hIsp->hDcmipp, DCMIPP_PIPE1);
  }
  else
  {
    halStatus = HAL_DCMIPP_PIPE_SetISPRemovalStatisticConfig(hIsp->hDcmipp, DCMIPP_PIPE1, pConfig->nbHeadLines, pConfig->nbValidLines);

    if (halStatus == HAL_OK)
    {
      halStatus = HAL_DCMIPP_PIPE_EnableISPRemovalStatistic(hIsp->hDcmipp, DCMIPP_PIPE1);
    }
  }

  if (halStatus != HAL_OK)
  {
    return ISP_ERR_STATREMOVAL_HAL;
  }

  return ISP_OK;
}

/**
  * @brief  ISP_SVC_ISP_SetDecimation
  *         Set the ISP Decimation configuration
  * @param  hIsp: ISP device handle
  * @param  pConfig: Pointer to the decimation configuration
  * @retval operation result
  */
ISP_StatusTypeDef ISP_SVC_ISP_SetDecimation(ISP_HandleTypeDef *hIsp, ISP_DecimationTypeDef *pConfig)
{
  DCMIPP_DecimationConfTypeDef decimationCfg;
  ISP_StatusTypeDef ret = ISP_OK;

  /* Check handles validity */
  if ((hIsp == NULL) || (pConfig == NULL))
  {
    return ISP_ERR_DECIMATION_EINVAL;
  }

  switch (pConfig->factor)
  {
  case ISP_DECIM_FACTOR_1:
    decimationCfg.VRatio = DCMIPP_VDEC_ALL;
    decimationCfg.HRatio = DCMIPP_HDEC_ALL;
    break;

  case ISP_DECIM_FACTOR_2:
    decimationCfg.VRatio = DCMIPP_VDEC_1_OUT_2;
    decimationCfg.HRatio = DCMIPP_HDEC_1_OUT_2;
    break;

  case ISP_DECIM_FACTOR_4:
    decimationCfg.VRatio = DCMIPP_VDEC_1_OUT_4;
    decimationCfg.HRatio = DCMIPP_HDEC_1_OUT_4;
    break;

  case ISP_DECIM_FACTOR_8:
    decimationCfg.VRatio = DCMIPP_VDEC_1_OUT_8;
    decimationCfg.HRatio = DCMIPP_HDEC_1_OUT_8;
    break;

  default:
    return ISP_ERR_DECIMATION_EINVAL;
  }

  if (HAL_DCMIPP_PIPE_SetISPDecimationConfig(hIsp->hDcmipp, DCMIPP_PIPE1, &decimationCfg) != HAL_OK)
  {
    return ISP_ERR_DECIMATION_HAL;
  }

  if (HAL_DCMIPP_PIPE_EnableISPDecimation(hIsp->hDcmipp, DCMIPP_PIPE1) != HAL_OK)
  {
    return ISP_ERR_DECIMATION_HAL;
  }

  /* Save decimation value */
  ISP_DecimationValue.factor = pConfig->factor;

  return ret;
}

/**
  * @brief  ISP_SVC_ISP_GetDecimation
  *         Get the ISP Decimation configuration
  * @param  hIsp: ISP device handle
  * @param  pConfig: Pointer to the decimation configuration
  * @retval operation result
  */
ISP_StatusTypeDef ISP_SVC_ISP_GetDecimation(ISP_HandleTypeDef *hIsp, ISP_DecimationTypeDef *pConfig)
{
  pConfig->factor = ISP_DecimationValue.factor;

  return ISP_OK;
}

/**
  * @brief  ISP_SVC_ISP_SetContrast
  *         Set the ISP Contrast luminance coefficients
  * @param  hIsp: ISP device handle
  * @param  pConfig: Pointer to the contrast configuration
  * @retval operation result
  */
ISP_StatusTypeDef ISP_SVC_ISP_SetContrast(ISP_HandleTypeDef *hIsp, ISP_ContrastTypeDef *pConfig)
{
  HAL_StatusTypeDef halStatus;
  DCMIPP_ContrastConfTypeDef contrast;

  if ((hIsp == NULL) || (pConfig == NULL) ||
      (pConfig->coeff.LUM_0 > ISP_CONTAST_LUMCOEFF_MAX) ||  (pConfig->coeff.LUM_32 > ISP_CONTAST_LUMCOEFF_MAX) ||
      (pConfig->coeff.LUM_64 > ISP_CONTAST_LUMCOEFF_MAX) || (pConfig->coeff.LUM_96 > ISP_CONTAST_LUMCOEFF_MAX) ||
      (pConfig->coeff.LUM_128 > ISP_CONTAST_LUMCOEFF_MAX) || (pConfig->coeff.LUM_160 > ISP_CONTAST_LUMCOEFF_MAX) ||
      (pConfig->coeff.LUM_192 > ISP_CONTAST_LUMCOEFF_MAX) || (pConfig->coeff.LUM_224 > ISP_CONTAST_LUMCOEFF_MAX) ||
      (pConfig->coeff.LUM_256 > ISP_CONTAST_LUMCOEFF_MAX))
  {
    return ISP_ERR_CONTRAST_EINVAL;
  }

  if (pConfig->enable == 0)
  {
    halStatus = HAL_DCMIPP_PIPE_DisableISPCtrlContrast(hIsp->hDcmipp, DCMIPP_PIPE1);
  }
  else
  {
    /* Concert coefficient unit from "percentage" to "6 bit" */
    contrast.LUM_0 = (pConfig->coeff.LUM_0 * 16) / 100;
    contrast.LUM_32 = (pConfig->coeff.LUM_32 * 16) / 100;
    contrast.LUM_64 = (pConfig->coeff.LUM_64 * 16) / 100;
    contrast.LUM_96 = (pConfig->coeff.LUM_96 * 16) / 100;
    contrast.LUM_128 = (pConfig->coeff.LUM_128 * 16) / 100;
    contrast.LUM_160 = (pConfig->coeff.LUM_160 * 16) / 100;
    contrast.LUM_192 = (pConfig->coeff.LUM_192 * 16) / 100;
    contrast.LUM_224 = (pConfig->coeff.LUM_224 * 16) / 100;
    contrast.LUM_256 = (pConfig->coeff.LUM_256 * 16) / 100;
    halStatus = HAL_DCMIPP_PIPE_SetISPCtrlContrastConfig(hIsp->hDcmipp, DCMIPP_PIPE1, &contrast);

    if (halStatus == HAL_OK)
    {
      halStatus = HAL_DCMIPP_PIPE_EnableISPCtrlContrast(hIsp->hDcmipp, DCMIPP_PIPE1);
    }
  }

  if (halStatus != HAL_OK)
  {
    return ISP_ERR_CONTRAST_HAL;
  }

  return ISP_OK;
}

/**
  * @brief  ISP_SVC_ISP_SetStatArea
  *         Set the ISP Statistic area
  * @param  hIsp: ISP device handle
  * @param  pConfig: Pointer to statistic area used by the IQ algorithms
  * @retval operation result
  */
ISP_StatusTypeDef ISP_SVC_ISP_SetStatArea(ISP_HandleTypeDef *hIsp, ISP_StatAreaTypeDef *pConfig)
{
  HAL_StatusTypeDef halStatus;
  DCMIPP_StatisticExtractionAreaConfTypeDef currentStatAreaCfg;
  ISP_StatusTypeDef ret = ISP_OK;

  if ((hIsp == NULL) || (pConfig == NULL) ||
      (pConfig->X0 > ISP_STATWINDOW_MAX) ||
      (pConfig->Y0 > ISP_STATWINDOW_MAX) ||
      (pConfig->XSize > ISP_STATWINDOW_MAX) ||
      (pConfig->YSize > ISP_STATWINDOW_MAX) ||
      (pConfig->XSize < ISP_STATWINDOW_MIN) ||
      (pConfig->YSize < ISP_STATWINDOW_MIN) ||
      (pConfig->X0 + pConfig->XSize > hIsp->sensorInfo.width) ||
      (pConfig->Y0 + pConfig->YSize > hIsp->sensorInfo.height))
  {
    return ISP_ERR_STATAREA_EINVAL;
  }

  /* Set coordinates in the 'decimated' referential */
  currentStatAreaCfg.HStart = pConfig->X0 / ISP_DecimationValue.factor;
  currentStatAreaCfg.VStart = pConfig->Y0 / ISP_DecimationValue.factor;
  currentStatAreaCfg.HSize = pConfig->XSize / ISP_DecimationValue.factor;
  currentStatAreaCfg.VSize = pConfig->YSize / ISP_DecimationValue.factor;

  if (HAL_DCMIPP_PIPE_SetISPAreaStatisticExtractionConfig(hIsp->hDcmipp, DCMIPP_PIPE1,
                                                          &currentStatAreaCfg) != HAL_OK)
  {
    return ISP_ERR_STATAREA_HAL;
  }
  else
  {
    halStatus = HAL_DCMIPP_PIPE_EnableISPAreaStatisticExtraction(hIsp->hDcmipp, DCMIPP_PIPE1);
  }

  if (halStatus != HAL_OK)
  {
    return ISP_ERR_STATAREA_HAL;
  }

  return ret;
}

/**
  * @brief  ISP_SVC_ISP_GetStatArea
  *         Get the ISP Statistic area
  * @param  hIsp: ISP device handle
  * @param  pConfig: Pointer to statistic area used by the IQ algorithms
  * @retval operation result
  */
ISP_StatusTypeDef ISP_SVC_ISP_GetStatArea(ISP_HandleTypeDef *hIsp, ISP_StatAreaTypeDef *pConfig)
{
  DCMIPP_StatisticExtractionAreaConfTypeDef currentStatAreaCfg;

  /* Check handles validity */
  if ((hIsp == NULL) || (pConfig == NULL))
  {
    return ISP_ERR_STATAREA_EINVAL;
  }

  if (HAL_DCMIPP_PIPE_IsEnabledISPAreaStatisticExtraction(hIsp->hDcmipp, DCMIPP_PIPE1) == 0)
  {
    pConfig->X0 = 0;
    pConfig->Y0 = 0;
    pConfig->XSize = 0;
    pConfig->YSize = 0;
  }
  else
  {
    HAL_DCMIPP_PIPE_GetISPAreaStatisticExtractionConfig(hIsp->hDcmipp, DCMIPP_PIPE1,
                                                        &currentStatAreaCfg);

    /* Consider decimation */
    pConfig->X0 = currentStatAreaCfg.HStart * ISP_DecimationValue.factor;
    pConfig->Y0 = currentStatAreaCfg.VStart * ISP_DecimationValue.factor;
    pConfig->XSize = currentStatAreaCfg.HSize * ISP_DecimationValue.factor;
    pConfig->YSize = currentStatAreaCfg.VSize * ISP_DecimationValue.factor;
  }

  return ISP_OK;
}

/**
  * @brief  ISP_SVC_ISP_SetBadPixel
  *         Set the ISP Bad pixel control configuration
  * @param  hIsp: ISP device handle
  * @param  pConfig: Pointer to the bad pixel configuration
  * @retval operation result
  */
ISP_StatusTypeDef ISP_SVC_ISP_SetBadPixel(ISP_HandleTypeDef *hIsp, ISP_BadPixelTypeDef *pConfig)
{
  HAL_StatusTypeDef halStatus;

  if ((hIsp == NULL) || (pConfig == NULL) || (pConfig->strength > ISP_BADPIXEL_STRENGTH_MAX))
  {
    return ISP_ERR_BADPIXEL_EINVAL;
  }

  if (pConfig->enable == 0)
  {
    halStatus = HAL_DCMIPP_PIPE_DisableISPBadPixelRemoval(hIsp->hDcmipp, DCMIPP_PIPE1);
  }
  else
  {
    halStatus = HAL_DCMIPP_PIPE_SetISPBadPixelRemovalConfig(hIsp->hDcmipp, DCMIPP_PIPE1, pConfig->strength);

    if (halStatus != HAL_OK)
    {
      return ISP_ERR_BADPIXEL_HAL;
    }

    halStatus = HAL_DCMIPP_PIPE_EnableISPBadPixelRemoval(hIsp->hDcmipp, DCMIPP_PIPE1);
  }

  if (halStatus != HAL_OK)
  {
    return ISP_ERR_BADPIXEL_HAL;
  }

  return ISP_OK;
}

/**
  * @brief  ISP_SVC_ISP_GetBadPixel
  *         Get the ISP Bad pixel control configuration
  * @param  hIsp: ISP device handle
  * @param  pConfig: Pointer to the bad pixel configuration
  * @retval operation result
  */
ISP_StatusTypeDef ISP_SVC_ISP_GetBadPixel(ISP_HandleTypeDef *hIsp, ISP_BadPixelTypeDef *pConfig)
{
  HAL_StatusTypeDef halStatus;

  /* Check handle validity */
  if ((hIsp == NULL) || (pConfig == NULL))
  {
    return ISP_ERR_BADPIXEL_EINVAL;
  }

  pConfig->enable = HAL_DCMIPP_PIPE_IsEnabledISPBadPixelRemoval(hIsp->hDcmipp, DCMIPP_PIPE1);
  pConfig->strength = (uint8_t) HAL_DCMIPP_PIPE_GetISPBadPixelRemovalConfig(hIsp->hDcmipp, DCMIPP_PIPE1);

  halStatus = HAL_DCMIPP_PIPE_GetISPRemovedBadPixelCounter(hIsp->hDcmipp, DCMIPP_PIPE1, &pConfig->count);

  if (halStatus != HAL_OK)
  {
    return ISP_ERR_BADPIXEL_HAL;
  }

  return ISP_OK;
}

/**
  * @brief  ISP_SVC_ISP_SetBlackLevel
  *         Set the ISP Black Level calibration
  * @param  hIsp: ISP device handle
  * @param  pConfig: Pointer to the black level configuration
  * @retval operation result
  */
ISP_StatusTypeDef ISP_SVC_ISP_SetBlackLevel(ISP_HandleTypeDef *hIsp, ISP_BlackLevelTypeDef *pConfig)
{
  HAL_StatusTypeDef halStatus;
  DCMIPP_BlackLevelConfTypeDef blackLevelConfig;

  /* Check handle validity */
  if ((hIsp == NULL) || (pConfig == NULL))
  {
    return ISP_ERR_BLACKLEVEL_EINVAL;
  }

  if (pConfig->enable == 0)
  {
    halStatus = HAL_DCMIPP_PIPE_DisableISPBlackLevelCalibration(hIsp->hDcmipp, DCMIPP_PIPE1);
  }
  else
  {
    blackLevelConfig.RedCompBlackLevel = pConfig->BLCR;
    blackLevelConfig.GreenCompBlackLevel = pConfig->BLCG;
    blackLevelConfig.BlueCompBlackLevel = pConfig->BLCB;
    halStatus = HAL_DCMIPP_PIPE_SetISPBlackLevelCalibrationConfig(hIsp->hDcmipp, DCMIPP_PIPE1, &blackLevelConfig);

    if (halStatus == HAL_OK)
    {
      halStatus = HAL_DCMIPP_PIPE_EnableISPBlackLevelCalibration(hIsp->hDcmipp, DCMIPP_PIPE1);
    }
  }

  if (halStatus != HAL_OK)
  {
    return ISP_ERR_BLACKLEVEL_HAL;
  }

  return ISP_OK;
}

/**
  * @brief  ISP_SVC_ISP_GetBlackLevel
  *         Get the ISP Black Level calibration
  * @param  hIsp: ISP device handle
  * @param  pConfig: Pointer to the black level configuration
  * @retval operation result
  */
ISP_StatusTypeDef ISP_SVC_ISP_GetBlackLevel(ISP_HandleTypeDef *hIsp, ISP_BlackLevelTypeDef *pConfig)
{
  DCMIPP_BlackLevelConfTypeDef blackLevelConfig;

  /* Check handle validity */
  if ((hIsp == NULL) || (pConfig == NULL))
  {
    return ISP_ERR_BLACKLEVEL_EINVAL;
  }

  pConfig->enable = HAL_DCMIPP_PIPE_IsEnabledISPBlackLevelCalibration(hIsp->hDcmipp, DCMIPP_PIPE1);

  HAL_DCMIPP_PIPE_GetISPBlackLevelCalibrationConfig(hIsp->hDcmipp, DCMIPP_PIPE1, &blackLevelConfig);

  pConfig->BLCR = blackLevelConfig.RedCompBlackLevel;
  pConfig->BLCG = blackLevelConfig.GreenCompBlackLevel;
  pConfig->BLCB = blackLevelConfig.BlueCompBlackLevel;

  return ISP_OK;
}

/**
  * @brief  ISP_SVC_ISP_SetGain
  *         Set the ISP Exposure and White Balance gains
  * @param  hIsp: ISP device handle
  * @param  pConfig: Pointer to the ISP gain configuration
  * @retval operation result
  */
ISP_StatusTypeDef ISP_SVC_ISP_SetGain(ISP_HandleTypeDef *hIsp, ISP_ISPGainTypeDef *pConfig)
{
  HAL_StatusTypeDef halStatus;
  DCMIPP_ExposureConfTypeDef exposureConfig;

  if ((hIsp == NULL) || (pConfig == NULL) ||
      (pConfig->ispGainR > ISP_EXPOSURE_GAIN_MAX) || (pConfig->ispGainG > ISP_EXPOSURE_GAIN_MAX) || (pConfig->ispGainB > ISP_EXPOSURE_GAIN_MAX))
  {
    return ISP_ERR_ISPGAIN_EINVAL;
  }

  if (pConfig->enable == 0)
  {
    halStatus = HAL_DCMIPP_PIPE_DisableISPExposure(hIsp->hDcmipp, DCMIPP_PIPE1);
  }
  else
  {
    To_Shift_Multiplier(pConfig->ispGainR, &exposureConfig.ShiftRed, &exposureConfig.MultiplierRed);
    To_Shift_Multiplier(pConfig->ispGainG, &exposureConfig.ShiftGreen, &exposureConfig.MultiplierGreen);
    To_Shift_Multiplier(pConfig->ispGainB, &exposureConfig.ShiftBlue, &exposureConfig.MultiplierBlue);
    halStatus = HAL_DCMIPP_PIPE_SetISPExposureConfig(hIsp->hDcmipp, DCMIPP_PIPE1, &exposureConfig);

    if (halStatus == HAL_OK)
    {
      halStatus = HAL_DCMIPP_PIPE_EnableISPExposure(hIsp->hDcmipp, DCMIPP_PIPE1);
    }
  }

  if (halStatus != HAL_OK)
  {
    return ISP_ERR_ISPGAIN_HAL;
  }

  return ISP_OK;
}

/**
  * @brief  ISP_SVC_ISP_GetGain
  *         Get the ISP Exposure and White Balance gains
  * @param  hIsp: ISP device handle
  * @param  pConfig: Pointer to the ISP gain configuration
  * @retval operation result
  */
ISP_StatusTypeDef ISP_SVC_ISP_GetGain(ISP_HandleTypeDef *hIsp, ISP_ISPGainTypeDef *pConfig)
{
  DCMIPP_ExposureConfTypeDef exposureConfig;

  /* Check handle validity */
  if ((hIsp == NULL) || (pConfig == NULL))
  {
    return ISP_ERR_ISPGAIN_EINVAL;
  }

  pConfig->enable = HAL_DCMIPP_PIPE_IsEnabledISPExposure(hIsp->hDcmipp, DCMIPP_PIPE1);
  HAL_DCMIPP_PIPE_GetISPExposureConfig(hIsp->hDcmipp, DCMIPP_PIPE1, &exposureConfig);

  pConfig->ispGainR = From_Shift_Multiplier(exposureConfig.ShiftRed, exposureConfig.MultiplierRed);
  pConfig->ispGainG = From_Shift_Multiplier(exposureConfig.ShiftGreen, exposureConfig.MultiplierGreen);
  pConfig->ispGainB = From_Shift_Multiplier(exposureConfig.ShiftBlue, exposureConfig.MultiplierBlue);

  return ISP_OK;
}

/**
  * @brief  ISP_SVC_ISP_SetColorConv
  *         Set the ISP Color Conversion
  * @param  hIsp: ISP device handle
  * @param  pConfig: Pointer to the Color Conversion configuration
  * @retval operation result
  */
ISP_StatusTypeDef ISP_SVC_ISP_SetColorConv(ISP_HandleTypeDef *hIsp, ISP_ColorConvTypeDef *pConfig)
{
  HAL_StatusTypeDef halStatus;
  DCMIPP_ColorConversionConfTypeDef colorConvConfig;
  uint32_t i, j;

  memset(&colorConvConfig, 0, sizeof(colorConvConfig));

  /* Check handle validity */
  if ((hIsp == NULL) || (pConfig == NULL))
  {
    return ISP_ERR_COLORCONV_EINVAL;
  }

  for (i = 0; i < 3; i++)
  {
    for (j = 0; j < 3; j++)
    {
      if ((pConfig->coeff[i][j] > ISP_COLORCONV_MAX) || (pConfig->coeff[i][j] < -ISP_COLORCONV_MAX))
      {
        return ISP_ERR_COLORCONV_EINVAL;
      }
    }
  }

  if (pConfig->enable == 0)
  {
    halStatus = HAL_DCMIPP_PIPE_DisableISPColorConversion(hIsp->hDcmipp, DCMIPP_PIPE1);
  }
  else
  {
    colorConvConfig.RR = To_CConv_Reg(pConfig->coeff[0][0]);
    colorConvConfig.RG = To_CConv_Reg(pConfig->coeff[0][1]);
    colorConvConfig.RB = To_CConv_Reg(pConfig->coeff[0][2]);
    colorConvConfig.GR = To_CConv_Reg(pConfig->coeff[1][0]);
    colorConvConfig.GG = To_CConv_Reg(pConfig->coeff[1][1]);
    colorConvConfig.GB = To_CConv_Reg(pConfig->coeff[1][2]);
    colorConvConfig.BR = To_CConv_Reg(pConfig->coeff[2][0]);
    colorConvConfig.BG = To_CConv_Reg(pConfig->coeff[2][1]);
    colorConvConfig.BB = To_CConv_Reg(pConfig->coeff[2][2]);
    halStatus = HAL_DCMIPP_PIPE_SetISPColorConversionConfig(hIsp->hDcmipp, DCMIPP_PIPE1, &colorConvConfig);

    if (halStatus == HAL_OK)
    {
      halStatus = HAL_DCMIPP_PIPE_EnableISPColorConversion(hIsp->hDcmipp, DCMIPP_PIPE1);
    }
  }

  if (halStatus != HAL_OK)
  {
    return ISP_ERR_COLORCONV_HAL;
  }

  return ISP_OK;
}

/**
  * @brief  ISP_SVC_ISP_GetColorConv
  *         Get the ISP Color Conversion
  * @param  hIsp: ISP device handle
  * @param  pConfig: Pointer to the Color Conversion configuration
  * @retval operation result
  */
ISP_StatusTypeDef ISP_SVC_ISP_GetColorConv(ISP_HandleTypeDef *hIsp, ISP_ColorConvTypeDef *pConfig)
{
  DCMIPP_ColorConversionConfTypeDef colorConvConfig;

  /* Check handle validity */
  if ((hIsp == NULL) || (pConfig == NULL))
  {
    return ISP_ERR_COLORCONV_EINVAL;
  }

  pConfig->enable = HAL_DCMIPP_PIPE_IsEnabledISPColorConversion(hIsp->hDcmipp, DCMIPP_PIPE1);

  HAL_DCMIPP_PIPE_GetISPColorConversionConfig(hIsp->hDcmipp, DCMIPP_PIPE1, &colorConvConfig);

  pConfig->coeff[0][0] = From_CConv_Reg(colorConvConfig.RR);
  pConfig->coeff[0][1] = From_CConv_Reg(colorConvConfig.RG);
  pConfig->coeff[0][2] = From_CConv_Reg(colorConvConfig.RB);
  pConfig->coeff[1][0] = From_CConv_Reg(colorConvConfig.GR);
  pConfig->coeff[1][1] = From_CConv_Reg(colorConvConfig.GG);
  pConfig->coeff[1][2] = From_CConv_Reg(colorConvConfig.GB);
  pConfig->coeff[2][0] = From_CConv_Reg(colorConvConfig.BR);
  pConfig->coeff[2][1] = From_CConv_Reg(colorConvConfig.BG);
  pConfig->coeff[2][2] = From_CConv_Reg(colorConvConfig.BB);

  return ISP_OK;
}

/**
  * @brief  ISP_SVC_Sensor_GetInfo
  *         Get the sensor info
  * @param  hIsp: ISP device handle
  * @param  pConfig: Pointer to the sensor info
  * @retval operation result
  */
ISP_StatusTypeDef ISP_SVC_Sensor_GetInfo(ISP_HandleTypeDef *hIsp, ISP_SensorInfoTypeDef *pConfig)
{
  /* Check handle validity */
  if ((hIsp == NULL) || (pConfig == NULL))
  {
    return ISP_ERR_SENSORINFO_EINVAL;
  }

  if (hIsp->appliHelpers.GetSensorInfo != NULL)
  {
    if (hIsp->appliHelpers.GetSensorInfo(hIsp->cameraInstance, pConfig) != 0)
    {
      return ISP_ERR_SENSORINFO;
    }
  }

  return ISP_OK;
}

/**
  * @brief  ISP_SVC_Sensor_SetGain
  *         Set the sensor gain
  * @param  hIsp: ISP device handle
  * @param  pConfig: Pointer to the sensor gain configuration
  * @retval operation result
  */
ISP_StatusTypeDef ISP_SVC_Sensor_SetGain(ISP_HandleTypeDef *hIsp, ISP_SensorGainTypeDef *pConfig)
{
  /* Check handle validity */
  if ((hIsp == NULL) || (pConfig == NULL))
  {
    return ISP_ERR_SENSORGAIN_EINVAL;
  }

  if (hIsp->appliHelpers.SetSensorGain != NULL)
  {
    if (hIsp->appliHelpers.SetSensorGain(hIsp->cameraInstance, pConfig->gain) != 0)
    {
      return ISP_ERR_SENSORGAIN;
    }
  }

  return ISP_OK;
}

/**
  * @brief  ISP_SVC_Sensor_GetGain
  *         Get the sensor gain
  * @param  hIsp: ISP device handle
  * @param  pConfig: Pointer to the sensor gain configuration
  * @retval operation result
  */
ISP_StatusTypeDef ISP_SVC_Sensor_GetGain(ISP_HandleTypeDef *hIsp, ISP_SensorGainTypeDef *pConfig)
{
  /* Check handle validity */
  if ((hIsp == NULL) || (pConfig == NULL))
  {
    return ISP_ERR_SENSORGAIN_EINVAL;
  }

  if (hIsp->appliHelpers.GetSensorGain != NULL)
  {
    if (hIsp->appliHelpers.GetSensorGain(hIsp->cameraInstance, (int32_t *)&pConfig->gain) != 0)
    {
      return ISP_ERR_SENSORGAIN;
    }
  }

  return ISP_OK;
}

/**
  * @brief  ISP_SVC_Sensor_SetExposure
  *         Set the sensor exposure
  * @param  hIsp: ISP device handle
  * @param  pConfig: Pointer to the sensor exposure configuration
  * @retval operation result
  */
ISP_StatusTypeDef ISP_SVC_Sensor_SetExposure(ISP_HandleTypeDef *hIsp, ISP_SensorExposureTypeDef *pConfig)
{
  /* Check handle validity */
  if ((hIsp == NULL) || (pConfig == NULL))
  {
    return ISP_ERR_SENSOREXPOSURE_EINVAL;
  }

  if (hIsp->appliHelpers.SetSensorExposure != NULL)
  {
    if (hIsp->appliHelpers.SetSensorExposure(hIsp->cameraInstance, pConfig->exposure) != 0)
    {
      return ISP_ERR_SENSOREXPOSURE;
    }
  }

  return ISP_OK;
}

/**
  * @brief  ISP_SVC_Sensor_GetExposure
  *         Get the sensor exposure
  * @param  hIsp: ISP device handle
  * @param  pConfig: Pointer to the sensor exposure configuration
  * @retval operation result
  */
ISP_StatusTypeDef ISP_SVC_Sensor_GetExposure(ISP_HandleTypeDef *hIsp, ISP_SensorExposureTypeDef *pConfig)
{
  /* Check handle validity */
  if ((hIsp == NULL) || (pConfig == NULL))
  {
    return ISP_ERR_SENSOREXPOSURE_EINVAL;
  }

  if (hIsp->appliHelpers.GetSensorExposure != NULL)
  {
    if (hIsp->appliHelpers.GetSensorExposure(hIsp->cameraInstance, (int32_t *)&pConfig->exposure) != 0)
    {
      return ISP_ERR_SENSOREXPOSURE;
    }
  }

  return ISP_OK;
}

/**
  * @brief  ISP_SVC_Sensor_SetTestPattern
  *         Set the sensor test pattern
  * @param  hIsp: ISP device handle
  * @param  pConfig: Pointer to the sensor test pattern configuration
  * @retval operation result
  */
ISP_StatusTypeDef ISP_SVC_Sensor_SetTestPattern(ISP_HandleTypeDef *hIsp, ISP_SensorTestPatternTypeDef *pConfig)
{
  /* Check handle validity */
  if ((hIsp == NULL) || (pConfig == NULL))
  {
    return ISP_ERR_SENSORTESTPATTERN;
  }

  if (hIsp->appliHelpers.SetSensorTestPattern == NULL)
  {
    return ISP_ERR_APP_HELPER_UNDEFINED;
  }

  if (hIsp->appliHelpers.SetSensorTestPattern(hIsp->cameraInstance, pConfig->mode) != 0)
  {
    return ISP_ERR_SENSORTESTPATTERN;
  }

  return ISP_OK;
}

/**
  * @brief  ISP_SVC_Misc_GetDCMIPPVersion
  *         Get the DCMIPP IP version
  * @param  hIsp: ISP device handle
  * @param  pMajRev: Pointer to major revision of DCMIPP
  * @param  pMinRev: Pointer to minor revision of DCMIPP
  * @retval ISP_OK if DCMIPP version is read properly, ISP_FAIL otherwise
  */
ISP_StatusTypeDef ISP_SVC_Misc_GetDCMIPPVersion(ISP_HandleTypeDef *hIsp, uint32_t *pMajRev, uint32_t *pMinRev)
{
  /* Check handle validity */
  if ((hIsp == NULL) || (pMajRev == NULL) || (pMinRev == NULL))
  {
    return ISP_ERR_EINVAL;
  }

  DCMIPP_HandleTypeDef *hDcmipp = hIsp->hDcmipp;
  *pMajRev = (hDcmipp->Instance->VERR & DCMIPP_VERR_MAJREV) >> DCMIPP_VERR_MAJREV_Pos;
  *pMinRev = (hDcmipp->Instance->VERR & DCMIPP_VERR_MINREV) >> DCMIPP_VERR_MINREV_Pos;

  return ISP_OK;
}

/**
  * @brief  ISP_SVC_Misc_IsDCMIPPReady
  *         Check that the DCMIPP device is ready
  * @param  hIsp: ISP device handle
  * @retval ISP_OK if DCMIPP is running, ISP_FAIL otherwise
  */
ISP_StatusTypeDef ISP_SVC_Misc_IsDCMIPPReady(ISP_HandleTypeDef *hIsp)
{
  /* Check handle validity */
  if (hIsp == NULL)
  {
    return ISP_ERR_EINVAL;
  }

  if (HAL_DCMIPP_GetState(hIsp->hDcmipp) != HAL_DCMIPP_STATE_READY)
  {
    return ISP_ERR_DCMIPP_STATE;
  }

  return ISP_OK;
}

/**
  * @brief  ISP_SVC_Misc_IncMainFrameId
  *         Increment the id of the frame output on the main pipe
  * @param  hIsp: ISP device handle
  * @retval none
  */
void ISP_SVC_Misc_IncMainFrameId(ISP_HandleTypeDef *hIsp)
{
  hIsp->MainPipe_FrameCount++;
}

/**
  * @brief  ISP_SVC_Misc_GetMainFrameId
  *         Return the id of the last frame output on the main pipe
  * @param  hIsp: ISP device handle
  * @retval Id of the last frame output on the main pipe
  */
uint32_t ISP_SVC_Misc_GetMainFrameId(ISP_HandleTypeDef *hIsp)
{
  return hIsp->MainPipe_FrameCount;
}

/**
  * @brief  ISP_SVC_Misc_IncAncillaryFrameId
  *         Increment the id of the frame output on the ancillary pipe
  * @param  hIsp: ISP device handle
  * @retval none
  */
void ISP_SVC_Misc_IncAncillaryFrameId(ISP_HandleTypeDef *hIsp)
{
  hIsp->AncillaryPipe_FrameCount++;
}

/**
  * @brief  ISP_SVC_Misc_GetAncillaryFrameId
  *         Return the id of the last frame output on the ancillary pipe
  * @param  hIsp: ISP device handle
  * @retval Id of the last frame output on the ancillary pipe
  */
uint32_t ISP_SVC_Misc_GetAncillaryFrameId(ISP_HandleTypeDef *hIsp)
{
  return hIsp->AncillaryPipe_FrameCount;
}

/**
  * @brief  ISP_SVC_Misc_IncDumpFrameId
  *         Increment the id of the frame output on the dump pipe
  * @param  hIsp: ISP device handle
  * @retval none
  */
void ISP_SVC_Misc_IncDumpFrameId(ISP_HandleTypeDef *hIsp)
{
  hIsp->DumpPipe_FrameCount++;
}

/**
  * @brief  ISP_SVC_Misc_GetDumpFrameId
  *         Return the id of the last frame output on the dump pipe
  * @param  hIsp: ISP device handle
  * @retval Id of the last frame output on the dump pipe
  */
uint32_t ISP_SVC_Misc_GetDumpFrameId(ISP_HandleTypeDef *hIsp)
{
  return hIsp->DumpPipe_FrameCount;
}

/**
  * @brief  ISP_SVC_Misc_SetWBRefMode
  *         Set the reference mode used to configure manually the white balance mode
  * @param  hIsp: ISP device handle
  * @param  RefColorTemp: reference color temperature
  * @retval Operation status
  */
ISP_StatusTypeDef ISP_SVC_Misc_SetWBRefMode(ISP_HandleTypeDef *hIsp, uint32_t RefColorTemp)
{
  (void)hIsp; /* unused */

  ISP_ManualWBRefColorTemp = RefColorTemp;

  return ISP_OK;
}

/**
  * @brief  ISP_SVC_Misc_GetWBRefMode
  *         Get the reference mode used to configure manually the white balance mode
  * @param  hIsp: ISP device handle
  * @param  pRefColorTemp: Pointer to reference color temperature
  * @retval Operation status
  */
ISP_StatusTypeDef ISP_SVC_Misc_GetWBRefMode(ISP_HandleTypeDef *hIsp, uint32_t *pRefColorTemp)
{
  (void)hIsp; /* unused */

  *pRefColorTemp = ISP_ManualWBRefColorTemp;

  return ISP_OK;
}

/**
  * @brief  ISP_SVC_Misc_StopPreview
  *         Stop streaming on DCMIPP Main Pipe
  * @param  hIsp: ISP device handle
  * @retval ISP_OK if DCMIPP is stopped successfully, ISP_FAIL otherwise
  */
ISP_StatusTypeDef ISP_SVC_Misc_StopPreview(ISP_HandleTypeDef *hIsp)
{
  /* Check handle validity */
  if (hIsp == NULL)
  {
    return ISP_ERR_EINVAL;
  }

  if (hIsp->appliHelpers.StopPreview == NULL)
  {
    return ISP_ERR_APP_HELPER_UNDEFINED;
  }

  return hIsp->appliHelpers.StopPreview(hIsp->hDcmipp);
}

/**
  * @brief  ISP_SVC_Misc_StartPreview
  *         Start streaming on DCMIPP Main Pipe
  * @param  hIsp: ISP device handle
  * @retval ISP_OK if DCMIPP is started successfully, ISP_FAIL otherwise
  */
ISP_StatusTypeDef ISP_SVC_Misc_StartPreview(ISP_HandleTypeDef *hIsp)
{
  /* Check handle validity */
  if (hIsp == NULL)
  {
    return ISP_ERR_EINVAL;
  }

  if (hIsp->appliHelpers.StartPreview == NULL)
  {
    return ISP_ERR_APP_HELPER_UNDEFINED;
  }

  return hIsp->appliHelpers.StartPreview(hIsp->hDcmipp);
}

/**
  * @brief  ISP_SVC_Misc_IsGammaEnabled
  *         Check if the gamma block is enabled
  * @param  hIsp: ISP device handle
  * @param  Pipe: DCMIPP pipe line
  * @retval 1 if enabled 0 otherwise
  */
bool ISP_SVC_Misc_IsGammaEnabled(ISP_HandleTypeDef *hIsp, uint32_t Pipe)
{
  uint8_t ret;

  /* Check handle validity */
  if (hIsp == NULL)
  {
    return ISP_ERR_EINVAL;
  }

  switch(Pipe)
  {
  case 1:
    ret = HAL_DCMIPP_PIPE_IsEnabledGammaConversion(hIsp->hDcmipp, DCMIPP_PIPE1);
    break;
  case 2:
    ret = HAL_DCMIPP_PIPE_IsEnabledGammaConversion(hIsp->hDcmipp, DCMIPP_PIPE2);
    break;
  default:
    ret = 0; /*  No gamma on pipe 0 */
  }

  return ret;
}

/**
  * @brief  ISP_SVC_ISP_SetGamma
  *         Set the Gamma on Pipe1 and/or Pipe2
  * @param  hIsp: ISP device handle
  * @param  pConfig: Pointer to the ISP gamma configuration
  * @retval operation result
  */
ISP_StatusTypeDef ISP_SVC_ISP_SetGamma(ISP_HandleTypeDef *hIsp, ISP_GammaTypeDef *pConfig)
{
  if ((hIsp == NULL) || (pConfig == NULL))
  {
    return ISP_ERR_DCMIPP_GAMMA;
  }

  if (pConfig->enablePipe1 == 0)
  {
    if (HAL_DCMIPP_PIPE_DisableGammaConversion(hIsp->hDcmipp, DCMIPP_PIPE1) != HAL_OK)
    {
      return ISP_ERR_DCMIPP_GAMMA;
    }
  }
  else
  {
    if (HAL_DCMIPP_PIPE_EnableGammaConversion(hIsp->hDcmipp, DCMIPP_PIPE1) != HAL_OK)
    {
      return ISP_ERR_DCMIPP_GAMMA;
    }
  }

  if (pConfig->enablePipe2 == 0)
  {
    if (HAL_DCMIPP_PIPE_DisableGammaConversion(hIsp->hDcmipp, DCMIPP_PIPE2) != HAL_OK)
    {
      return ISP_ERR_DCMIPP_GAMMA;
    }
  }
  else
  {
    if (HAL_DCMIPP_PIPE_EnableGammaConversion(hIsp->hDcmipp, DCMIPP_PIPE2) != HAL_OK)
    {
      return ISP_ERR_DCMIPP_GAMMA;
    }
  }

  return ISP_OK;
}

/**
  * @brief  ISP_SVC_Dump_GetFrame
  *         Dump a frame
  * @param  hIsp: ISP device handle
  * @param  pBuffer: pointer to the address of the dumped buffer
  * @param  pMeta: buffer meta data
  * @param  DumpConfig: Type of dump configuration requested
  * @retval ISP_OK if DCMIPP is running, ISP_FAIL otherwise
  */
ISP_StatusTypeDef ISP_SVC_Dump_GetFrame(ISP_HandleTypeDef *hIsp, uint32_t **pBuffer, ISP_DumpCfgTypeDef DumpConfig, ISP_DumpFrameMetaTypeDef *pMeta)
{
  uint32_t DumpPipe;

  /* Check handle validity */
  if ((hIsp == NULL) || (pBuffer == NULL) || (pMeta == NULL))
  {
    return ISP_ERR_EINVAL;
  }

  if (hIsp->appliHelpers.DumpFrame == NULL)
  {
    return ISP_ERR_APP_HELPER_UNDEFINED;
  }


  if (DumpConfig == ISP_DUMP_CFG_DUMP_PIPE_SENSOR) {
    /* Dump the frame on pipe0 in its native RAW format */
    DumpPipe = DCMIPP_PIPE0;
  }
  else if (DumpConfig == ISP_DUMP_CFG_FULLSIZE_RGB888) {
    /* Dump the full frame on pipe2 */
    DumpPipe = DCMIPP_PIPE2;
  }
  else {
    /* Live Streaming dump on pipe2 in its default configuration */
    DumpPipe = DCMIPP_PIPE2;
  }

  /* Call the DumpFrame application function */
  return hIsp->appliHelpers.DumpFrame(hIsp->hDcmipp, DumpPipe, DumpConfig, pBuffer, pMeta);
}

/**
  * @brief  ISP_SVC_IQParam_Init
  *         Initialize the IQ parameters cache with values from non volatile memory
  * @param  hIsp: ISP device handle
  * @retval ISP status
  */
ISP_StatusTypeDef ISP_SVC_IQParam_Init(ISP_HandleTypeDef *hIsp, const ISP_IQParamTypeDef *ISP_IQParamCacheInit)
{
  (void)hIsp; /* unused */

  ISP_IQParamCache = *ISP_IQParamCacheInit;
  return ISP_OK;
}

/**
  * @brief  ISP_SVC_IQParam_Get
  *         Get the pointer to the IQ parameters cache
  * @param  hIsp: ISP device handle
  * @retval Pointer to the IQ Param config
  */
ISP_IQParamTypeDef *ISP_SVC_IQParam_Get(ISP_HandleTypeDef *hIsp)
{
  (void)hIsp; /* unused */

  return &ISP_IQParamCache;
}

/**
  * @brief  ISP_SVC_Stats_Init
  *         Initialize the statistic engine
  * @param  hIsp: ISP device handle
  * @retval None
  */
void ISP_SVC_Stats_Init(ISP_HandleTypeDef *hIsp)
{
  memset(&ISP_SVC_StatEngine, 0, sizeof(ISP_SVC_StatEngineTypeDef));
}

/**
  * @brief  ISP_SVC_Stats_Gather
  *         Gather statistics
  * @param  hIsp: ISP device handle
  * @retval None
  */
void ISP_SVC_Stats_Gather(ISP_HandleTypeDef *hIsp)
{
  static ISP_SVC_StatEngineStage stagePrevious1 = ISP_STAT_CFG_LAST, stagePrevious2 = ISP_STAT_CFG_LAST;
  DCMIPP_StatisticExtractionConfTypeDef statConf[3];
  ISP_IQParamTypeDef *IQParamConfig;
  ISP_SVC_StatStateTypeDef *ongoing;
  uint32_t i, avgR, avgG, avgB, frameId;

  /* Check handle validity */
  if (hIsp == NULL)
  {
    printf("ERROR: ISP handle is NULL\r\n");
    return;
  }

  if (hIsp->hDcmipp == NULL)
  {
    /* ISP is not initialized
     * This situation happens when the ISP is de-initialized and interrupts still activated.
     */
    return;
  }

  /* Read the stats according to the configuration applied 2 VSYNC (shadow register + stat computation)
   * stages earlier.
   */
  ongoing = &ISP_SVC_StatEngine.ongoing;
  switch(stagePrevious2)
  {
  case ISP_STAT_CFG_UP_AVG:
    HAL_DCMIPP_PIPE_GetISPAccumulatedStatisticsCounter(hIsp->hDcmipp, DCMIPP_PIPE1, DCMIPP_STATEXT_MODULE1, &avgR);
    HAL_DCMIPP_PIPE_GetISPAccumulatedStatisticsCounter(hIsp->hDcmipp, DCMIPP_PIPE1, DCMIPP_STATEXT_MODULE2, &avgG);
    HAL_DCMIPP_PIPE_GetISPAccumulatedStatisticsCounter(hIsp->hDcmipp, DCMIPP_PIPE1, DCMIPP_STATEXT_MODULE3, &avgB);

    ongoing->up.averageR = GetAvgStats(hIsp, ISP_STAT_LOC_UP, ISP_RED, avgR);
    ongoing->up.averageG = GetAvgStats(hIsp, ISP_STAT_LOC_UP, ISP_GREEN, avgG);
    ongoing->up.averageB = GetAvgStats(hIsp, ISP_STAT_LOC_UP, ISP_BLUE, avgB);
    ongoing->up.averageL = LuminanceFromRGB(ongoing->up.averageR, ongoing->up.averageG, ongoing->up.averageB);
    break;

  case ISP_STAT_CFG_UP_BINS_0_2:
    ReadStatHistogram(hIsp, &ongoing->up.histogram[0]);
    break;

  case ISP_STAT_CFG_UP_BINS_3_5:
    ReadStatHistogram(hIsp, &ongoing->up.histogram[3]);
    break;

  case ISP_STAT_CFG_UP_BINS_6_8:
    ReadStatHistogram(hIsp, &ongoing->up.histogram[6]);
    break;

  case ISP_STAT_CFG_UP_BINS_9_11:
    ReadStatHistogram(hIsp, &ongoing->up.histogram[9]);
    break;

  case ISP_STAT_CFG_DOWN_AVG:
    HAL_DCMIPP_PIPE_GetISPAccumulatedStatisticsCounter(hIsp->hDcmipp, DCMIPP_PIPE1, DCMIPP_STATEXT_MODULE1, &avgR);
    HAL_DCMIPP_PIPE_GetISPAccumulatedStatisticsCounter(hIsp->hDcmipp, DCMIPP_PIPE1, DCMIPP_STATEXT_MODULE2, &avgG);
    HAL_DCMIPP_PIPE_GetISPAccumulatedStatisticsCounter(hIsp->hDcmipp, DCMIPP_PIPE1, DCMIPP_STATEXT_MODULE3, &avgB);

    ongoing->down.averageR = GetAvgStats(hIsp, ISP_STAT_LOC_DOWN, ISP_RED, avgR);
    ongoing->down.averageG = GetAvgStats(hIsp, ISP_STAT_LOC_DOWN, ISP_GREEN, avgG);
    ongoing->down.averageB = GetAvgStats(hIsp, ISP_STAT_LOC_DOWN, ISP_BLUE, avgB);
    IQParamConfig = ISP_SVC_IQParam_Get(hIsp);
    if ((hIsp->sensorInfo.bayer_pattern == ISP_DEMOS_TYPE_MONO) || (!IQParamConfig->demosaicing.enable))
    {
      ongoing->down.averageL = LuminanceFromRGBMono(ongoing->down.averageR, ongoing->down.averageG, ongoing->down.averageB);
    }
    else
    {
      ongoing->down.averageL = LuminanceFromRGB(ongoing->down.averageR, ongoing->down.averageG, ongoing->down.averageB);
    }
    break;

  case ISP_STAT_CFG_DOWN_BINS_0_2:
    ReadStatHistogram(hIsp, &ongoing->down.histogram[0]);
    break;

  case ISP_STAT_CFG_DOWN_BINS_3_5:
    ReadStatHistogram(hIsp, &ongoing->down.histogram[3]);
    break;

  case ISP_STAT_CFG_DOWN_BINS_6_8:
    ReadStatHistogram(hIsp, &ongoing->down.histogram[6]);
    break;

  case ISP_STAT_CFG_DOWN_BINS_9_11:
    ReadStatHistogram(hIsp, &ongoing->down.histogram[9]);
    break;

  default:
    /* No Read */
    break;
  }

  /* Configure stat for a new stage */
  switch(ISP_SVC_StatEngine.stage)
  {
  case ISP_STAT_CFG_UP_AVG:
    for (i = 0; i < 3; i++)
    {
      statConf[i].Mode = DCMIPP_STAT_EXT_MODE_AVERAGE;
      statConf[i].Source = avgRGBUp[i];
      statConf[i].Bins = DCMIPP_STAT_EXT_AVER_MODE_ALL_PIXELS;
    }
    break;

  case ISP_STAT_CFG_UP_BINS_0_2:
    SetStatConfig(statConf, &statConfUpBins_0_2);
    break;

  case ISP_STAT_CFG_UP_BINS_3_5:
    SetStatConfig(statConf, &statConfUpBins_3_5);
    break;

  case ISP_STAT_CFG_UP_BINS_6_8:
    SetStatConfig(statConf, &statConfUpBins_6_8);
    break;

  case ISP_STAT_CFG_UP_BINS_9_11:
    SetStatConfig(statConf, &statConfUpBins_9_11);
    break;

  case ISP_STAT_CFG_DOWN_AVG:
    for (i = 0; i < 3; i++)
    {
      statConf[i].Mode = DCMIPP_STAT_EXT_MODE_AVERAGE;
      statConf[i].Source = avgRGBDown[i];
      statConf[i].Bins = DCMIPP_STAT_EXT_AVER_MODE_ALL_PIXELS;
    }
    break;

  case ISP_STAT_CFG_DOWN_BINS_0_2:
    SetStatConfig(statConf, &statConfDownBins_0_2);
    break;

  case ISP_STAT_CFG_DOWN_BINS_3_5:
    SetStatConfig(statConf, &statConfDownBins_3_5);
    break;

  case ISP_STAT_CFG_DOWN_BINS_6_8:
    SetStatConfig(statConf, &statConfDownBins_6_8);
    break;

  case ISP_STAT_CFG_DOWN_BINS_9_11:
    SetStatConfig(statConf, &statConfDownBins_9_11);
    break;

  default:
    /* Configure Unchanged */
    break;
  }

  /* Apply configuration (for an output result available 2 VSYNC later) */
  for (i = DCMIPP_STATEXT_MODULE1; i <= DCMIPP_STATEXT_MODULE3; i++)
  {
    if (HAL_DCMIPP_PIPE_SetISPStatisticExtractionConfig(hIsp->hDcmipp, DCMIPP_PIPE1, i, &statConf[i - DCMIPP_STATEXT_MODULE1]) != HAL_OK)
    {
      printf("ERROR: can't set Statistic Extraction config\r\n");
      return;
    }

    if (HAL_DCMIPP_PIPE_EnableISPStatisticExtraction(hIsp->hDcmipp, DCMIPP_PIPE1, i) != HAL_OK)
    {
      printf("ERROR: can't enable Statistic Extraction config\r\n");
      return;
    }
  }

  /* Cycle start / end */
  frameId = ISP_SVC_Misc_GetMainFrameId(hIsp);

  if (stagePrevious2 == GetStatCycleStart(ISP_STAT_LOC_UP))
  {
    ongoing->upFrameIdStart = frameId;
  }

  if (stagePrevious2 == GetStatCycleStart(ISP_STAT_LOC_DOWN))
  {
    ongoing->downFrameIdStart = frameId;
  }

  if ((stagePrevious2 == GetStatCycleEnd(ISP_STAT_LOC_UP)) && (ongoing->upFrameIdStart != 0))
  {
    /* Last measure of the up cycle : update the 'last' struct */
    ISP_SVC_StatEngine.last.up = ongoing->up;
    ISP_SVC_StatEngine.last.upFrameIdEnd = frameId;
    ISP_SVC_StatEngine.last.upFrameIdStart = ongoing->upFrameIdStart;

    memset(&ongoing->up, 0, sizeof(ongoing->up));
    ongoing->upFrameIdStart = 0;
    ongoing->upFrameIdEnd = 0;
  }

  if ((stagePrevious2 == GetStatCycleEnd(ISP_STAT_LOC_DOWN)) && (ongoing->downFrameIdStart != 0))
  {
    /* Last measure of the down cycle : update the 'last' struct */
    ISP_SVC_StatEngine.last.down = ongoing->down;
    ISP_SVC_StatEngine.last.downFrameIdEnd = frameId;
    ISP_SVC_StatEngine.last.downFrameIdStart = ongoing->downFrameIdStart;

    memset(&ongoing->down, 0, sizeof(ongoing->down));
    ongoing->downFrameIdStart = 0;
    ongoing->downFrameIdEnd = 0;
  }

  if (((ISP_SVC_StatEngine.upRequest & ISP_STAT_TYPE_ALL_TMP) ||
       (ISP_SVC_StatEngine.downRequest & ISP_STAT_TYPE_ALL_TMP)) &&
      (frameId > ISP_SVC_StatEngine.requestAllCounter))
  {
    /* Stop the special temporary mode "request all stats" when its delay expires */
    ISP_SVC_StatEngine.upRequest &= ~ISP_STAT_TYPE_ALL_TMP;
    ISP_SVC_StatEngine.downRequest &= ~ISP_STAT_TYPE_ALL_TMP;
  }

  /* Save the two last processed stages and go to next stage */
  stagePrevious2 = stagePrevious1;
  stagePrevious1 = ISP_SVC_StatEngine.stage;
  ISP_SVC_StatEngine.stage = GetNextStatStage(ISP_SVC_StatEngine.stage);
}

/**
  * @brief  ISP_SVC_Stats_ProcessCallbacks
  *         If the conditions are met, call the client registered callbacks
  * @param  hIsp: ISP device handle
  * @retval ISP status
  */
ISP_StatusTypeDef ISP_SVC_Stats_ProcessCallbacks(ISP_HandleTypeDef *hIsp)
{
  (void)hIsp; /* unused */
  ISP_SVC_StatStateTypeDef *pLastStat;
  ISP_SVC_StatRegisteredClient *client;
  ISP_StatusTypeDef retcb, ret = ISP_OK;

  pLastStat = &ISP_SVC_StatEngine.last;

  for (uint32_t i = 0; i < ISP_SVC_STAT_MAX_CB; i++)
  {
    client = &ISP_SVC_StatEngine.client[i];

    if (client->callback == NULL)
      continue;

    /* Check if stats are available for a client, comparing the location and the specified frameId */
    if (((client->location == ISP_STAT_LOC_DOWN) && (client->refFrameId <= pLastStat->downFrameIdStart)) ||
        ((client->location == ISP_STAT_LOC_UP) && (client->refFrameId <= pLastStat->upFrameIdStart)) ||
        ((client->location == ISP_STAT_LOC_UP_AND_DOWN) && (client->refFrameId <= pLastStat->upFrameIdStart) && (client->refFrameId <= pLastStat->downFrameIdStart)))
    {
      /* Copy the stats into the client buffer */
      *(client->pStats) = *pLastStat;

      /* Call its callback */
      retcb = client->callback(client->pAlgo);
      if (retcb != ISP_OK)
      {
        ret = retcb;
      }

      /* Remove the client from the registered list */
      client->callback = NULL;
    }
  }

  return ret;
}

/**
  * @brief  ISP_SVC_Stats_GetLatest
  *         Get the latest available stats. Stats are immediately returned (no callback)
  * @param  hIsp: ISP device handle
  * @param  pStats: pointer to the latest computed statistics (output parameter)
  * @retval ISP status
  */
ISP_StatusTypeDef ISP_SVC_Stats_GetLatest(ISP_HandleTypeDef *hIsp, ISP_SVC_StatStateTypeDef *pStats)
{
  /* Check handle validity */
  if ((hIsp == NULL) || (pStats == NULL))
  {
    return ISP_ERR_EINVAL;
  }

  *pStats = ISP_SVC_StatEngine.last;

  return ISP_OK;
}

/**
  * @brief  ISP_SVC_Stats_GetNext
  *         Ask to be informed with a callback when stats meeting some conditions(location, type
  *         and frameDelay) are available.
  * @param  hIsp: ISP device handle
  * @param  callback: function to be called when stats are available
  * @param  pAlgo: pointer to the algorithm context. Will be provided as the callback parameter.
  * @param  pStats: pointer to the statistics. Updated just before the callback is called.
  * @param  location: location (up and/or down) where the statistics are requested.
  * @param  type: type (average, bins or both) of requested statistics.
  * @param  frameDelay: number of frames to wait before considering the stats as valid
  * @retval ISP status
  */
ISP_StatusTypeDef ISP_SVC_Stats_GetNext(ISP_HandleTypeDef *hIsp, ISP_stat_ready_cb callback, ISP_AlgoTypeDef *pAlgo, ISP_SVC_StatStateTypeDef *pStats,
                                        ISP_SVC_StatLocation location, ISP_SVC_StatType type, uint32_t frameDelay)
{
  uint32_t i, refFrameId;

  /* Check handle validity */
  if ((hIsp == NULL) || (pStats == NULL))
  {
    return ISP_ERR_EINVAL;
  }

  refFrameId = ISP_SVC_Misc_GetMainFrameId(hIsp) + frameDelay;

  /* Register the callback */
  for (i = 0; i < ISP_SVC_STAT_MAX_CB; i++)
  {
    if (ISP_SVC_StatEngine.client[i].callback == NULL)
      break;
  }

  if (i == ISP_SVC_STAT_MAX_CB)
  {
    /* Too much callback registered */
    return ISP_ERR_STAT_MAXCLIENTS;
  }

  /* Add this requested stat to the list of requested stats */
  if (location & ISP_STAT_LOC_UP)
  {
    ISP_SVC_StatEngine.upRequest |= type;
  }
  if (location & ISP_STAT_LOC_DOWN)
  {
    ISP_SVC_StatEngine.downRequest |= type;
  }

  if (type == ISP_STAT_TYPE_ALL_TMP)
  {
    /* Special case: request all stats for a short time (3 cycle) */
    ISP_SVC_StatEngine.requestAllCounter = ISP_SVC_Misc_GetMainFrameId(hIsp) + 3 * ISP_STAT_CFG_CYCLE_SIZE;
  }

  /* Register client */
  ISP_SVC_StatEngine.client[i].callback = callback;
  ISP_SVC_StatEngine.client[i].pAlgo = pAlgo;
  ISP_SVC_StatEngine.client[i].pStats = pStats;
  ISP_SVC_StatEngine.client[i].location = location;
  ISP_SVC_StatEngine.client[i].type = type;
  ISP_SVC_StatEngine.client[i].refFrameId = refFrameId;

  return ISP_OK;
}
