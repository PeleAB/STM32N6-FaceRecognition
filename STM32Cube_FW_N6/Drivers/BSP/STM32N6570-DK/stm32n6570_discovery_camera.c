/**
  ******************************************************************************
  * @file    stm32n6570_discovery_camera.c
  * @author  MCD Application Team
  * @brief   This file includes the driver for Camera modules mounted on
  *          STM32N6570-DK board.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* File Info: ------------------------------------------------------------------
                                   User NOTES
1. How to use this driver:
--------------------------
   - This driver is used to drive the camera.
   - The IMX335 component driver MUST be included with this driver.
   - The STM32 ISP Middleware MUST be included with this driver

2. Driver description:
---------------------
     o Initialize the camera instance using the BSP_CAMERA_Init() function with the required
       Resolution and Pixel format where:
       - Instance: Is the physical pipe interface and it can be one from the available pipes on this board.
       - Resolution: The camera resolution
       - PixelFormat: The camera Pixel format

     o DeInitialize the camera instance using the BSP_CAMERA_DeInit() . This
       function will firstly stop the camera to insure the data transfer complete.
       Then de-initializes the dcmi peripheral.

     o Get the camera instance capabilities using the BSP_CAMERA_GetCapabilities().
       This function must be called after the BSP_CAMERA_Init() to get the sensor capabilities

     o Start the camera using the CAMERA_Start() function by specifying the capture Mode:
       - CAMERA_MODE_CONTINUOUS: For continuous capture
       - CAMERA_MODE_SNAPSHOT  : For on shot capture

     o Suspend, resume or stop the camera capture using the following functions:
      - BSP_CAMERA_Suspend()
      - BSP_CAMERA_Resume()
      - BSP_CAMERA_Stop()

     o Call BSP_CAMERA_SetResolution()/BSP_CAMERA_GetResolution() to set/get the camera resolution
       Resolution: - CAMERA_R2592x1940

     o Call BSP_CAMERA_SetPixelFormat()/BSP_CAMERA_GetPixelFormat() to set/get the camera pixel format
       PixelFormat: - CAMERA_PF_RAW_RGGB10

     o Call BSP_CAMERA_SetLightMode()/BSP_CAMERA_GetLightMode() to set/get the camera light mode
       LightMode: - CAMERA_LIGHT_AUTO
                  - CAMERA_LIGHT_SUNNY
                  - CAMERA_LIGHT_OFFICE
                  - CAMERA_LIGHT_HOME
                  - CAMERA_LIGHT_CLOUDY
      Note that this feature is unavailable for the IMX335.

     o Call BSP_CAMERA_SetColorEffect()/BSP_CAMERA_GetColorEffect() to set/get the camera color effects
       Effect: - CAMERA_COLOR_EFFECT_NONE
               - CAMERA_COLOR_EFFECT_BLUE
               - CAMERA_COLOR_EFFECT_RED
               - CAMERA_COLOR_EFFECT_GREEN
               - CAMERA_COLOR_EFFECT_BW
               - CAMERA_COLOR_EFFECT_SEPIA
               - CAMERA_COLOR_EFFECT_NEGATIVE
      Note that this feature is unavailable for the IMX335.

     o Call BSP_CAMERA_SetBrightness()/BSP_CAMERA_GetBrightness() to set/get the camera Brightness
       Brightness is value between -4(Level 4 negative) and 4(Level 4 positive).
       Note that this feature is unavailable for the IMX335.

     o Call BSP_CAMERA_SetSaturation()/BSP_CAMERA_GetSaturation() to set/get the camera Saturation
       Saturation is value between -4(Level 4 negative) and 4(Level 4 positive).
       Note that this feature is unavailable for the IMX335.

     o Call BSP_CAMERA_SetContrast()/BSP_CAMERA_GetContrast() to set/get the camera Contrast
       Contrast is value between -4(Level 4 negative) and 4(Level 4 positive).
       Note that this feature is unavailable for the IMX335.

     o Call BSP_CAMERA_SetHueDegree()/BSP_CAMERA_GetHueDegree() to set/get the camera Hue Degree
       HueDegree is value between -4(180 degree negative) and 4(150 degree positive).
       Note that this feature is unavailable for the IMX335.

     o Call BSP_CAMERA_SetMirrorFlip()/BSP_CAMERA_GetMirrorFlip() to set/get the camera mirror and flip
       MirrorFlip could be any combination of: - CAMERA_MIRRORFLIP_NONE
                                               - CAMERA_MIRRORFLIP_FLIP
                                               - CAMERA_MIRRORFLIP_MIRROR

     o Call BSP_CAMERA_SetZoom()/BSP_CAMERA_GetZoom() to set/get the camera zooming
       Zoom could be any value of:
       - CAMERA_ZOOM_x8
       - CAMERA_ZOOM_x4
       - CAMERA_ZOOM_x2
       - CAMERA_ZOOM_x1
       Note that this feature is unavailable for the IMX335.

     o Call BSP_CAMERA_EnableNightMode() to enable night mode.
      Note that this feature is unavailable for the IMX335.

     o Call BSP_CAMERA_DisableNightMode() to disable night mode.
      Note that this feature is unavailable for the IMX335.

     o Call BSP_CAMERA_RegisterDefaultMspCallbacks() to register Msp default callbacks
     o Call BSP_CAMERA_RegisterMspCallbacks() to register application Msp callbacks.

     o Error, line event, vsync event and frame event are handled through dedicated weak
      callbacks that can be override at application level: BSP_CAMERA_LineEventCallback(),
      BSP_CAMERA_FrameEventCallback(), BSP_CAMERA_VsyncEventCallback(), BSP_CAMERA_ErrorCallback()

     o BSP_CAMERA_BackgroundProcess() calls the ISP background process function and should be called regularly
     (inside the main function or a thread) to compute and apply the necessary ISP configuration.
------------------------------------------------------------------------------*/

/* Includes ------------------------------------------------------------------*/
#include "stm32n6570_discovery_camera.h"
#include "stm32n6570_discovery_bus.h"
#include "isp_param_conf.h"
/** @addtogroup BSP
  * @{
  */

/** @addtogroup STM32N6570-DK
  * @{
  */

/** @addtogroup STM32N6570-DK_CAMERA
  * @{
  */

/** @defgroup STM32N6570-DK_CAMERA_Exported_Variables CAMERA Exported Variables
  * @{
  */
void                *Camera_CompObj = NULL;
DCMIPP_HandleTypeDef hcamera_dcmipp;
CAMERA_Ctx_t         Camera_Ctx[CAMERA_INSTANCES_NBR];
ISP_HandleTypeDef    hcamera_isp;
/**
  * @}
  */

/** @defgroup STM32N6570-DK_CAMERA_Private_Variables CAMERA Private Variables
  * @{
  */
static CAMERA_Drv_t *Camera_Drv = NULL;
static CAMERA_Capabilities_t Camera_Cap;
static int32_t isp_gain;
static int32_t isp_exposure;

/**
  * @}
  */

/** @defgroup STM32N6570-DK_CAMERA_Private_FunctionsPrototypes CAMERA Private Functions Prototypes
  * @{
  */
static void DCMIPP_MspInit(const DCMIPP_HandleTypeDef *hdcmipp);
static void DCMIPP_MspDeInit(const DCMIPP_HandleTypeDef *hdcmipp);

