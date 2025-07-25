 /**
 ******************************************************************************
 * @file    cmw_camera.c
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


/* Includes ------------------------------------------------------------------*/
#include "cmw_camera.h"

#include "isp_api.h"
#include "stm32n6xx_hal_dcmipp.h"
#include "cmw_utils.h"
#include "cmw_io.h"
#if defined(USE_VD55G1_SENSOR)
#include "cmw_vd55g1.h"
#endif
#if defined(USE_IMX335_SENSOR)
#include "cmw_imx335.h"
#endif
#if defined(USE_VD66GY_SENSOR)
#include "cmw_vd66gy.h"
#endif

typedef struct
{
  uint32_t Resolution;
  uint32_t pixel_format;
  uint32_t LightMode;
  uint32_t ColorEffect;
  int32_t  Brightness;
  int32_t  Saturation;
  int32_t  Contrast;
  int32_t  HueDegree;
  int32_t  Gain;
  int32_t  Exposure;
  int32_t  ExposureMode;
  uint32_t MirrorFlip;
  uint32_t Zoom;
  uint32_t NightMode;
  uint32_t IsMspCallbacksValid;
  uint32_t TestPattern;
} CAMERA_Ctx_t;

CMW_CameraInit_t  camera_conf;
CMW_Sensor_Name_t detected_sensor;
CAMERA_Ctx_t  Camera_Ctx;

DCMIPP_HandleTypeDef hcamera_dcmipp;
static CMW_Sensor_if_t Camera_Drv;

static union
{
#if defined(USE_IMX335_SENSOR)
  CMW_IMX335_t imx335_bsp;
#endif
#if defined(USE_VD55G1_SENSOR)
  CMW_VD55G1_t vd55g1_bsp;
#endif
#if defined(USE_VD66GY_SENSOR)
  CMW_VD66GY_t vd66gy_bsp;
#endif

} camera_bsp;

int is_camera_init = 0;
int is_camera_started = 0;
int is_pipe1_2_shared = 0;

#if defined(USE_IMX335_SENSOR)
static int32_t CMW_CAMERA_IMX335_Init( CMW_Sensor_Init_t *initSensors_params);
#endif
#if defined(USE_VD55G1_SENSOR)
static int32_t CMW_CAMERA_VD55G1_Init( CMW_Sensor_Init_t *initSensors_params);
#endif
#if defined(USE_VD66GY_SENSOR)
static int32_t CMW_CAMERA_VD66GY_Init(CMW_Sensor_Init_t *initValues);
#endif
static void CMW_CAMERA_EnableGPIOs(void);
static void CMW_CAMERA_PwrDown(void);
static int32_t CMW_CAMERA_SetPipe(DCMIPP_HandleTypeDef *hdcmipp, uint32_t pipe, CMW_DCMIPP_Conf_t *p_conf, uint32_t *pitch);
static int CMW_CAMERA_Probe_Sensor(CMW_Sensor_Init_t *initValues, CMW_Sensor_Name_t *sensorName);

DCMIPP_HandleTypeDef* CMW_CAMERA_GetDCMIPPHandle(void)
{
    return &hcamera_dcmipp;
}

int32_t CMW_CAMERA_SetPipeConfig(uint32_t pipe, CMW_DCMIPP_Conf_t *p_conf, uint32_t *pitch)
{
  return CMW_CAMERA_SetPipe(&hcamera_dcmipp, pipe, p_conf, pitch);
}

/**
  * @brief  Get Sensor name.
  * @param  sensorName  Camera sensor name
  * @retval CMW status
  */
int32_t CMW_CAMERA_GetSensorName(CMW_Sensor_Name_t *sensorName)
{
  int32_t ret = CMW_ERROR_NONE;
  CMW_Sensor_Init_t initValues;

  if (is_camera_init != 0)
  {
    *sensorName = detected_sensor;
    return CMW_ERROR_NONE;
  }

  initValues.width = 0;
  initValues.height = 0;
  initValues.fps = 30;
  initValues.pixel_format = DCMIPP_PIXEL_PACKER_FORMAT_RGB565_1;
  initValues.mirrorFlip = CMW_MIRRORFLIP_NONE;

  /* Set DCMIPP instance */
  hcamera_dcmipp.Instance = DCMIPP;

  /* Configure DCMIPP clock */
  ret = MX_DCMIPP_ClockConfig(&hcamera_dcmipp);
  if (ret != HAL_OK)
  {
    return CMW_ERROR_PERIPH_FAILURE;
  }
  /* Enable DCMIPP clock */
  ret = HAL_DCMIPP_Init(&hcamera_dcmipp);
  if (ret != HAL_OK)
  {
    return CMW_ERROR_PERIPH_FAILURE;
  }

  CMW_CAMERA_EnableGPIOs();

  ret = CMW_CAMERA_Probe_Sensor(&initValues, &detected_sensor);
  if (ret != CMW_ERROR_NONE)
  {
    return CMW_ERROR_UNKNOWN_COMPONENT;
  }
  *sensorName = detected_sensor;
  return CMW_ERROR_NONE;
}

static int CMW_CAMERA_Probe_Sensor(CMW_Sensor_Init_t *initValues, CMW_Sensor_Name_t *sensorName)
{
  int ret;
#if defined(USE_VD55G1_SENSOR)
  ret = CMW_CAMERA_VD55G1_Init(initValues);
  if (ret == CMW_ERROR_NONE)
  {
    *sensorName = CMW_VD55G1_Sensor;
    return ret;
  }
#endif

#if defined(USE_VD66GY_SENSOR)
  ret = CMW_CAMERA_VD66GY_Init(initValues);
  if (ret == CMW_ERROR_NONE)
  {
    *sensorName = CMW_VD66GY_Sensor;
    return ret;
  }
#endif
#if defined(USE_IMX335_SENSOR)
  ret = CMW_CAMERA_IMX335_Init(initValues);
  if (ret == CMW_ERROR_NONE)
  {
    *sensorName = CMW_IMX335_Sensor;
    return ret;
  }
#endif
  else
  {
    return CMW_ERROR_UNKNOWN_COMPONENT;
  }
}