#if (USE_HAL_DCMIPP_REGISTER_CALLBACKS > 0)
static void DCMIPP_PIPE_LineEventCallback(DCMIPP_HandleTypeDef *hdcmipp, uint32_t Pipe);
static void DCMIPP_PIPE_FrameEventCallback(DCMIPP_HandleTypeDef *hdcmipp, uint32_t Pipe);
static void DCMIPP_PIPE_VsyncEventCallback(DCMIPP_HandleTypeDef *hdcmipp, uint32_t Pipe);
static void DCMIPP_PIPE_ErrorCallback(DCMIPP_HandleTypeDef *hdcmipp, uint32_t Pipe);
static void DCMIPP_ErrorCallback(DCMIPP_HandleTypeDef *hdcmipp);
#endif /* (USE_HAL_DCMIPP_REGISTER_CALLBACKS > 0) */

static ISP_StatusTypeDef BSP_GetSensorInfoHelper(uint32_t Instance, ISP_SensorInfoTypeDef *SensorInfo);
static ISP_StatusTypeDef BSP_SetSensorGainHelper(uint32_t Instance, int32_t Gain);
static ISP_StatusTypeDef BSP_GetSensorGainHelper(uint32_t Instance, int32_t *Gain);
static ISP_StatusTypeDef BSP_SetSensorExposureHelper(uint32_t Instance, int32_t Exposure);
static ISP_StatusTypeDef BSP_GetSensorExposureHelper(uint32_t Instance, int32_t *Exposure);

static int32_t IMX335_Probe(uint32_t Resolution, uint32_t PixelFormat);

/**
  * @}
  */

/** @defgroup STM32N6570-DK_CAMERA_Exported_Functions CAMERA Exported Functions
  * @{
  */

/**
  * @brief  Initializes the camera.
  * @param  Instance    Camera instance.
  * @param  Resolution  Camera sensor requested resolution (x, y) : standard resolution
  *         naming QQVGA, QVGA, VGA ...
  * @param  PixelFormat Capture pixel format
  * @retval BSP status
  */
int32_t BSP_CAMERA_Init(uint32_t Instance, uint32_t Resolution, uint32_t PixelFormat)
{
  int32_t ret = BSP_ERROR_NONE;
  ISP_AppliHelpersTypeDef appliHelpers = {0};
  ISP_StatAreaTypeDef statArea = {0};
  if (Instance >= CAMERA_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if ((PixelFormat != CAMERA_PF_RAW_RGGB10) || (Resolution != CAMERA_R2592x1944))
    {
      ret = BSP_ERROR_WRONG_PARAM;
    }

    else
    {
      /* Check if another instance was Initialized */
      Camera_Ctx[Instance].Resolution = Resolution;
      Camera_Ctx[Instance].PixelFormat = PixelFormat;

      /* Set DCMIPP instance */
      hcamera_dcmipp.Instance = DCMIPP;

      /* DCMIPP Initialization */
#if (USE_HAL_DCMIPP_REGISTER_CALLBACKS > 0)
      /* Register the DCMIPP MSP Callbacks */
      if (Camera_Ctx[Instance].IsMspCallbacksValid == 0U)
      {
        if (BSP_CAMERA_RegisterDefaultMspCallbacks(Instance) != BSP_ERROR_NONE)
        {
          return BSP_ERROR_MSP_FAILURE;
        }
      }
#else
      /* DCMIPP Initialization */
      DCMIPP_MspInit(&hcamera_dcmipp);
#endif /* USE_HAL_DCMIPP_REGISTER_CALLBACKS */
      if(MX_DCMIPP_ClockConfig(&hcamera_dcmipp) != HAL_OK)
      {
        ret = BSP_ERROR_PERIPH_FAILURE;
      }
      else if (BSP_CAMERA_HwReset(0) != BSP_ERROR_NONE)
      {
        ret = BSP_ERROR_BUS_FAILURE;
      }
      else
      {
        /* No action */
      }

      if(ret == BSP_ERROR_NONE)
      {
        if (MX_DCMIPP_Init(&hcamera_dcmipp) != HAL_OK)
        {
          ret = BSP_ERROR_PERIPH_FAILURE;
        }
        else
        {
          if (IMX335_Probe(Resolution, PixelFormat) != BSP_ERROR_NONE)
          {
            ret = BSP_ERROR_UNKNOWN_COMPONENT;
          }
          else
          {
#if (USE_HAL_DCMIPP_REGISTER_CALLBACKS > 0)
            /* Register DCMIPP LineEvent, FrameEvent and Error callbacks */
            if (HAL_DCMIPP_PIPE_RegisterCallback(&hcamera_dcmipp, HAL_DCMIPP_PIPE_LINE_EVENT_CB_ID, DCMIPP_PIPE_LineEventCallback) != HAL_OK)
            {
              ret = BSP_ERROR_PERIPH_FAILURE;
            }
            else if (HAL_DCMIPP_PIPE_RegisterCallback(&hcamera_dcmipp, HAL_DCMIPP_PIPE_FRAME_EVENT_CB_ID, DCMIPP_PIPE_FrameEventCallback) != HAL_OK)
            {
              ret = BSP_ERROR_PERIPH_FAILURE;
            }
            else if (HAL_DCMIPP_PIPE_RegisterCallback(&hcamera_dcmipp, HAL_DCMIPP_PIPE_VSYNC_EVENT_CB_ID, DCMIPP_PIPE_VsyncEventCallback) != HAL_OK)
            {
              ret = BSP_ERROR_PERIPH_FAILURE;
            }
            else if (HAL_DCMIPP_PIPE_RegisterCallback(&hcamera_dcmipp, HAL_DCMIPP_PIPE_ERROR_CB_ID, DCMIPP_PIPE_ErrorCallback) != HAL_OK)
            {
              ret = BSP_ERROR_PERIPH_FAILURE;
            }
            else if (HAL_DCMIPP_RegisterCallback(&hcamera_dcmipp, HAL_DCMIPP_ERROR_CB_ID, DCMIPP_ErrorCallback) != HAL_OK)
            {
              ret = BSP_ERROR_PERIPH_FAILURE;
            }
            else
            {
#endif /* (USE_HAL_DCMIPP_REGISTER_CALLBACKS > 0) */
              /* Fill init struct with Camera driver helpers */
              appliHelpers.GetSensorInfo = BSP_GetSensorInfoHelper;
              appliHelpers.SetSensorGain = BSP_SetSensorGainHelper;
              appliHelpers.GetSensorGain = BSP_GetSensorGainHelper;
              appliHelpers.SetSensorExposure = BSP_SetSensorExposureHelper;
              appliHelpers.GetSensorExposure = BSP_GetSensorExposureHelper;

              statArea.X0 = 0;
              statArea.Y0 = 0;
              statArea.XSize = 2592;
              statArea.YSize = 1944;
              /* Initialize the Image Signal Processing middleware */
              if(ISP_Init(&hcamera_isp, &hcamera_dcmipp, 0, &appliHelpers, &statArea, ISP_IQParamCacheInit[0]) != ISP_OK)
              {
                ret = BSP_ERROR_PERIPH_FAILURE;
              }
              else
              {
                ret = BSP_ERROR_NONE;
              }
#if (USE_HAL_DCMIPP_REGISTER_CALLBACKS > 0)
            }
#endif /* (USE_HAL_DCMIPP_REGISTER_CALLBACKS > 0) */
          }
        }
      }
    }
  }

  /* BSP status */
  return ret;
}