/**
  * @brief  Initializes the camera.
  * @param  initConf  Camera sensor requested config
  * @retval CMW status
  */
int32_t CMW_CAMERA_Init(CMW_CameraInit_t *initConf)
{
  int32_t ret = CMW_ERROR_NONE;
  CMW_Sensor_Init_t initValues;
  ISP_SensorInfoTypeDef info = {0};

  initValues.width = initConf->width;
  initValues.height = initConf->height;
  initValues.fps = initConf->fps;
  initValues.pixel_format = initConf->pixel_format;
  initValues.mirrorFlip = initConf->mirror_flip;

  if (is_camera_init != 0)
  {
    return CMW_ERROR_NONE;
  }

  /* Set DCMIPP instance */
  hcamera_dcmipp.Instance = DCMIPP;

  /* Configure DCMIPP clock */
  ret = MX_DCMIPP_ClockConfig(&hcamera_dcmipp);
  if (ret != HAL_OK)
  {
    return CMW_ERROR_PERIPH_FAILURE;
  }
  /* Enable DCMIPP clock */
  ret = HAL_DCMIPP_Init(&hcamera_dcmipp);
  if (ret != HAL_OK)
  {
    return CMW_ERROR_PERIPH_FAILURE;
  }

  CMW_CAMERA_EnableGPIOs();

  ret = CMW_CAMERA_Probe_Sensor(&initValues, &detected_sensor);
  if (ret != CMW_ERROR_NONE)
  {
    return CMW_ERROR_UNKNOWN_COMPONENT;
  }

  /* Configure exposure and gain for a more suitable quality */
  ret = CMW_CAMERA_GetSensorInfo(&info);
  if (ret == CMW_ERROR_COMPONENT_FAILURE)
  {
    return CMW_ERROR_UNKNOWN_COMPONENT;
  }
  ret = CMW_CAMERA_SetExposure(info.exposure_min);
  if (ret == CMW_ERROR_COMPONENT_FAILURE)
  {
    return CMW_ERROR_UNKNOWN_COMPONENT;
  }
  ret = CMW_CAMERA_SetGain(info.gain_min);
  if (ret == CMW_ERROR_COMPONENT_FAILURE)
  {
    return CMW_ERROR_UNKNOWN_COMPONENT;
  }

  /* Write back the initValue width and height that might be changed */
  initConf->width = initValues.width;
  initConf->height = initValues.height ;
  camera_conf = *initConf;

  is_camera_init++;
  /* CMW status */
  ret = CMW_ERROR_NONE;
  return ret;
}

/**
  * @brief  Set the camera Mirror/Flip.
  * @param  MirrorFlip CMW_MIRRORFLIP_NONE CMW_MIRRORFLIP_FLIP CMW_MIRRORFLIP_MIRROR CMW_MIRRORFLIP_FLIP_MIRROR
  * @retval CMW status
*/
int32_t CMW_CAMERA_SetMirrorFlip(int32_t MirrorFlip)
{
  int ret;

  if (Camera_Drv.SetMirrorFlip == NULL)
  {
    return CMW_ERROR_FEATURE_NOT_SUPPORTED;
  }

  ret = Camera_Drv.SetMirrorFlip(&camera_bsp, MirrorFlip);
  if (ret != CMW_ERROR_NONE)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  camera_conf.mirror_flip = MirrorFlip;
  ret = CMW_ERROR_NONE;
  /* Return CMW status */
  return ret;
}

/**
  * @brief  Get the camera Mirror/Flip.
  * @param  MirrorFlip CMW_MIRRORFLIP_NONE CMW_MIRRORFLIP_FLIP CMW_MIRRORFLIP_MIRROR CMW_MIRRORFLIP_FLIP_MIRROR
  * @retval CMW status
*/
int32_t CMW_CAMERA_GetMirrorFlip(int32_t *MirrorFlip)
{
  *MirrorFlip = camera_conf.mirror_flip;
  return CMW_ERROR_NONE;
}

/**
  * @brief  Starts the camera capture in the selected mode.
  * @param  pipe  DCMIPP Pipe
  * @param  pbuff pointer to the camera output buffer
  * @param  mode  CMW_MODE_CONTINUOUS or CMW_MODE_SNAPSHOT
  * @retval CMW status
  */
int32_t CMW_CAMERA_Start(uint32_t pipe, uint8_t *pbuff, uint32_t mode)
{
  int32_t ret = CMW_ERROR_NONE;

  if (pipe >= DCMIPP_NUM_OF_PIPES)
  {
    return CMW_ERROR_WRONG_PARAM;
  }

  ret = HAL_DCMIPP_CSI_PIPE_Start(&hcamera_dcmipp, pipe, DCMIPP_VIRTUAL_CHANNEL0, (uint32_t)pbuff, mode);
  if (ret != HAL_OK)
  {
    return CMW_ERROR_PERIPH_FAILURE;
  }

  if (!is_camera_started)
  {
    ret = Camera_Drv.Start(&camera_bsp);
    if (ret != CMW_ERROR_NONE)
    {
      return CMW_ERROR_COMPONENT_FAILURE;
    }
    is_camera_started++;
  }

  /* Return CMW status */
  return ret;
}

#if defined (STM32N657xx)
/**
  * @brief  Starts the camera capture in the selected mode.
  * @param  pipe  DCMIPP Pipe
  * @param  pbuff1 pointer to the first camera output buffer
  * @param  pbuff2 pointer to the second camera output buffer
  * @param  mode  CMW_MODE_CONTINUOUS or CMW_MODE_SNAPSHOT
  * @retval CMW status
  */
int32_t CMW_CAMERA_DoubleBufferStart(uint32_t pipe, uint8_t *pbuff1, uint8_t *pbuff2, uint32_t Mode)
{
  int32_t ret = CMW_ERROR_NONE;

  if (pipe >= DCMIPP_NUM_OF_PIPES)
  {
    return CMW_ERROR_WRONG_PARAM;
  }

  if (HAL_DCMIPP_CSI_PIPE_DoubleBufferStart(&hcamera_dcmipp, pipe, DCMIPP_VIRTUAL_CHANNEL0, (uint32_t)pbuff1,
                                            (uint32_t)pbuff2, Mode) != HAL_OK)
  {
    return CMW_ERROR_PERIPH_FAILURE;
  }

  if (!is_camera_started)
  {
    ret = Camera_Drv.Start(&camera_bsp);
    if (ret != CMW_ERROR_NONE)
    {
      return CMW_ERROR_COMPONENT_FAILURE;
    }
    is_camera_started++;
  }

  /* Return CMW status */
  return ret;
}
#endif




/**
  * @brief  DCMIPP Clock Config for DCMIPP.
  * @param  hdcmipp  DCMIPP Handle
  *         Being __weak it can be overwritten by the application
  * @retval HAL_status
  */
__weak HAL_StatusTypeDef MX_DCMIPP_ClockConfig(DCMIPP_HandleTypeDef *hdcmipp)
{
  UNUSED(hdcmipp);

  return HAL_OK;
}

/**
  * @brief  DeInitializes the camera.
  * @retval CMW status
  */
int32_t CMW_CAMERA_DeInit(void)
{
  int32_t ret = CMW_ERROR_NONE;


  ret = HAL_DCMIPP_CSI_PIPE_Stop(&hcamera_dcmipp, DCMIPP_PIPE1, DCMIPP_VIRTUAL_CHANNEL0);
  if (ret != HAL_OK)
  {
    return CMW_ERROR_PERIPH_FAILURE;
  }

  ret = HAL_DCMIPP_CSI_PIPE_Stop(&hcamera_dcmipp, DCMIPP_PIPE2, DCMIPP_VIRTUAL_CHANNEL0);
  if (ret != HAL_OK)
  {
    return CMW_ERROR_PERIPH_FAILURE;
  }

  ret = HAL_DCMIPP_DeInit(&hcamera_dcmipp);
  if (ret != HAL_OK)
  {
    return CMW_ERROR_PERIPH_FAILURE;
  }

  if (is_camera_init <= 0)
  {
    return CMW_ERROR_NONE;
  }

  /* De-initialize the camera module */
  ret = Camera_Drv.DeInit(&camera_bsp);
  if (ret != CMW_ERROR_NONE)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }
  /* Set Camera in Power Down */
  CMW_CAMERA_PwrDown();

  /* Update DCMIPPInit counter */
  is_camera_init--;
  is_camera_started--;
  is_pipe1_2_shared--;

  /* Return CMW status */
  ret = CMW_ERROR_NONE;
  return ret;
}

/**
  * @brief  Suspend the CAMERA capture on selected pipe
  * @param  pipe Dcmipp pipe.
  * @retval CMW status
  */
int32_t CMW_CAMERA_Suspend(uint32_t pipe)
{
  if (hcamera_dcmipp.PipeState[pipe] > HAL_DCMIPP_PIPE_STATE_READY)
  {
    if (HAL_DCMIPP_PIPE_Suspend(&hcamera_dcmipp, pipe) != HAL_OK)
    {
      return CMW_ERROR_PERIPH_FAILURE;
    }
  }

  /* Return CMW status */
  return CMW_ERROR_NONE;
}

/**
  * @brief  Resume the CAMERA capture on selected pipe
  * @param  pipe Dcmipp pipe.
  * @retval CMW status
  */
int32_t CMW_CAMERA_Resume(uint32_t pipe)
{
  if (hcamera_dcmipp.PipeState[pipe] > HAL_DCMIPP_PIPE_STATE_BUSY)
  {
    if (HAL_DCMIPP_PIPE_Resume(&hcamera_dcmipp, pipe) != HAL_OK)
    {
      return CMW_ERROR_PERIPH_FAILURE;
    }
  }

  /* Return CMW status */
  return CMW_ERROR_NONE;
}

/**
  * @brief  Set the camera gain.
  * @param  Gain     Gain in mdB
  * @retval CMW status
  */
int CMW_CAMERA_SetGain(int32_t Gain)
{
  int ret;
  if(Camera_Drv.SetGain == NULL)
  {
    return CMW_ERROR_FEATURE_NOT_SUPPORTED;
  }

  ret = Camera_Drv.SetGain(&camera_bsp, Gain);
  if (ret != CMW_ERROR_NONE)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  Camera_Ctx.Gain = Gain;
  return CMW_ERROR_NONE;
}

/**
  * @brief  Get the camera gain.
  * @param  Gain     Gain in mdB
  * @retval CMW status
  */
int CMW_CAMERA_GetGain(int32_t *Gain)
{
  *Gain = Camera_Ctx.Gain;
  return CMW_ERROR_NONE;
}

/**
  * @brief  Set the camera exposure.
  * @param  exposure exposure in microseconds
  * @retval CMW status
  */
int CMW_CAMERA_SetExposure(int32_t exposure)
{
  int ret;

  if(Camera_Drv.SetExposure == NULL)
  {
    return CMW_ERROR_FEATURE_NOT_SUPPORTED;
  }

  ret = Camera_Drv.SetExposure(&camera_bsp, exposure);
  if (ret != CMW_ERROR_NONE)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  Camera_Ctx.Exposure = exposure;
  return CMW_ERROR_NONE;
}

/**
  * @brief  Get the camera exposure.
  * @param  exposure exposure in microseconds
  * @retval CMW status
  */
int CMW_CAMERA_GetExposure(int32_t *exposure)
{
  *exposure = Camera_Ctx.Exposure;
  return CMW_ERROR_NONE;
}

/**
  * @brief  Set the camera exposure mode.
  * @param  exposureMode Exposure mode CMW_EXPOSUREMODE_AUTO, CMW_EXPOSUREMODE_AUTOFREEZE, CMW_EXPOSUREMODE_MANUAL
  * @retval CMW status
  */
int32_t CMW_CAMERA_SetExposureMode(int32_t exposureMode)
{
  int ret;

  if(Camera_Drv.SetExposureMode == NULL)
  {
    return CMW_ERROR_FEATURE_NOT_SUPPORTED;
  }

  ret = Camera_Drv.SetExposureMode(&camera_bsp, exposureMode);
  if (ret != CMW_ERROR_NONE)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  Camera_Ctx.ExposureMode = exposureMode;
  return CMW_ERROR_NONE;
}

/**
  * @brief  Get the camera exposure mode.
  * @param  exposureMode Exposure mode CAMERA_EXPOSURE_AUTO, CAMERA_EXPOSURE_AUTOFREEZE, CAMERA_EXPOSURE_MANUAL
  * @retval CMW status
  */
int32_t CMW_CAMERA_GetExposureMode(int32_t *exposureMode)
{
  *exposureMode = Camera_Ctx.ExposureMode;
  return CMW_ERROR_NONE;
}

/**
  * @brief  Set (Enable/Disable and Configure) the camera test pattern
  * @param  mode Pattern mode (sensor specific value) to be configured. '-1' means disable.
  * @retval CMW status
  */
int32_t CMW_CAMERA_SetTestPattern(int32_t mode)
{
  int32_t ret;

  if(Camera_Drv.SetTestPattern == NULL)
  {
    return CMW_ERROR_FEATURE_NOT_SUPPORTED;
  }

  ret = Camera_Drv.SetTestPattern(&camera_bsp, mode);
  if (ret != CMW_ERROR_NONE)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  Camera_Ctx.TestPattern = mode;
  return CMW_ERROR_NONE;
}

/**
  * @brief  Get the camera test pattern
  * @param  mode Pattern mode (sensor specific value) to be returned. '-1' means disable.
  * @retval CMW status
  */
int32_t CMW_CAMERA_GetTestPattern(int32_t *mode)
{
  *mode = Camera_Ctx.TestPattern;
  return CMW_ERROR_NONE;
}

/**
  * @brief  Get the Camera Sensor info.
  * @param  info  pointer to sensor info
  * @note   This function should be called after the init. This to get Capabilities
  *         from the camera sensor
  * @retval Component status
  */
int32_t CMW_CAMERA_GetSensorInfo(ISP_SensorInfoTypeDef *info)
{

  int32_t ret;

  if(Camera_Drv.GetSensorInfo == NULL)
  {
    return CMW_ERROR_FEATURE_NOT_SUPPORTED;
  }

  ret = Camera_Drv.GetSensorInfo(&camera_bsp, info);
  if (ret != CMW_ERROR_NONE)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  return CMW_ERROR_NONE;
}



int32_t CMW_CAMERA_Run()
{
  if(Camera_Drv.Run != NULL)
  {
      return Camera_Drv.Run(&camera_bsp);
  }
  return CMW_ERROR_NONE;
}

/**
 * @brief  Vsync Event callback on pipe
 * @param  Pipe  Pipe receiving the callback
 * @retval None
 */
__weak int CMW_CAMERA_PIPE_VsyncEventCallback(uint32_t pipe)
{
  UNUSED(pipe);

  return HAL_OK;
}

/**
 * @brief  Frame Event callback on pipe
 * @param  Pipe  Pipe receiving the callback
 * @retval None
 */
__weak int CMW_CAMERA_PIPE_FrameEventCallback(uint32_t pipe)
{
  UNUSED(pipe);

  return HAL_OK;
}

/**
 * @brief  Vsync Event callback on pipe
 * @param  hdcmipp DCMIPP device handle
 *         Pipe    Pipe receiving the callback
 * @retval None
 */
void HAL_DCMIPP_PIPE_VsyncEventCallback(DCMIPP_HandleTypeDef *hdcmipp, uint32_t Pipe)
{
  UNUSED(hdcmipp);
  if(Camera_Drv.VsyncEventCallback != NULL)
  {
      Camera_Drv.VsyncEventCallback(&camera_bsp, Pipe);
  }
  CMW_CAMERA_PIPE_VsyncEventCallback(Pipe);
}

/**
 * @brief  Frame Event callback on pipe
 * @param  hdcmipp DCMIPP device handle
 *         Pipe    Pipe receiving the callback
 * @retval None
 */
void HAL_DCMIPP_PIPE_FrameEventCallback(DCMIPP_HandleTypeDef *hdcmipp, uint32_t Pipe)
{
  UNUSED(hdcmipp);
  if(Camera_Drv.FrameEventCallback != NULL)
  {
      Camera_Drv.FrameEventCallback(&camera_bsp, Pipe);
  }
  CMW_CAMERA_PIPE_FrameEventCallback(Pipe);
}

/**
  * @brief  Initializes the DCMIPP MSP.
  * @param  hdcmipp  DCMIPP handle
  * @retval None
  */
void HAL_DCMIPP_MspInit(DCMIPP_HandleTypeDef *hdcmipp)
{
  UNUSED(hdcmipp);

  /*** Enable peripheral clock ***/
  /* Enable DCMIPP clock */
  __HAL_RCC_DCMIPP_CLK_ENABLE();
  __HAL_RCC_DCMIPP_CLK_SLEEP_ENABLE();
  __HAL_RCC_DCMIPP_FORCE_RESET();
  __HAL_RCC_DCMIPP_RELEASE_RESET();

  /*** Configure the NVIC for DCMIPP ***/
  /* NVIC configuration for DCMIPP transfer complete interrupt */
  HAL_NVIC_SetPriority(DCMIPP_IRQn, 0x07, 0);
  HAL_NVIC_EnableIRQ(DCMIPP_IRQn);

  /*** Enable peripheral clock ***/
  /* Enable CSI clock */
  __HAL_RCC_CSI_CLK_ENABLE();
  __HAL_RCC_CSI_CLK_SLEEP_ENABLE();
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
void HAL_DCMIPP_MspDeInit(DCMIPP_HandleTypeDef *hdcmipp)
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

/**
  * @brief  CAMERA hardware reset
  * @retval CMW status
  */
static void CMW_CAMERA_EnableGPIOs(void)
{
  GPIO_InitTypeDef gpio_init_structure = {0};

  /* Enable GPIO clocks */
  EN_CAM_GPIO_ENABLE_VDDIO();
  EN_CAM_GPIO_CLK_ENABLE();
  NRST_CAM_GPIO_ENABLE_VDDIO();
  NRST_CAM_GPIO_CLK_ENABLE();

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
}

/**
  * @brief  CAMERA power down
  * @retval CMW status
  */
static void CMW_CAMERA_PwrDown(void)
{
  GPIO_InitTypeDef gpio_init_structure = {0};

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

static void CMW_CAMERA_ShutdownPin(int value)
{
  HAL_GPIO_WritePin(NRST_CAM_PORT, NRST_CAM_PIN, value ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void CMW_CAMERA_EnablePin(int value)
{
  HAL_GPIO_WritePin(EN_CAM_PORT, EN_CAM_PIN, value ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

#if defined(USE_VD66GY_SENSOR) || defined(USE_IMX335_SENSOR)
static ISP_StatusTypeDef CB_ISP_SetSensorGain(uint32_t camera_instance, int32_t gain)
{
  if (CMW_CAMERA_SetGain(gain) != CMW_ERROR_NONE)
    return ISP_ERR_SENSORGAIN;

  return ISP_OK;
}

static ISP_StatusTypeDef CB_ISP_GetSensorGain(uint32_t camera_instance, int32_t *gain)
{
  if (CMW_CAMERA_GetGain(gain) != CMW_ERROR_NONE)
    return ISP_ERR_SENSORGAIN;

  return ISP_OK;
}

static ISP_StatusTypeDef CB_ISP_SetSensorExposure(uint32_t camera_instance, int32_t exposure)
{
  if (CMW_CAMERA_SetExposure(exposure) != CMW_ERROR_NONE)
    return ISP_ERR_SENSOREXPOSURE;

  return ISP_OK;
}

static ISP_StatusTypeDef CB_ISP_GetSensorExposure(uint32_t camera_instance, int32_t *exposure)
{
  if (CMW_CAMERA_GetExposure(exposure) != CMW_ERROR_NONE)
    return ISP_ERR_SENSOREXPOSURE;

  return ISP_OK;
}

static ISP_StatusTypeDef CB_ISP_GetSensorInfo(uint32_t camera_instance, ISP_SensorInfoTypeDef *Info)
{
  if(Camera_Drv.GetSensorInfo != NULL)
  {
    if (Camera_Drv.GetSensorInfo(&camera_bsp, Info) != CMW_ERROR_NONE)
      return ISP_ERR_SENSOREXPOSURE;
  }
  return ISP_OK;
}
#endif

#if defined(USE_VD55G1_SENSOR)
static int32_t CMW_CAMERA_VD55G1_Init( CMW_Sensor_Init_t *initSensors_params)
{
  int32_t ret = CMW_ERROR_NONE;
  DCMIPP_CSI_ConfTypeDef csi_conf = { 0 };
  DCMIPP_CSI_PIPE_ConfTypeDef csi_pipe_conf = { 0 };

  memset(&camera_bsp, 0, sizeof(camera_bsp));
  camera_bsp.vd55g1_bsp.Address     = CAMERA_VD55G1_ADDRESS;
  camera_bsp.vd55g1_bsp.ClockInHz   = CAMERA_VD55G1_FREQ_IN_HZ;
  camera_bsp.vd55g1_bsp.Init        = CMW_I2C_INIT;
  camera_bsp.vd55g1_bsp.DeInit      = CMW_I2C_DEINIT;
  camera_bsp.vd55g1_bsp.WriteReg    = CMW_I2C_WRITEREG16;
  camera_bsp.vd55g1_bsp.ReadReg     = CMW_I2C_READREG16;
  camera_bsp.vd55g1_bsp.Delay       = HAL_Delay;
  camera_bsp.vd55g1_bsp.ShutdownPin = CMW_CAMERA_ShutdownPin;
  camera_bsp.vd55g1_bsp.EnablePin   = CMW_CAMERA_EnablePin;

  ret = CMW_VD55G1_Probe(&camera_bsp.vd55g1_bsp, &Camera_Drv);
  if (ret != CMW_ERROR_NONE)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  /* Special case: when resolution is not specified take the full sensor resolution */
  if ((initSensors_params->width == 0) || (initSensors_params->height == 0))
  {
    ISP_SensorInfoTypeDef sensor_info;
    Camera_Drv.GetSensorInfo(&camera_bsp, &sensor_info);
    initSensors_params->width = sensor_info.width;
    initSensors_params->height = sensor_info.height;
  }

  ret = Camera_Drv.Init(&camera_bsp, initSensors_params);
  if (ret != CMW_ERROR_NONE)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  csi_conf.NumberOfLanes = DCMIPP_CSI_ONE_DATA_LANE;
  csi_conf.DataLaneMapping = DCMIPP_CSI_PHYSICAL_DATA_LANES;
  csi_conf.PHYBitrate = DCMIPP_CSI_PHY_BT_800;
  ret = HAL_DCMIPP_CSI_SetConfig(&hcamera_dcmipp, &csi_conf);
  if (ret != HAL_OK)
  {
    return CMW_ERROR_PERIPH_FAILURE;
  }

  ret = HAL_DCMIPP_CSI_SetVCConfig(&hcamera_dcmipp, DCMIPP_VIRTUAL_CHANNEL0, DCMIPP_CSI_DT_BPP8);
  if (ret != HAL_OK)
  {
    return CMW_ERROR_PERIPH_FAILURE;
  }

  csi_pipe_conf.DataTypeMode = DCMIPP_DTMODE_DTIDA;
  csi_pipe_conf.DataTypeIDA = DCMIPP_DT_RAW8;
  csi_pipe_conf.DataTypeIDB = 0;
  /* Pre-initialize CSI config for all the pipes */
  for (uint32_t i = DCMIPP_PIPE0; i <= DCMIPP_PIPE2; i++)
  {
    ret = HAL_DCMIPP_CSI_PIPE_SetConfig(&hcamera_dcmipp, i, &csi_pipe_conf);
    if (ret != HAL_OK)
    {
      return CMW_ERROR_PERIPH_FAILURE;
    }
  }

  return CMW_ERROR_NONE;
}
#endif

#if defined(USE_VD66GY_SENSOR)
static int32_t CMW_CAMERA_VD66GY_Init( CMW_Sensor_Init_t *initSensors_params)
{
  int32_t ret = CMW_ERROR_NONE;
  DCMIPP_CSI_ConfTypeDef csi_conf = { 0 };
  DCMIPP_CSI_PIPE_ConfTypeDef csi_pipe_conf = { 0 };

  memset(&camera_bsp, 0, sizeof(camera_bsp));
  camera_bsp.vd66gy_bsp.Address     = CAMERA_VD66GY_ADDRESS;
  camera_bsp.vd66gy_bsp.ClockInHz   = CAMERA_VD66GY_FREQ_IN_HZ;
  camera_bsp.vd66gy_bsp.Init        = CMW_I2C_INIT;
  camera_bsp.vd66gy_bsp.DeInit      = CMW_I2C_DEINIT;
  camera_bsp.vd66gy_bsp.ReadReg     = CMW_I2C_READREG16;
  camera_bsp.vd66gy_bsp.WriteReg    = CMW_I2C_WRITEREG16;
  camera_bsp.vd66gy_bsp.Delay       = HAL_Delay;
  camera_bsp.vd66gy_bsp.ShutdownPin = CMW_CAMERA_ShutdownPin;
  camera_bsp.vd66gy_bsp.EnablePin   = CMW_CAMERA_EnablePin;
  camera_bsp.vd66gy_bsp.hdcmipp     = &hcamera_dcmipp;
  camera_bsp.vd66gy_bsp.appliHelpers.SetSensorGain = CB_ISP_SetSensorGain;
  camera_bsp.vd66gy_bsp.appliHelpers.GetSensorGain = CB_ISP_GetSensorGain;
  camera_bsp.vd66gy_bsp.appliHelpers.SetSensorExposure = CB_ISP_SetSensorExposure;
  camera_bsp.vd66gy_bsp.appliHelpers.GetSensorExposure = CB_ISP_GetSensorExposure;
  camera_bsp.vd66gy_bsp.appliHelpers.GetSensorInfo = CB_ISP_GetSensorInfo;

  ret = CMW_VD66GY_Probe(&camera_bsp.vd66gy_bsp, &Camera_Drv);
  if (ret != CMW_ERROR_NONE)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  /* Special case: when resolution is not specified take the full sensor resolution */
  if ((initSensors_params->width == 0) || (initSensors_params->height == 0))
  {
    ISP_SensorInfoTypeDef sensor_info;
    Camera_Drv.GetSensorInfo(&camera_bsp, &sensor_info);
    initSensors_params->width = sensor_info.width;
    initSensors_params->height = sensor_info.height;
  }

  ret = Camera_Drv.Init(&camera_bsp, initSensors_params);
  if (ret != CMW_ERROR_NONE)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  csi_conf.NumberOfLanes = DCMIPP_CSI_TWO_DATA_LANES;
  csi_conf.DataLaneMapping = DCMIPP_CSI_PHYSICAL_DATA_LANES;
  csi_conf.PHYBitrate = DCMIPP_CSI_PHY_BT_800;
  ret = HAL_DCMIPP_CSI_SetConfig(&hcamera_dcmipp, &csi_conf);
  if (ret != HAL_OK)
  {
    return CMW_ERROR_PERIPH_FAILURE;
  }

  ret = HAL_DCMIPP_CSI_SetVCConfig(&hcamera_dcmipp, DCMIPP_VIRTUAL_CHANNEL0, DCMIPP_CSI_DT_BPP8);
  if (ret != HAL_OK)
  {
    return CMW_ERROR_PERIPH_FAILURE;
  }

  csi_pipe_conf.DataTypeMode = DCMIPP_DTMODE_DTIDA;
  csi_pipe_conf.DataTypeIDA = DCMIPP_DT_RAW8;
  csi_pipe_conf.DataTypeIDB = 0;
  /* Pre-initialize CSI config for all the pipes */
  for (uint32_t i = DCMIPP_PIPE0; i <= DCMIPP_PIPE2; i++)
  {
    ret = HAL_DCMIPP_CSI_PIPE_SetConfig(&hcamera_dcmipp, i, &csi_pipe_conf);
    if (ret != HAL_OK)
    {
      return CMW_ERROR_PERIPH_FAILURE;
    }
  }

  return CMW_ERROR_NONE;
}
#endif

#if defined(USE_IMX335_SENSOR)
static int32_t CMW_CAMERA_IMX335_Init( CMW_Sensor_Init_t *initSensors_params)
{
  int32_t ret = CMW_ERROR_NONE;
  DCMIPP_CSI_ConfTypeDef csi_conf = { 0 };
  DCMIPP_CSI_PIPE_ConfTypeDef csi_pipe_conf = { 0 };

  memset(&camera_bsp, 0, sizeof(camera_bsp));
  camera_bsp.imx335_bsp.Address     = CAMERA_IMX335_ADDRESS;
  camera_bsp.imx335_bsp.Init        = CMW_I2C_INIT;
  camera_bsp.imx335_bsp.DeInit      = CMW_I2C_DEINIT;
  camera_bsp.imx335_bsp.ReadReg     = CMW_I2C_READREG16;
  camera_bsp.imx335_bsp.WriteReg    = CMW_I2C_WRITEREG16;
  camera_bsp.imx335_bsp.GetTick     = BSP_GetTick;
  camera_bsp.imx335_bsp.Delay       = HAL_Delay;
  camera_bsp.imx335_bsp.ShutdownPin = CMW_CAMERA_ShutdownPin;
  camera_bsp.imx335_bsp.EnablePin   = CMW_CAMERA_EnablePin;
  camera_bsp.imx335_bsp.hdcmipp     = &hcamera_dcmipp;
  camera_bsp.imx335_bsp.appliHelpers.SetSensorGain = CB_ISP_SetSensorGain;
  camera_bsp.imx335_bsp.appliHelpers.GetSensorGain = CB_ISP_GetSensorGain;
  camera_bsp.imx335_bsp.appliHelpers.SetSensorExposure = CB_ISP_SetSensorExposure;
  camera_bsp.imx335_bsp.appliHelpers.GetSensorExposure = CB_ISP_GetSensorExposure;
  camera_bsp.imx335_bsp.appliHelpers.GetSensorInfo = CB_ISP_GetSensorInfo;

  ret = CMW_IMX335_Probe(&camera_bsp.imx335_bsp, &Camera_Drv);
  if (ret != CMW_ERROR_NONE)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  /* Special case: when resolution is not specified take the full sensor resolution */
  if ((initSensors_params->width == 0) || (initSensors_params->height == 0))
  {
    ISP_SensorInfoTypeDef sensor_info;
    Camera_Drv.GetSensorInfo(&camera_bsp, &sensor_info);
    initSensors_params->width = sensor_info.width;
    initSensors_params->height = sensor_info.height;
  }

  ret = Camera_Drv.Init(&camera_bsp, initSensors_params);
  if (ret != CMW_ERROR_NONE)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  ret = Camera_Drv.SetFrequency(&camera_bsp, IMX335_INCK_24MHZ);
  if (ret != CMW_ERROR_NONE)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  csi_conf.NumberOfLanes = DCMIPP_CSI_TWO_DATA_LANES;
  csi_conf.DataLaneMapping = DCMIPP_CSI_PHYSICAL_DATA_LANES;
  csi_conf.PHYBitrate = DCMIPP_CSI_PHY_BT_1600;
  ret = HAL_DCMIPP_CSI_SetConfig(&hcamera_dcmipp, &csi_conf);
  if (ret != HAL_OK)
  {
    return CMW_ERROR_PERIPH_FAILURE;
  }

  ret = HAL_DCMIPP_CSI_SetVCConfig(&hcamera_dcmipp, DCMIPP_VIRTUAL_CHANNEL0, DCMIPP_CSI_DT_BPP10);
  if (ret != HAL_OK)
  {
    return CMW_ERROR_PERIPH_FAILURE;
  }

  csi_pipe_conf.DataTypeMode = DCMIPP_DTMODE_DTIDA;
  csi_pipe_conf.DataTypeIDA = DCMIPP_DT_RAW10;
  csi_pipe_conf.DataTypeIDB = 0;
  /* Pre-initialize CSI config for all the pipes */
  for (uint32_t i = DCMIPP_PIPE0; i <= DCMIPP_PIPE2; i++)
  {
    ret = HAL_DCMIPP_CSI_PIPE_SetConfig(&hcamera_dcmipp, i, &csi_pipe_conf);
    if (ret != HAL_OK)
    {
      return CMW_ERROR_PERIPH_FAILURE;
    }
  }


  return ret;
}
#endif

static int32_t CMW_CAMERA_SetPipe(DCMIPP_HandleTypeDef *hdcmipp, uint32_t pipe, CMW_DCMIPP_Conf_t *p_conf, uint32_t *pitch)
{
  DCMIPP_DecimationConfTypeDef dec_conf = { 0 };
  DCMIPP_PipeConfTypeDef pipe_conf = { 0 };
  DCMIPP_DownsizeTypeDef down_conf = { 0 };
  DCMIPP_CropConfTypeDef crop_conf = { 0 };
  int ret;

  /* specific case for pipe0 which is only a dump pipe */
  if (pipe == DCMIPP_PIPE0)
  {
    /*  TODO: properly configure the dump pipe with decimation and crop */
    pipe_conf.FrameRate = DCMIPP_FRAME_RATE_ALL;
    ret = HAL_DCMIPP_PIPE_SetConfig(hdcmipp, pipe, &pipe_conf);
    if (ret != HAL_OK)
    {
      return CMW_ERROR_COMPONENT_FAILURE;
    }

    return HAL_OK;
  }

  CMW_UTILS_GetPipeConfig(camera_conf.width, camera_conf.height, p_conf, &crop_conf, &dec_conf, &down_conf);

  if (crop_conf.VSize != 0 || crop_conf.HSize != 0)
  {
    ret = HAL_DCMIPP_PIPE_SetCropConfig(hdcmipp, pipe, &crop_conf);
    if (ret != HAL_OK)
    {
      return CMW_ERROR_COMPONENT_FAILURE;
    }

    ret = HAL_DCMIPP_PIPE_EnableCrop(hdcmipp, pipe);
    if (ret != HAL_OK)
    {
      return CMW_ERROR_COMPONENT_FAILURE;
    }
  }
  else
  {
    ret = HAL_DCMIPP_PIPE_DisableCrop(hdcmipp, pipe);
    if (ret != HAL_OK)
    {
      return CMW_ERROR_COMPONENT_FAILURE;
    }
  }

  if (dec_conf.VRatio != 0 || dec_conf.HRatio != 0)
  {
    ret = HAL_DCMIPP_PIPE_SetDecimationConfig(hdcmipp, pipe, &dec_conf);
    if (ret != HAL_OK)
    {
      return CMW_ERROR_COMPONENT_FAILURE;
    }

    ret = HAL_DCMIPP_PIPE_EnableDecimation(hdcmipp, pipe);
    if (ret != HAL_OK)
    {
      return CMW_ERROR_COMPONENT_FAILURE;
    }
  }
  else
  {
    ret = HAL_DCMIPP_PIPE_DisableDecimation(hdcmipp, pipe);
    if (ret != HAL_OK)
    {
      return CMW_ERROR_COMPONENT_FAILURE;
    }
  }

  ret = HAL_DCMIPP_PIPE_SetDownsizeConfig(hdcmipp, pipe, &down_conf);
  if (ret != HAL_OK)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  ret = HAL_DCMIPP_PIPE_EnableDownsize(hdcmipp, pipe);
  if (ret != HAL_OK)
  {
    return CMW_ERROR_COMPONENT_FAILURE;
  }

  if (p_conf->enable_swap)
  {
    /* Config pipe */
    ret = HAL_DCMIPP_PIPE_EnableRedBlueSwap(hdcmipp, pipe);
    if (ret != HAL_OK)
    {
      return CMW_ERROR_COMPONENT_FAILURE;
    }
  }
  else
  {
    ret = HAL_DCMIPP_PIPE_DisableRedBlueSwap(hdcmipp, pipe);
    if (ret != HAL_OK)
    {
      return CMW_ERROR_COMPONENT_FAILURE;
    }
  }

  /* Ignore the configuration of gamma if -1
   * Activation is then done by the ISP Library
   */
  if (p_conf->enable_gamma_conversion > -1)
  {
    if (p_conf->enable_gamma_conversion)
    {
      ret = HAL_DCMIPP_PIPE_EnableGammaConversion(hdcmipp, pipe);
      if (ret != HAL_OK)
      {
        return CMW_ERROR_COMPONENT_FAILURE;
      }
    }
    else
    {
      ret = HAL_DCMIPP_PIPE_DisableGammaConversion(hdcmipp, pipe);
      if (ret != HAL_OK)
      {
        return CMW_ERROR_COMPONENT_FAILURE;
      }
    }
  }

  if (pipe == DCMIPP_PIPE2)
  {
    if (!is_pipe1_2_shared)
    {
      ret = HAL_DCMIPP_PIPE_CSI_EnableShare(hdcmipp, pipe);
      if (ret != HAL_OK)
      {
        return CMW_ERROR_COMPONENT_FAILURE;
      }
      is_pipe1_2_shared++;
    }
  }

  pipe_conf.FrameRate = DCMIPP_FRAME_RATE_ALL;
  pipe_conf.PixelPipePitch = p_conf->output_width * p_conf->output_bpp;
  /* Hardware constraint, pitch must be multiple of 16 */
  pipe_conf.PixelPipePitch = (pipe_conf.PixelPipePitch + 15) & (uint32_t) ~15;
  pipe_conf.PixelPackerFormat = p_conf->output_format;
  if (hcamera_dcmipp.PipeState[pipe] == HAL_DCMIPP_PIPE_STATE_RESET)
  {
    ret = HAL_DCMIPP_PIPE_SetConfig(hdcmipp, pipe, &pipe_conf);
    if (ret != HAL_OK)
    {
      return CMW_ERROR_COMPONENT_FAILURE;
    }
  }
  else
  {
    if (HAL_DCMIPP_PIPE_SetPixelPackerFormat(hdcmipp, pipe, pipe_conf.PixelPackerFormat) != HAL_OK)
    {
      return CMW_ERROR_COMPONENT_FAILURE;
    }

    if (HAL_DCMIPP_PIPE_SetPitch(hdcmipp, pipe, pipe_conf.PixelPipePitch) != HAL_OK)
    {
      return CMW_ERROR_COMPONENT_FAILURE;
    }
  }

  /* Update the pitch field so that application can use this information for
   * buffer alignement */
  *pitch = pipe_conf.PixelPipePitch;

  return HAL_OK;
}