/**
  * @brief  DeInitializes the camera.
  * @param  Instance Camera instance.
  * @retval BSP status
  */
int32_t BSP_CAMERA_DeInit(uint32_t Instance)
{
  int32_t ret;

  if (Instance >= CAMERA_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {

    hcamera_dcmipp.Instance = DCMIPP;

    /* First stop the camera to insure all data are transferred */
    if (BSP_CAMERA_Stop(Instance) != BSP_ERROR_NONE)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else if (HAL_DCMIPP_DeInit(&hcamera_dcmipp) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else
    {
#if (USE_HAL_DCMIPP_REGISTER_CALLBACKS == 0)
        DCMIPP_MspDeInit(&hcamera_dcmipp);
#endif /* (USE_HAL_DCMIPP_REGISTER_CALLBACKS == 0) */

      /* De-initialize the camera module */
      if (Camera_Drv->DeInit(Camera_CompObj) != IMX335_OK)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
      /* Set Camera in Power Down */
      else if (BSP_CAMERA_PwrDown(Instance) != BSP_ERROR_NONE)
      {
        ret = BSP_ERROR_BUS_FAILURE;
      }
      else
      {
        if(ISP_DeInit(&hcamera_isp) != ISP_OK)
        {
          ret = BSP_ERROR_COMPONENT_FAILURE;
        }
        else
        {
          ret = BSP_ERROR_NONE;
        }
      }
    }
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Initializes the DCMIPP peripheral
  * @param  hdcmipp  DCMIPP handle
  * @note   Being __weak it can be overwritten by the application
  * @retval HAL status
  */
__weak HAL_StatusTypeDef MX_DCMIPP_Init(DCMIPP_HandleTypeDef *hdcmipp)
{
  DCMIPP_PipeConfTypeDef pPipeConf = {0};
  DCMIPP_CSI_PIPE_ConfTypeDef pCSIPipeConf = {0};
  DCMIPP_CSI_ConfTypeDef csiconf = {0};
  DCMIPP_DownsizeTypeDef DonwsizeConf ={0};

  if (HAL_DCMIPP_Init(hdcmipp) != HAL_OK)
  {
    return HAL_ERROR;
  }

  /* Configure the CSI */
  csiconf.DataLaneMapping = DCMIPP_CSI_PHYSICAL_DATA_LANES;
  csiconf.NumberOfLanes   = DCMIPP_CSI_TWO_DATA_LANES;
  csiconf.PHYBitrate      = DCMIPP_CSI_PHY_BT_1600;
  if(HAL_DCMIPP_CSI_SetConfig(hdcmipp, &csiconf) != HAL_OK)
  {
    return HAL_ERROR;
  }
  /* Configure the Virtual Channel 0 */
  /* Set Virtual Channel config */
  if(HAL_DCMIPP_CSI_SetVCConfig(hdcmipp, DCMIPP_VIRTUAL_CHANNEL0, DCMIPP_CSI_DT_BPP10) != HAL_OK)
  {
    return HAL_ERROR;
  }

  /* Configure the serial Pipe */
  pCSIPipeConf.DataTypeMode = DCMIPP_DTMODE_DTIDA;
  pCSIPipeConf.DataTypeIDA  = DCMIPP_DT_RAW10;
  pCSIPipeConf.DataTypeIDB  = DCMIPP_DT_RAW10; /* Don't Care */


  if (HAL_DCMIPP_CSI_PIPE_SetConfig(hdcmipp, DCMIPP_PIPE1, &pCSIPipeConf) != HAL_OK)
  {
    return HAL_ERROR;
  }

  pPipeConf.FrameRate  = DCMIPP_FRAME_RATE_ALL;
  pPipeConf.PixelPackerFormat = DCMIPP_PIXEL_PACKER_FORMAT_RGB565_1;

  /* Set Pitch for Main and Ancillary Pipes */
  pPipeConf.PixelPipePitch  = 1600 ; /* Number of bytes */

  /* Configure Pipe */
  if (HAL_DCMIPP_PIPE_SetConfig(hdcmipp, DCMIPP_PIPE1, &pPipeConf) != HAL_OK)
  {
    return HAL_ERROR;
  }

  /* Configure the downsize */
  DonwsizeConf.HRatio      = 25656;
  DonwsizeConf.VRatio      = 33161;
  DonwsizeConf.HSize       = 800;
  DonwsizeConf.VSize       = 480;
  DonwsizeConf.HDivFactor  = 316;
  DonwsizeConf.VDivFactor  = 253;

  if(HAL_DCMIPP_PIPE_SetDownsizeConfig(hdcmipp, DCMIPP_PIPE1, &DonwsizeConf) != HAL_OK)
  {
    return HAL_ERROR;
  }
  if(HAL_DCMIPP_PIPE_EnableDownsize(hdcmipp, DCMIPP_PIPE1) != HAL_OK)
  {
    return HAL_ERROR;
  }

  return HAL_OK;
}

#if (USE_HAL_DCMIPP_REGISTER_CALLBACKS > 0)
/* Register-UnRegister Callback through BSP not yet supported */
/**
  * @brief Default BSP CAMERA Msp Callbacks
  * @param Instance CAMERA Instance.
  * @retval BSP status
  */
int32_t BSP_CAMERA_RegisterDefaultMspCallbacks(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  if (Instance >= CAMERA_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    /* Register MspInit/MspDeInit Callbacks */
    if (HAL_DCMIPP_RegisterCallback(&hcamera_dcmipp, HAL_DCMIPP_MSPINIT_CB_ID, ((pDCMIPP_CallbackTypeDef) DCMIPP_MspInit)) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else if (HAL_DCMIPP_RegisterCallback(&hcamera_dcmipp, HAL_DCMIPP_MSPDEINIT_CB_ID, ((pDCMIPP_CallbackTypeDef) DCMIPP_MspDeInit)) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else
    {
      Camera_Ctx[Instance].IsMspCallbacksValid = 1;
    }
  }
  /* Return BSP status */
  return ret;
}

/**
  * @brief BSP CAMERA Msp Callback registering
  * @param Instance     CAMERA Instance.
  * @param CallBacks    pointer to MspInit/MspDeInit callbacks functions
  * @retval BSP status
  */
int32_t BSP_CAMERA_RegisterMspCallbacks(uint32_t Instance, BSP_CAMERA_Cb_t *CallBacks)
{
  int32_t ret = BSP_ERROR_NONE;

  if (Instance >= CAMERA_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    /* Register MspInit/MspDeInit Callbacks */
    if (HAL_DCMIPP_RegisterCallback(&hcamera_dcmipp, HAL_DCMIPP_MSPINIT_CB_ID, CallBacks->pMspInitCb) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else if (HAL_DCMIPP_RegisterCallback(&hcamera_dcmipp, HAL_DCMIPP_MSPDEINIT_CB_ID, CallBacks->pMspDeInitCb) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else
    {
      Camera_Ctx[Instance].IsMspCallbacksValid = 1;
    }
  }
  /* Return BSP status */
  return ret;
}
#endif /* (USE_HAL_DCMIPP_REGISTER_CALLBACKS > 0) */

/**
  * @brief  DCMIPP Clock Config for DCMIPP.
  * @param  hdcmipp  DCMIPP Handle
  *         Being __weak it can be overwritten by the application
  * @retval HAL_status
  */
__weak HAL_StatusTypeDef MX_DCMIPP_ClockConfig(DCMIPP_HandleTypeDef *hdcmipp)
{
  UNUSED(hdcmipp);
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
  /* DCMIPP Clock Config */
  /* DCMIPP clock configuration */
  /* Typical PCLK is 333 MHz so the PLL1 is configured to provide this clock */
  /* Configure DCMIPP clock to IC17 with PLL1  */
  /* PLL1_VCO Input = HSI_VALUE/PLLM = 64 Mhz / 4 = 16 */
  /* PLL1_VCO Output = PLL3_VCO Input * PLLN = 16 Mhz * 75 = 1200 */
  /* PLLLCDCLK = PLL3_VCO Output/(PLLP1 * PLLP2) = 1200/4 = 300Mhz */
  /* DCMIPP clock frequency = PLLLCDCLK = 300 Mhz */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_DCMIPP;
  PeriphClkInitStruct.DcmippClockSelection = RCC_DCMIPPCLKSOURCE_IC17;
  PeriphClkInitStruct.ICSelection[RCC_IC17].ClockSelection = RCC_ICCLKSOURCE_PLL1;
  PeriphClkInitStruct.ICSelection[RCC_IC17].ClockDivider = 4;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    return HAL_ERROR;
  }

  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_CSI;
  PeriphClkInitStruct.ICSelection[RCC_IC18].ClockSelection = RCC_ICCLKSOURCE_PLL1;
  PeriphClkInitStruct.ICSelection[RCC_IC18].ClockDivider = 60;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    return HAL_ERROR;
  }

  return HAL_OK;
}

/**
  * @brief  Starts the camera capture in the selected mode.
  * @param  Instance Camera instance.
  * @param  pbuff     pointer to the camera output buffer
  * @param  Mode CAMERA_MODE_CONTINUOUS or CAMERA_MODE_SNAPSHOT
  * @retval BSP status
  */
int32_t BSP_CAMERA_Start(uint32_t Instance, uint8_t *pbuff, uint32_t Mode)
{
  int32_t ret = BSP_ERROR_NONE;

  if (Instance >= CAMERA_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if (HAL_DCMIPP_CSI_PIPE_Start(&hcamera_dcmipp, DCMIPP_PIPE1, DCMIPP_VIRTUAL_CHANNEL0 , (uint32_t)pbuff, Mode) != HAL_OK)
  {
    ret = BSP_ERROR_PERIPH_FAILURE;
  }
  else
  {
    /* No action */
  }
  /* Start the Image Signal Processing */
  if (ISP_Start(&hcamera_isp) != ISP_OK)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  /* Return BSP status */
  return ret;
}

/**
  * @brief  Starts the camera capture in selected mode.
  * @param  Instance Camera instance.
  * @param  pbuff1     pointer to the camera first output buffer
  * @param  pbuff2     pointer to the camera second output buffer
  * @param  Mode CAMERA_MODE_CONTINUOUS or CAMERA_MODE_SNAPSHOT
  * @retval BSP status
  */
int32_t BSP_CAMERA_DoubleBufferStart(uint32_t Instance, uint8_t *pbuff1, uint8_t *pbuff2, uint32_t Mode)
{
  int32_t ret = BSP_ERROR_NONE;

  if (Instance >= CAMERA_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if (HAL_DCMIPP_CSI_PIPE_DoubleBufferStart(&hcamera_dcmipp, DCMIPP_PIPE1,DCMIPP_VIRTUAL_CHANNEL0, (uint32_t)pbuff1, (uint32_t)pbuff2, Mode) != HAL_OK)
  {
    ret = BSP_ERROR_PERIPH_FAILURE;
  }
  else
  {
    /* No action */
  }
  /* Start the Image Signal Processing */
  if (ISP_Start(&hcamera_isp) != ISP_OK)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  /* Return BSP status */
  return ret;
}

/**
  * @brief  Suspend the camera capture.
  * @param  Instance Camera instance.
  * @retval BSP status
  */
int32_t BSP_CAMERA_Suspend(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  if (Instance >= CAMERA_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if (HAL_DCMIPP_PIPE_Suspend(&hcamera_dcmipp, DCMIPP_PIPE1) != HAL_OK)
  {
    ret = BSP_ERROR_PERIPH_FAILURE;
  }
  else
  {
    /* No action */
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Resume the camera capture.
  * @param  Instance Camera instance.
  * @retval BSP status
  */
int32_t BSP_CAMERA_Resume(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  if (Instance >= CAMERA_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if (HAL_DCMIPP_PIPE_Resume(&hcamera_dcmipp, DCMIPP_PIPE1) != HAL_OK)
  {
    ret = BSP_ERROR_PERIPH_FAILURE;
  }
  else
  {
    /* No action */
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Stop the CAMERA capture
  * @param  Instance Camera instance.
  * @retval BSP status
  */
int32_t BSP_CAMERA_Stop(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  if (Instance >= CAMERA_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if (HAL_DCMIPP_CSI_PIPE_Stop(&hcamera_dcmipp, DCMIPP_PIPE1, DCMIPP_VIRTUAL_CHANNEL0) != HAL_OK)
  {
    ret = BSP_ERROR_PERIPH_FAILURE;
  }
  else
  {
    /* No action */
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Get the Camera Capabilities.
  * @param  Instance  Camera instance.
  * @param  Capabilities  pointer to camera Capabilities
  * @note   This function should be called after the init. This to get Capabilities
  *         from the camera sensor IMX335
  * @retval Component status
  */
int32_t BSP_CAMERA_GetCapabilities(uint32_t Instance, CAMERA_Capabilities_t *Capabilities)
{
  int32_t ret;

  if (Instance >= CAMERA_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if (Camera_Drv->GetCapabilities(Camera_CompObj, Capabilities) < 0)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  else
  {
    ret = BSP_ERROR_NONE;
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Set the camera pixel format.
  * @param  Instance  Camera instance.
  * @param  PixelFormat pixel format to be configured
  * @retval BSP status
  */
int32_t BSP_CAMERA_SetPixelFormat(uint32_t Instance, uint32_t PixelFormat)
{
  int32_t ret;

  if (Instance >= CAMERA_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if (Camera_Drv->SetPixelFormat(Camera_CompObj, PixelFormat) < 0)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  else
  {
    Camera_Ctx[Instance].PixelFormat = PixelFormat;
    ret = BSP_ERROR_NONE;
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Get the camera pixel format.
  * @param  Instance  Camera instance.
  * @param  PixelFormat pixel format to be returned
  * @retval BSP status
  */
int32_t BSP_CAMERA_GetPixelFormat(uint32_t Instance, uint32_t *PixelFormat)
{
  int32_t ret;

  if (Instance >= CAMERA_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    *PixelFormat = Camera_Ctx[Instance].PixelFormat;
    ret = BSP_ERROR_NONE;
  }

  /* Return BSP status */
  return ret;
}


/**
  * @brief  Set the camera Resolution.
  * @param  Instance  Camera instance.
  * @param  Resolution Resolution to be configured
  * @retval BSP status
  */
int32_t BSP_CAMERA_SetResolution(uint32_t Instance, uint32_t Resolution)
{
  int32_t ret;

  if (Instance >= CAMERA_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if (Camera_Cap.Resolution == 0U)
  {
    ret = BSP_ERROR_FEATURE_NOT_SUPPORTED;
  }
  else if (Camera_Drv->SetResolution(Camera_CompObj, Resolution) < 0)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  else
  {
    Camera_Ctx[Instance].Resolution = Resolution;
    ret = BSP_ERROR_NONE;
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Get the camera Resolution.
  * @param  Instance  Camera instance.
  * @param  Resolution Resolution to be returned
  * @retval BSP status
  */
int32_t BSP_CAMERA_GetResolution(uint32_t Instance, uint32_t *Resolution)
{
  int32_t ret;

  if (Instance >= CAMERA_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if (Camera_Cap.Resolution == 0U)
  {
    ret = BSP_ERROR_FEATURE_NOT_SUPPORTED;
  }
  else
  {
    *Resolution = Camera_Ctx[Instance].Resolution;
    ret = BSP_ERROR_NONE;
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Set the camera Light Mode.
  * @param  Instance  Camera instance.
  * @param  LightMode Light Mode to be configured
  * @retval BSP status
  */
int32_t BSP_CAMERA_SetLightMode(uint32_t Instance, uint32_t LightMode)
{
  int32_t ret;

  if (Instance >= CAMERA_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if (Camera_Cap.LightMode == 0U)
  {
    ret = BSP_ERROR_FEATURE_NOT_SUPPORTED;
  }
  else if (Camera_Drv->SetLightMode(Camera_CompObj, LightMode) < 0)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  else
  {
    Camera_Ctx[Instance].LightMode = LightMode;
    ret = BSP_ERROR_NONE;
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Get the camera Light Mode.
  * @param  Instance  Camera instance.
  * @param  LightMode Light Mode to be returned
  * @retval BSP status
  */
int32_t BSP_CAMERA_GetLightMode(uint32_t Instance, uint32_t *LightMode)
{
  int32_t ret;

  if (Instance >= CAMERA_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if (Camera_Cap.LightMode == 0U)
  {
    ret = BSP_ERROR_FEATURE_NOT_SUPPORTED;
  }
  else
  {
    *LightMode = Camera_Ctx[Instance].LightMode;
    ret = BSP_ERROR_NONE;
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Set the camera Special Effect.
  * @param  Instance Camera instance.
  * @param  ColorEffect Effect to be configured
  * @retval BSP status
  */
int32_t BSP_CAMERA_SetColorEffect(uint32_t Instance, uint32_t ColorEffect)
{
  int32_t ret;

  if (Instance >= CAMERA_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if (Camera_Cap.ColorEffect == 0U)
  {
    ret = BSP_ERROR_FEATURE_NOT_SUPPORTED;
  }
  else if (Camera_Drv->SetColorEffect(Camera_CompObj, ColorEffect) < 0)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  else
  {
    Camera_Ctx[Instance].ColorEffect = ColorEffect;
    ret = BSP_ERROR_NONE;
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Get the camera Special Effect.
  * @param  Instance Camera instance.
  * @param  ColorEffect Effect to be returned
  * @retval BSP status
  */
int32_t BSP_CAMERA_GetColorEffect(uint32_t Instance, uint32_t *ColorEffect)
{
  int32_t ret;

  if (Instance >= CAMERA_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if (Camera_Cap.ColorEffect == 0U)
  {
    ret = BSP_ERROR_FEATURE_NOT_SUPPORTED;
  }
  else
  {
    *ColorEffect = Camera_Ctx[Instance].ColorEffect;
    ret = BSP_ERROR_NONE;
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Set the camera Brightness Level.
  * @param  Instance   Camera instance.
  * @param  Brightness Brightness Level
  * @retval BSP status
  */
int32_t BSP_CAMERA_SetBrightness(uint32_t Instance, int32_t Brightness)
{
  int32_t ret;

  if ((Instance >= CAMERA_INSTANCES_NBR) || ((Brightness < CAMERA_BRIGHTNESS_MIN)
                                             && (Brightness > CAMERA_BRIGHTNESS_MAX)))
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if (Camera_Cap.Brightness == 0U)
  {
    ret = BSP_ERROR_FEATURE_NOT_SUPPORTED;
  }
  else if (Camera_Drv->SetBrightness(Camera_CompObj, Brightness) < 0)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  else
  {
    Camera_Ctx[Instance].Brightness = Brightness;
    ret = BSP_ERROR_NONE;
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Get the camera Brightness Level.
  * @param  Instance Camera instance.
  * @param  Brightness  Brightness Level
  * @retval BSP status
  */
int32_t BSP_CAMERA_GetBrightness(uint32_t Instance, int32_t *Brightness)
{
  int32_t ret;

  if (Instance >= CAMERA_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if (Camera_Cap.Brightness == 0U)
  {
    ret = BSP_ERROR_FEATURE_NOT_SUPPORTED;
  }
  else
  {
    *Brightness = Camera_Ctx[Instance].Brightness;
    ret = BSP_ERROR_NONE;
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Set the camera Saturation Level.
  * @param  Instance    Camera instance.
  * @param  Saturation  Saturation Level
  * @retval BSP status
  */
int32_t BSP_CAMERA_SetSaturation(uint32_t Instance, int32_t Saturation)
{
  int32_t ret;

  if ((Instance >= CAMERA_INSTANCES_NBR) || ((Saturation < CAMERA_SATURATION_MIN)
                                             && (Saturation > CAMERA_SATURATION_MAX)))
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if (Camera_Cap.Saturation == 0U)
  {
    ret = BSP_ERROR_FEATURE_NOT_SUPPORTED;
  }
  else if (Camera_Drv->SetSaturation(Camera_CompObj, Saturation)  < 0)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  else
  {
    Camera_Ctx[Instance].Saturation = Saturation;
    ret = BSP_ERROR_NONE;
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Get the camera Saturation Level.
  * @param  Instance    Camera instance.
  * @param  Saturation  Saturation Level
  * @retval BSP status
  */
int32_t BSP_CAMERA_GetSaturation(uint32_t Instance, int32_t *Saturation)
{
  int32_t ret;

  if (Instance >= CAMERA_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if (Camera_Cap.Saturation == 0U)
  {
    ret = BSP_ERROR_FEATURE_NOT_SUPPORTED;
  }
  else
  {
    *Saturation = Camera_Ctx[Instance].Saturation;
    ret = BSP_ERROR_NONE;
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Set the camera Contrast Level.
  * @param  Instance Camera instance.
  * @param  Contrast Contrast Level
  * @retval BSP status
  */
int32_t BSP_CAMERA_SetContrast(uint32_t Instance, int32_t Contrast)
{
  int32_t ret;

  if ((Instance >= CAMERA_INSTANCES_NBR) || ((Contrast < CAMERA_CONTRAST_MIN) && (Contrast > CAMERA_CONTRAST_MAX)))
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if (Camera_Cap.Contrast == 0U)
  {
    ret = BSP_ERROR_FEATURE_NOT_SUPPORTED;
  }
  else if (Camera_Drv->SetContrast(Camera_CompObj, Contrast)  < 0)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  else
  {
    Camera_Ctx[Instance].Contrast = Contrast;
    ret = BSP_ERROR_NONE;
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Get the camera Contrast Level.
  * @param  Instance Camera instance.
  * @param  Contrast Contrast Level
  * @retval BSP status
  */
int32_t BSP_CAMERA_GetContrast(uint32_t Instance, int32_t *Contrast)
{
  int32_t ret;

  if (Instance >= CAMERA_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if (Camera_Cap.Contrast == 0U)
  {
    ret = BSP_ERROR_FEATURE_NOT_SUPPORTED;
  }
  else
  {
    *Contrast = Camera_Ctx[Instance].Contrast;
    ret = BSP_ERROR_NONE;
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Set the camera Hue Degree.
  * @param  Instance   Camera instance.
  * @param  HueDegree  Hue Degree
  * @retval BSP status
  */
int32_t BSP_CAMERA_SetHueDegree(uint32_t Instance, int32_t HueDegree)
{
  int32_t ret;

  if ((Instance >= CAMERA_INSTANCES_NBR) || ((HueDegree < CAMERA_HUEDEGREE_MIN) && (HueDegree > CAMERA_HUEDEGREE_MAX)))
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if (Camera_Cap.HueDegree == 0U)
  {
    ret = BSP_ERROR_FEATURE_NOT_SUPPORTED;
  }
  else if (Camera_Drv->SetHueDegree(Camera_CompObj, HueDegree) < 0)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  else
  {
    Camera_Ctx[Instance].HueDegree = HueDegree;
    ret = BSP_ERROR_NONE;
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Get the camera Hue Degree.
  * @param  Instance   Camera instance.
  * @param  HueDegree  Hue Degree
  * @retval BSP status
  */
int32_t BSP_CAMERA_GetHueDegree(uint32_t Instance, int32_t *HueDegree)
{
  int32_t ret;

  if (Instance >= CAMERA_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if (Camera_Cap.HueDegree == 0U)
  {
    ret = BSP_ERROR_FEATURE_NOT_SUPPORTED;
  }
  else
  {
    *HueDegree = Camera_Ctx[Instance].HueDegree;
    ret = BSP_ERROR_NONE;
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Set the camera Mirror/Flip.
  * @param  Instance  Camera instance.
  * @param  MirrorFlip CAMERA_MIRRORFLIP_NONE or any combination of
  *                    CAMERA_MIRRORFLIP_FLIP and CAMERA_MIRRORFLIP_MIRROR
  * @retval BSP status
  */
int32_t BSP_CAMERA_SetMirrorFlip(uint32_t Instance, uint32_t MirrorFlip)
{
  int32_t ret;

  if (Instance >= CAMERA_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if (Camera_Cap.MirrorFlip == 0U)
  {
    ret = BSP_ERROR_FEATURE_NOT_SUPPORTED;
  }
  else if (Camera_Drv->MirrorFlipConfig(Camera_CompObj, MirrorFlip)  < 0)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  else
  {
    Camera_Ctx[Instance].MirrorFlip = MirrorFlip;
    ret = BSP_ERROR_NONE;
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Get the camera Mirror/Flip.
  * @param  Instance   Camera instance.
  * @param  MirrorFlip Mirror/Flip config
  * @retval BSP status
  */
int32_t BSP_CAMERA_GetMirrorFlip(uint32_t Instance, uint32_t *MirrorFlip)
{
  int32_t ret;

  if (Instance >= CAMERA_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if (Camera_Cap.MirrorFlip == 0U)
  {
    ret = BSP_ERROR_FEATURE_NOT_SUPPORTED;
  }
  else
  {
    *MirrorFlip = Camera_Ctx[Instance].MirrorFlip;
    ret = BSP_ERROR_NONE;
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Set the camera zoom
  * @param  Instance Camera instance.
  * @param  Zoom     Zoom to be configured
  * @retval BSP status
  */
int32_t BSP_CAMERA_SetZoom(uint32_t Instance, uint32_t Zoom)
{
  int32_t ret;

  if (Instance >= CAMERA_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if (Camera_Cap.Zoom == 0U)
  {
    ret = BSP_ERROR_FEATURE_NOT_SUPPORTED;
  }
  else if (Camera_Drv->ZoomConfig(Camera_CompObj, Zoom) < 0)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  else
  {
    Camera_Ctx[Instance].Zoom = Zoom;
    ret = BSP_ERROR_NONE;
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Get the camera zoom
  * @param  Instance Camera instance.
  * @param  Zoom     Zoom to be returned
  * @retval BSP status
  */
int32_t BSP_CAMERA_GetZoom(uint32_t Instance, uint32_t *Zoom)
{
  int32_t ret;

  if (Instance >= CAMERA_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if (Camera_Cap.Zoom == 0U)
  {
    ret = BSP_ERROR_FEATURE_NOT_SUPPORTED;
  }
  else
  {
    *Zoom = Camera_Ctx[Instance].Zoom;
    ret = BSP_ERROR_NONE;
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Enable the camera night mode
  * @param  Instance Camera instance.
  * @retval BSP status
  */
int32_t BSP_CAMERA_EnableNightMode(uint32_t Instance)
{
  int32_t ret;

  if (Instance >= CAMERA_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if (Camera_Cap.NightMode == 0U)
  {
    ret = BSP_ERROR_FEATURE_NOT_SUPPORTED;
  }
  else if (Camera_Drv->NightModeConfig(Camera_CompObj, CAMERA_NIGHT_MODE_SET) < 0)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  else
  {
    ret = BSP_ERROR_NONE;
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  Disable the camera night mode
  * @param  Instance Camera instance.
  * @retval BSP status
  */
int32_t BSP_CAMERA_DisableNightMode(uint32_t Instance)
{
  int32_t ret;

  if (Instance >= CAMERA_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if (Camera_Cap.NightMode == 0U)
  {
    ret = BSP_ERROR_FEATURE_NOT_SUPPORTED;
  }
  else if (Camera_Drv->NightModeConfig(Camera_CompObj, CAMERA_NIGHT_MODE_RESET) < 0)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  else
  {
    ret = BSP_ERROR_NONE;
  }

  /* Return BSP status */
  return ret;
}

/**
  * @brief  CAMERA hardware reset
  * @param  Instance Camera instance.
  * @retval BSP status
  */
int32_t BSP_CAMERA_HwReset(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;
  GPIO_InitTypeDef gpio_init_structure = {0};

  /* Enable GPIO clocks */
  __HAL_RCC_GPIOO_CLK_ENABLE(); // EN Cam
  __HAL_RCC_GPIOD_CLK_ENABLE(); // NRST Cam

  if (Instance >= CAMERA_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    gpio_init_structure.Pin       = EN_CAM_PIN;
    gpio_init_structure.Pull      = GPIO_NOPULL;
    gpio_init_structure.Mode      = GPIO_MODE_OUTPUT_PP;
    gpio_init_structure.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(EN_CAM_PORT, &gpio_init_structure);

    gpio_init_structure.Pin       = NRST_CAM_PIN;
    gpio_init_structure.Pull      = GPIO_NOPULL;
    gpio_init_structure.Mode      = GPIO_MODE_OUTPUT_PP;
    gpio_init_structure.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(NRST_CAM_PORT, &gpio_init_structure);

    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET); // Disable MB1723 2V8 signal
    HAL_Delay(100);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_RESET); /* RESET low (reset active low) */
    HAL_Delay(100);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_SET); // ENABLE MB1723 2V8 signal
    HAL_Delay(100);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_SET); /* RESET high (release reset) */
    HAL_Delay(100);
  }
  return ret;
}

/**
  * @brief  CAMERA power down
  * @param  Instance Camera instance.
  * @retval BSP status
  */
int32_t BSP_CAMERA_PwrDown(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;
  GPIO_InitTypeDef gpio_init_structure = {0};

  if (Instance >= CAMERA_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    gpio_init_structure.Pin       = EN_CAM_PIN;
    gpio_init_structure.Pull      = GPIO_NOPULL;
    gpio_init_structure.Mode      = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(EN_CAM_PORT, &gpio_init_structure);

    gpio_init_structure.Pin       = NRST_CAM_PIN;
    gpio_init_structure.Pull      = GPIO_NOPULL;
    gpio_init_structure.Mode      = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(NRST_CAM_PORT, &gpio_init_structure);

    /* Camera power down sequence */
    /* Assert the camera Enable pin (active high) */
    HAL_GPIO_WritePin(EN_CAM_PORT, EN_CAM_PIN, GPIO_PIN_RESET);

    /* De-assert the camera NRST pin (active low) */
    HAL_GPIO_WritePin(NRST_CAM_PORT, NRST_CAM_PIN, GPIO_PIN_RESET);

  }

  return ret;
}

/**
  * @brief  This function handles the Background process required for the ISP Middleware.
  * @retval BSP status
  */
int32_t BSP_CAMERA_BackgroundProcess(void)
{
  if (ISP_BackgroundProcess(&hcamera_isp) != ISP_OK)
  {
    return BSP_ERROR_PERIPH_FAILURE;
  }
  return BSP_ERROR_NONE;
}

/**
  * @brief  This function handles DCMIPP interrupt request.
  * @param  Instance Camera instance
  * @retval None
  */
void BSP_CAMERA_IRQHandler(uint32_t Instance)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(Instance);

  HAL_DCMIPP_IRQHandler(&hcamera_dcmipp);
}

/**
  * @brief  Line Event callback.
  * @param  Instance Camera instance.
  * @retval None
  */
__weak void BSP_CAMERA_LineEventCallback(uint32_t Instance)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(Instance);

  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_DCMIPP_PIPE_LineEventCallback could be implemented in the user file
   */
}

/**
  * @brief  Frame Event callback.
  * @param  Instance Camera instance.
  * @retval None
  */
__weak void BSP_CAMERA_FrameEventCallback(uint32_t Instance)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(Instance);

  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_DCMIPP_PIPE_FrameEventCallback could be implemented in the user file
   */
}

/**
  * @brief  Vsync Event callback.
  * @param  Instance Camera instance.
  * @retval None
  */
__weak void BSP_CAMERA_VsyncEventCallback(uint32_t Instance)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(Instance);

  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_DCMIPP__PIPE_VsyncEventCallback could be implemented in the user file
   */
}

/**
  * @brief  Pipe Error callback.
  * @param  Instance Camera instance.
  * @retval None
  */
__weak void BSP_CAMERA_PipeErrorCallback(uint32_t Instance)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(Instance);

  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_DCMIPP_PIPE_ErrorCallback could be implemented in the user file
   */
}

/**
  * @brief  Error callback.
  * @param  Instance Camera instance.
  * @retval None
  */
__weak void BSP_CAMERA_ErrorCallback(uint32_t Instance)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(Instance);

  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_DCMIPP_ErrorCallback could be implemented in the user file
   */
}

#if (USE_HAL_DCMIPP_REGISTER_CALLBACKS == 0) || !defined(USE_HAL_DCMIPP_REGISTER_CALLBACKS)
/**
  * @brief  Line event callback
  * @param  hdcmipp  pointer to the DCMIPP handle
  * @param  Pipe  pipe value
  * @retval None
  */
void HAL_DCMIPP_PIPE_LineEventCallback(DCMIPP_HandleTypeDef *hdcmipp, uint32_t Pipe)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hdcmipp);
  UNUSED(Pipe);

  BSP_CAMERA_LineEventCallback(0);
}

/**
  * @brief  Frame event callback
  * @param  hdcmipp pointer to the DCMIPP handle
  * @param  Pipe  pipe value
  * @retval None
  */
void HAL_DCMIPP_PIPE_FrameEventCallback(DCMIPP_HandleTypeDef *hdcmipp, uint32_t Pipe)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hdcmipp);
  UNUSED(Pipe);

  BSP_CAMERA_FrameEventCallback(0);
}

/**
  * @brief  Vsync event callback
  * @param  hdcmipp pointer to the DCMIPP handle
  * @param  Pipe  pipe value
  * @retval None
  */
void HAL_DCMIPP_PIPE_VsyncEventCallback(DCMIPP_HandleTypeDef *hdcmipp, uint32_t Pipe)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hdcmipp);
  UNUSED(Pipe);

  /* Update the frame counter and call the ISP statistics handler */
  ISP_IncMainFrameId(&hcamera_isp);
  ISP_GatherStatistics(&hcamera_isp);

  BSP_CAMERA_VsyncEventCallback(0);
}

/**
  * @brief  Pipe Error callback
  * @param  hdcmipp pointer to the DCMIPP handle
  * @param  Pipe  pipe value
  * @retval None
  */
void HAL_DCMIPP_PIPE_ErrorCallback(DCMIPP_HandleTypeDef *hdcmipp, uint32_t Pipe)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hdcmipp);
  UNUSED(Pipe);

  BSP_CAMERA_PipeErrorCallback(0);
}

/**
  * @brief  Error callback
  * @param  hdcmipp pointer to the DCMIPP handle
  * @retval None
  */
void HAL_DCMIPP_ErrorCallback(DCMIPP_HandleTypeDef *hdcmipp)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hdcmipp);

  BSP_CAMERA_ErrorCallback(0);
}
#endif /*(USE_HAL_DCMIPP_REGISTER_CALLBACKS == 0) || !defined(USE_HAL_DCMIPP_REGISTER_CALLBACKS) */

/**
  * @}
  */

/** @defgroup STM32N6570-DK_CAMERA_Private_Functions CAMERA Private Functions
  * @{
  */

/**
  * @brief  Initializes the DCMIPP MSP.
  * @param  hdcmipp  DCMIPP handle
  * @retval None
  */
static void DCMIPP_MspInit(const DCMIPP_HandleTypeDef *hdcmipp)
{
  UNUSED(hdcmipp);

  /*** Enable peripheral clock ***/
  /* Enable DCMIPP clock */
  __HAL_RCC_DCMIPP_CLK_ENABLE();

  __HAL_RCC_DCMIPP_FORCE_RESET();
  __HAL_RCC_DCMIPP_RELEASE_RESET();

  /*** Configure the NVIC for DCMIPP ***/
  /* NVIC configuration for DCMIPP transfer complete interrupt */
  HAL_NVIC_SetPriority(DCMIPP_IRQn, 0x07, 0);
  HAL_NVIC_EnableIRQ(DCMIPP_IRQn);

  /*** Enable peripheral clock ***/
  /* Enable CSI clock */
  __HAL_RCC_CSI_CLK_ENABLE();
  __HAL_RCC_CSI_CLK_SLEEP_DISABLE();
  __HAL_RCC_CSI_FORCE_RESET();
  __HAL_RCC_CSI_RELEASE_RESET();

  /*** Configure the NVIC for CSI ***/
  /* NVIC configuration for CSI transfer complete interrupt */
  HAL_NVIC_SetPriority(CSI_IRQn, 0x07, 0);
  HAL_NVIC_EnableIRQ(CSI_IRQn);
}

/**
  * @brief  DeInitializes the DCMIPP MSP.
  * @param  hdcmipp  DCMIPP handle
  * @retval None
  */
static void DCMIPP_MspDeInit(const DCMIPP_HandleTypeDef *hdcmipp)
{
  UNUSED(hdcmipp);

  __HAL_RCC_DCMIPP_FORCE_RESET();
  __HAL_RCC_DCMIPP_RELEASE_RESET();

  /* Disable NVIC  for DCMIPP transfer complete interrupt */
  HAL_NVIC_DisableIRQ(DCMIPP_IRQn);

  /* Disable DCMIPP clock */
  __HAL_RCC_DCMIPP_CLK_DISABLE();

  __HAL_RCC_CSI_FORCE_RESET();
  __HAL_RCC_CSI_RELEASE_RESET();

  /* Disable NVIC  for DCMIPP transfer complete interrupt */
  HAL_NVIC_DisableIRQ(CSI_IRQn);

  /* Disable DCMIPP clock */
  __HAL_RCC_CSI_CLK_DISABLE();
}

#if (USE_HAL_DCMIPP_REGISTER_CALLBACKS > 0)
/**
  * @brief  Line event callback
  * @param  hdcmipp  pointer to the DCMIPP handle
  * @retval None
  */
static void DCMIPP_PIPE_LineEventCallback(DCMIPP_HandleTypeDef *hdcmipp, uint32_t Pipe)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hdcmipp);

  BSP_CAMERA_LineEventCallback(0);
}

/**
  * @brief  Frame event callback
  * @param  hdcmipp pointer to the DCMIPP handle
  * @retval None
  */
static void DCMIPP_PIPE_FrameEventCallback(DCMIPP_HandleTypeDef *hdcmipp, uint32_t Pipe)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hdcmipp);

  BSP_CAMERA_FrameEventCallback(0);
}

/**
  * @brief  Vsync event callback
  * @param  hdcmipp  pointer to the DCMIPP handle
  * @retval None
  */
static void DCMIPP_PIPE_VsyncEventCallback(DCMIPP_HandleTypeDef *hdcmipp, uint32_t Pipe)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hdcmipp);

  BSP_CAMERA_VsyncEventCallback(0);
}
/**
  * @brief  Pipe Error callback
  * @param  hdcmipp pointer to the DCMIPP handle
  * @retval None
  */
static void DCMIPP_PIPE_ErrorCallback(DCMIPP_HandleTypeDef *hdcmipp, uint32_t Pipe)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hdcmipp);

  BSP_CAMERA_ErrorCallback(0);
}

/**
  * @brief  Error callback
  * @param  hdcmipp pointer to the DCMIPP handle
  * @retval None
  */
static void DCMIPP_ErrorCallback(DCMIPP_HandleTypeDef *hdcmipp)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hdcmipp);

  BSP_CAMERA_ErrorCallback(0);
}
#endif /* (USE_HAL_DCMIPP_REGISTER_CALLBACKS > 0) */

/**
  * @brief  ISP Middleware helper. Camera sensor info getter
  * @retval ISP Status
  */
static ISP_StatusTypeDef BSP_GetSensorInfoHelper(uint32_t Instance, ISP_SensorInfoTypeDef *SensorInfo)
{
  UNUSED(Instance);
  return (ISP_StatusTypeDef) IMX335_GetSensorInfo(Camera_CompObj, (IMX335_SensorInfo_t *) SensorInfo);
}

/**
  * @brief  ISP Middleware helper. Camera gain setter
  * @retval ISP Status
  */
static ISP_StatusTypeDef BSP_SetSensorGainHelper(uint32_t Instance, int32_t Gain)
{
  UNUSED(Instance);
  isp_gain = Gain;
  return (ISP_StatusTypeDef) IMX335_SetGain(Camera_CompObj, Gain);
}

/**
  * @brief  ISP Middleware helper. Camera gain getter
  * @retval ISP Status
  */
static ISP_StatusTypeDef BSP_GetSensorGainHelper(uint32_t Instance, int32_t *Gain)
{
  UNUSED(Instance);
  *Gain = isp_gain;
  return ISP_OK;
}

/**
  * @brief  ISP Middleware helper. Camera exposure setter
  * @retval ISP Status
  */
static ISP_StatusTypeDef BSP_SetSensorExposureHelper(uint32_t Instance, int32_t Exposure)
{
  UNUSED(Instance);
  isp_exposure = Exposure;
  return (ISP_StatusTypeDef) IMX335_SetExposure(Camera_CompObj, Exposure);
}

/**
  * @brief  ISP Middleware helper. Camera exposure getter
  * @retval ISP Status
  */
static ISP_StatusTypeDef BSP_GetSensorExposureHelper(uint32_t Instance, int32_t *Exposure)
{
  UNUSED(Instance);
  *Exposure = isp_exposure;
  return ISP_OK;
}


/**
  * @brief  Register Bus IOs if component ID is OK
  * @retval error status
  */
static int32_t IMX335_Probe(uint32_t Resolution, uint32_t PixelFormat)
{
  int32_t ret;
  IMX335_IO_t              IOCtx;
  uint32_t                 id;
  static IMX335_Object_t   IMX335Obj;

  /* Configure the camera driver */
  IOCtx.Address     = CAMERA_IMX335_ADDRESS;
  IOCtx.Init        = BSP_I2C1_Init;
  IOCtx.DeInit      = BSP_I2C1_DeInit;
  IOCtx.ReadReg     = BSP_I2C1_ReadReg16;
  IOCtx.WriteReg    = BSP_I2C1_WriteReg16;
  IOCtx.GetTick     = BSP_GetTick;

  if (IMX335_RegisterBusIO(&IMX335Obj, &IOCtx) != IMX335_OK)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  else if (IMX335_ReadID(&IMX335Obj, &id) != IMX335_OK)
  {
    ret = BSP_ERROR_COMPONENT_FAILURE;
  }
  else
  {
    if (id != (uint32_t) IMX335_CHIP_ID)
    {
      ret = BSP_ERROR_UNKNOWN_COMPONENT;
    }
    else
    {
      Camera_Drv = (CAMERA_Drv_t *) &IMX335_CAMERA_Driver;
      Camera_CompObj = &IMX335Obj;
      if (Camera_Drv->Init(Camera_CompObj, Resolution, PixelFormat) != IMX335_OK)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
      else if(Camera_Drv->SetFrequency(Camera_CompObj, IMX335_INCK_24MHZ)!= IMX335_OK)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
      else if (Camera_Drv->GetCapabilities(Camera_CompObj, &Camera_Cap) != IMX335_OK)
      {
        ret = BSP_ERROR_COMPONENT_FAILURE;
      }
      else
      {
        ret = BSP_ERROR_NONE;
      }
    }
  }

  return ret;
}



/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */
