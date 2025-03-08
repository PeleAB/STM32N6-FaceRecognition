/**
  ******************************************************************************
  * @file    stm32n6570_discovery_lcd.c
  * @author  MCD Application Team
  * @brief   This file includes the driver for Liquid Crystal Display (LCD) module
  *          mounted on STM32N6570_DK board.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  @verbatim
  How To use this driver:
  --------------------------
   - This driver is used to drive directly a LCD TFT using the LTDC controller.
   - This driver uses the adequate timing and setting for the RK050HR18 LCD component

  Driver description:
  ---------------------
   + Initialization steps:
     o Initialize the LCD in default mode using the BSP_LCD_Init() function with the
       following settings:
        - Pixelformat : LCD_PIXEL_FORMAT_RGB888
        - Orientation : LCD_ORIENTATION_LANDSCAPE.
        - Width       : LCD_DEFAULT_WIDTH (800)
        - Height      : LCD_DEFAULT_HEIGHT(480)
       The default LTDC layer configured is layer 0.
       BSP_LCD_Init() includes LTDC, LTDC Layer and clock configurations by calling:
        - MX_LTDC_ClockConfig()
        - MX_LTDC_Init()
        - MX_LTDC_ConfigLayer()

     o Initialize the LCD with required parameters using the BSP_LCD_InitEx() function.

     o Select the LCD layer to be used using the BSP_LCD_SelectLayer() function.
     o Enable the LCD display using the BSP_LCD_DisplayOn() function.
     o Disable the LCD display using the BSP_LCD_DisplayOff() function.
     o Write a pixel to the LCD memory using the BSP_LCD_WritePixel() function.
     o Read a pixel from the LCD memory using the BSP_LCD_ReadPixel() function.
     o Draw an horizontal line using the BSP_LCD_DrawHLine() function.
     o Draw a vertical line using the BSP_LCD_DrawVLine() function.
     o Draw a bitmap image using the BSP_LCD_DrawBitmap() function.

   + Options
     o Configure the LTDC reload mode by calling BSP_LCD_Reload(). By default, the
       reload mode is set to BSP_LCD_RELOAD_IMMEDIATE then LTDC is reloaded immediately.
       To control the reload mode:
         - Call BSP_LCD_Reload() with ReloadType parameter set to BSP_LCD_RELOAD_NONE
         - Configure LTDC (color keying, transparency ..)
         - Call BSP_LCD_Reload() with ReloadType parameter set to BSP_LCD_RELOAD_IMMEDIATE
           for immediate reload or BSP_LCD_RELOAD_VERTICAL_BLANKING for LTDC reload
           in the next vertical blanking
     o Configure LTDC layers using BSP_LCD_ConfigLayer()
     o Control layer visibility using BSP_LCD_SetLayerVisible()
     o Configure and enable the color keying functionality using the
       BSP_LCD_SetColorKeying() function.
     o Disable the color keying functionality using the BSP_LCD_ResetColorKeying() function.
     o Modify on the fly the transparency and/or the frame buffer address
       using the following functions:
       - BSP_LCD_SetTransparency()
       - BSP_LCD_SetLayerAddress()

   + Display on LCD
     o To draw and fill a basic shapes (dot, line, rectangle, circle, ellipse, .. bitmap)
       on LCD and display text, utility basic_gui.c/.h must be called. Once the LCD is initialized,
       user should call GUI_SetFuncDriver() API to link board LCD drivers to BASIC GUI LCD drivers.
       The basic gui services, defined in basic_gui utility, are ready for use.

  Note:
  --------
    Regarding the "Instance" parameter, needed for all functions, it is used to select
    an LCD instance. On the STM32N6570 Discovery board, there's one instance. Then, this
    parameter should be 0.

  @endverbatim
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32n6570_discovery_lcd.h"

/** @addtogroup BSP
  * @{
  */

/** @addtogroup STM32N6570_DK
  * @{
  */

/** @defgroup STM32N6570_DK_LCD LCD
  * @{
  */

/** @defgroup STM32N6570_DK_LCD_Private_Variables LCD Private Variables
  * @{
  */

/**
  * @}
  */

/** @defgroup STM32N6570_DK_LCD_Private_TypesDefinitions LCD Private TypesDefinitions
  * @{
  */
const LCD_UTILS_Drv_t LCD_Driver =
{
  BSP_LCD_DrawBitmap,
  BSP_LCD_FillRGBRect,
  BSP_LCD_DrawHLine,
  BSP_LCD_DrawVLine,
  BSP_LCD_FillRect,
  BSP_LCD_ReadPixel,
  BSP_LCD_WritePixel,
  BSP_LCD_GetXSize,
  BSP_LCD_GetYSize,
  BSP_LCD_SetActiveLayer,
  BSP_LCD_GetPixelFormat
};

/**
  * @}
  */

/** @defgroup STM32N6570_DK_LCD_Exported_Variables LCD Exported Variables
  * @{
  */
DMA2D_HandleTypeDef hlcd_dma2d;
LTDC_HandleTypeDef  hlcd_ltdc;
BSP_LCD_Ctx_t       Lcd_Ctx[LCD_INSTANCES_NBR];
/**
  * @}
  */

/** @defgroup STM32N6570_DK_LCD_Private_FunctionPrototypes LCD Private FunctionPrototypes
  * @{
  */

static void LTDC_MspInit(LTDC_HandleTypeDef *hltdc);
static void LTDC_MspDeInit(LTDC_HandleTypeDef *hltdc);
static void DMA2D_MspInit(DMA2D_HandleTypeDef *hdma2d);
static void DMA2D_MspDeInit(DMA2D_HandleTypeDef *hdma2d);
static void LL_FillBuffer(uint32_t Instance, uint32_t *pDst, uint32_t xSize, uint32_t ySize, uint32_t OffLine, uint32_t Color);
static void LL_ConvertLineToRGB(uint32_t Instance, uint32_t *pSrc, uint32_t *pDst, uint32_t xSize, uint32_t ColorMode);
/**
  * @}
  */
/** @defgroup STM32N6570_DK_LCD_Private_Macros  Private Macros
  * @{
  */
#define CONVERTRGB5652ARGB8888(Color)((((((((Color) >> (11U)) & 0x1FU) * 527U) + 23U) >> (6U)) << (16U)) |\
                                     (((((((Color) >> (5U)) & 0x3FU) * 259U) + 33U) >> (6U)) << (8U)) |\
                                     (((((Color) & 0x1FU) * 527U) + 23U) >> (6U)) | (0xFF000000U))

#define CONVERTARGB44442ARGB8888(Color)((((((Color) >> 12U) & 0xFU) * 17U) << 24U) |\
                                        (((((Color) >>  8U) & 0xFU) * 17U) << 16U) |\
                                        (((((Color) >>  4U) & 0xFU) * 17U) << 8U) |\
                                        (((((Color) >>  0U) & 0xFU) * 17U) << 0U))

#define CONVERTRGB8882ARGB8888(Color)((Color) | 0xFF000000U)

#define CONVERTARGB88882RGB888(Color)((Color) & 0x00FFFFFFU)

/**
  * @}
  */

/** @defgroup STM32N6570_DK_LCD_Exported_Functions LCD Exported Functions
  * @{
  */
/**
  * @brief  Initializes the LCD in default mode.
  * @param  Instance    LCD Instance
  * @param  Orientation LCD_ORIENTATION_LANDSCAPE
  * @retval BSP status
  */

int32_t BSP_LCD_Init(uint32_t Instance, uint32_t Orientation)
{
  return BSP_LCD_InitEx(Instance, Orientation, LCD_PIXEL_FORMAT_RGB565, LCD_DEFAULT_WIDTH, LCD_DEFAULT_HEIGHT);
}

/**
  * @brief  Initializes the LCD.
  * @param  Instance    LCD Instance
  * @param  Orientation LCD_ORIENTATION_LANDSCAPE
  * @param  PixelFormat LCD_PIXEL_FORMAT_RGB565, LCD_PIXEL_FORMAT_ARGB4444, LCD_PIXEL_FORMAT_ARGB8888
  *                     or LCD_PIXEL_FORMAT_RGB888
  * @param  Width       Display width
  * @param  Height      Display height
  * @retval BSP status
  */
int32_t BSP_LCD_InitEx(uint32_t Instance, uint32_t Orientation, uint32_t PixelFormat, uint32_t Width, uint32_t Height)
{
  int32_t ret = BSP_ERROR_NONE;
  uint32_t ltdc_pixel_format;
  MX_LTDC_LayerConfig_t config = {0};

  if ((Orientation > LCD_ORIENTATION_LANDSCAPE) || (Instance >= LCD_INSTANCES_NBR) || \
     ((PixelFormat != LCD_PIXEL_FORMAT_RGB565) && (PixelFormat != LCD_PIXEL_FORMAT_RGB888) && \
      (PixelFormat != LCD_PIXEL_FORMAT_ARGB8888) &&  (PixelFormat != LCD_PIXEL_FORMAT_ARGB4444)))
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if (PixelFormat == LCD_PIXEL_FORMAT_RGB565)
    {
      ltdc_pixel_format = LTDC_PIXEL_FORMAT_RGB565;
      Lcd_Ctx[Instance].BppFactor = 2U;
    }
    else if  (PixelFormat == LCD_PIXEL_FORMAT_RGB888)
    {
      ltdc_pixel_format = LTDC_PIXEL_FORMAT_RGB888;
      Lcd_Ctx[Instance].BppFactor = 3U;
    }
    else if  (PixelFormat == LCD_PIXEL_FORMAT_ARGB4444)
    {
      ltdc_pixel_format = LTDC_PIXEL_FORMAT_ARGB4444;
      Lcd_Ctx[Instance].BppFactor = 2U;
    }
    else /* LCD_PIXEL_FORMAT_ARGB8888 */
    {
      ltdc_pixel_format = LTDC_PIXEL_FORMAT_ARGB8888;
      Lcd_Ctx[Instance].BppFactor = 4U;
    }

    /* Store pixel format, xsize and ysize information */
    Lcd_Ctx[Instance].PixelFormat = PixelFormat;
    Lcd_Ctx[Instance].XSize  = Width;
    Lcd_Ctx[Instance].YSize  = Height;

    /* Initializes peripherals instance value */
    hlcd_ltdc.Instance = LTDC;
    hlcd_dma2d.Instance = DMA2D;

    /* MSP initialization */
#if (USE_HAL_LTDC_REGISTER_CALLBACKS == 1)
    /* Register the LTDC MSP Callbacks */
    if (Lcd_Ctx[Instance].IsMspCallbacksValid == 0U)
    {
      if (BSP_LCD_RegisterDefaultMspCallbacks(0) != BSP_ERROR_NONE)
      {
        return BSP_ERROR_PERIPH_FAILURE;
      }
    }
#else
    LTDC_MspInit(&hlcd_ltdc);
#endif /* (USE_HAL_LTDC_REGISTER_CALLBACKS == 1) */

    DMA2D_MspInit(&hlcd_dma2d);

    if (MX_LTDC_ClockConfig(&hlcd_ltdc) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else
    {
      if (MX_LTDC_Init(&hlcd_ltdc, Width, Height) != HAL_OK)
      {
        ret = BSP_ERROR_PERIPH_FAILURE;
      }
    }

    if (ret == BSP_ERROR_NONE)
    {
#if defined(DATA_IN_ExtRAM)
      /* Before configuring LTDC layer, ensure XSPI RAM is initialized */
      /* Initialize the XSPI RAM */
      if (BSP_XSPI_RAM_Init(0) != BSP_ERROR_NONE)
      {
        ret = BSP_ERROR_NO_INIT;
      }
      else if (BSP_XSPI_RAM_EnableMemoryMappedMode(0) != BSP_ERROR_NONE)
      {
        ret = BSP_ERROR_PERIPH_FAILURE;
      }
      else
      {
#endif /* DATA_IN_ExtRAM */
        /* Configure default LTDC Layer 0. This configuration can be override by calling
        BSP_LCD_ConfigLayer() at application level */
        config.X0          = 0;
        config.X1          = Width;
        config.Y0          = 0;
        config.Y1          = Height;
        config.PixelFormat = ltdc_pixel_format;
        config.Address     = LCD_LAYER_0_ADDRESS;

        if (MX_LTDC_ConfigLayer(&hlcd_ltdc, 0, &config) != HAL_OK)
        {
          ret = BSP_ERROR_PERIPH_FAILURE;
        }

        /* By default the reload is activated and executed immediately */
        Lcd_Ctx[Instance].ReloadEnable = 1U;
#if defined(DATA_IN_ExtRAM)
      }
#endif /* DATA_IN_ExtRAM */
    }
  }

  return ret;
}

/**
  * @brief  De-Initializes the LCD resources.
  * @param  Instance    LCD Instance
  * @retval BSP status
  */
int32_t BSP_LCD_DeInit(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
#if (USE_HAL_LTDC_REGISTER_CALLBACKS == 0)
    LTDC_MspDeInit(&hlcd_ltdc);
#endif /* (USE_HAL_LTDC_REGISTER_CALLBACKS == 0) */

    (void)HAL_LTDC_DeInit(&hlcd_ltdc);
    if(HAL_DMA2D_DeInit(&hlcd_dma2d) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else
    {
      DMA2D_MspDeInit(&hlcd_dma2d);
      Lcd_Ctx[Instance].IsMspCallbacksValid = 0;
    }
  }

  return ret;
}

/**
  * @brief  Initializes the LTDC.
  * @param  hltdc  LTDC handle
  * @param  Width  LTDC width
  * @param  Height LTDC height
  * @retval HAL status
  */
__weak HAL_StatusTypeDef MX_LTDC_Init(LTDC_HandleTypeDef *hltdc, uint32_t Width, uint32_t Height)
{
  hltdc->Instance = LTDC;
  hltdc->Init.HSPolarity = LTDC_HSPOLARITY_AL;
  hltdc->Init.VSPolarity = LTDC_VSPOLARITY_AL;
  hltdc->Init.DEPolarity = LTDC_DEPOLARITY_AL;
  hltdc->Init.PCPolarity = LTDC_PCPOLARITY_IPC;

  hltdc->Init.HorizontalSync     = (uint32_t)RK050HR18_HSYNC - 1U;
  hltdc->Init.AccumulatedHBP     = (uint32_t)RK050HR18_HSYNC + (uint32_t)RK050HR18_HBP - 1U;
  hltdc->Init.AccumulatedActiveW = (uint32_t)RK050HR18_HSYNC + Width + (uint32_t)RK050HR18_HBP -1U;
  hltdc->Init.TotalWidth         = (uint32_t)RK050HR18_HSYNC + Width + (uint32_t)RK050HR18_HBP + RK050HR18_HFP - 1U;
  hltdc->Init.VerticalSync       = (uint32_t)RK050HR18_VSYNC - 1U;
  hltdc->Init.AccumulatedVBP     = (uint32_t)RK050HR18_VSYNC + (uint32_t)RK050HR18_VBP - 1U;
  hltdc->Init.AccumulatedActiveH = (uint32_t)RK050HR18_VSYNC + Height + (uint32_t)RK050HR18_VBP -1U;
  hltdc->Init.TotalHeigh         = (uint32_t)RK050HR18_VSYNC + Height + (uint32_t)RK050HR18_VBP + RK050HR18_VFP - 1U;

  hltdc->Init.Backcolor.Blue  = 0x0;
  hltdc->Init.Backcolor.Green = 0x0;
  hltdc->Init.Backcolor.Red   = 0x0;

  return HAL_LTDC_Init(hltdc);
}

/**
  * @brief  MX LTDC layer configuration.
  * @param  hltdc      LTDC handle
  * @param  LayerIndex Layer 0 or 1
  * @param  Config     Layer configuration
  * @retval HAL status
  */
__weak HAL_StatusTypeDef MX_LTDC_ConfigLayer(LTDC_HandleTypeDef *hltdc, uint32_t LayerIndex, MX_LTDC_LayerConfig_t *Config)
{
  LTDC_LayerCfgTypeDef pLayerCfg ={0};

  pLayerCfg.WindowX0 = Config->X0;
  pLayerCfg.WindowX1 = Config->X1;
  pLayerCfg.WindowY0 = Config->Y0;
  pLayerCfg.WindowY1 = Config->Y1;
  pLayerCfg.PixelFormat = Config->PixelFormat;
  pLayerCfg.Alpha = LTDC_LxCACR_CONSTA;
  pLayerCfg.Alpha0 = 0;
  pLayerCfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA;
  pLayerCfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA;
  pLayerCfg.FBStartAdress = Config->Address;
  pLayerCfg.ImageWidth = (Config->X1 - Config->X0);
  pLayerCfg.ImageHeight = (Config->Y1 - Config->Y0);
  pLayerCfg.Backcolor.Blue = 0;
  pLayerCfg.Backcolor.Green = 0;
  pLayerCfg.Backcolor.Red = 0;
  return HAL_LTDC_ConfigLayer(hltdc, &pLayerCfg, LayerIndex);
}

/**
  * @brief  LTDC Clock Config for LCD DPI display.
  * @param  hltdc  LTDC Handle
  *         Being __weak it can be overwritten by the application
  * @retval HAL_status
  */
__weak HAL_StatusTypeDef MX_LTDC_ClockConfig(LTDC_HandleTypeDef *hltdc)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hltdc);

  HAL_StatusTypeDef   status =  HAL_OK;
  RCC_PeriphCLKInitTypeDef RCC_PeriphCLKInitStruct = {0};

  /* LCD clock configuration */
  /* Typical PCLK is 25 MHz so the PLL4 is configured to provide this clock */
  /* LTDC - PLL4 */
  /* Configure LTDC clock to IC16 with PLL4  */

  /* LCD clock configuration */
  /* Typical PCLK is 25 MHz so the PLL4 is configured to provide this clock */
  /* LCD clock configuration */
  /* PLL3_VCO Input = HSE_VALUE/PLLM = 48 Mhz / 192 = 0.25 */
  /* PLL3_VCO Output = PLL3_VCO Input * PLLN = 0.25 Mhz * 100 = 25 */
  /* PLLLCDCLK = PLL3_VCO Output/(PLLP1 * PLLP2) = 25/1 = 25Mhz */
  /* LTDC clock frequency = PLLLCDCLK = 25 Mhz */

  RCC_PeriphCLKInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
  RCC_PeriphCLKInitStruct.LtdcClockSelection = RCC_LTDCCLKSOURCE_IC16;
  RCC_PeriphCLKInitStruct.ICSelection[RCC_IC16].ClockSelection = RCC_ICCLKSOURCE_PLL4;
  RCC_PeriphCLKInitStruct.ICSelection[RCC_IC16].ClockDivider = 2;
  if (HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphCLKInitStruct) != HAL_OK)
  {
    status = HAL_ERROR;
  }

  return status;
}

#if (USE_HAL_LTDC_REGISTER_CALLBACKS == 1)
/**
  * @brief Default BSP LCD Msp Callbacks
  * @param Instance BSP LCD Instance
  * @retval BSP status
  */
int32_t BSP_LCD_RegisterDefaultMspCallbacks (uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if(HAL_LTDC_RegisterCallback(&hlcd_ltdc, HAL_LTDC_MSPINIT_CB_ID, LTDC_MspInit) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else
    {
      if(HAL_LTDC_RegisterCallback(&hlcd_ltdc, HAL_LTDC_MSPDEINIT_CB_ID, LTDC_MspDeInit) != HAL_OK)
      {
        ret = BSP_ERROR_PERIPH_FAILURE;
      }
    }

    Lcd_Ctx[Instance].IsMspCallbacksValid = 1;
  }

  return ret;
}

/**
  * @brief BSP LCD Msp Callback registering
  * @param Instance    LCD Instance
  * @param CallBacks   pointer to LCD MspInit/MspDeInit functions
  * @retval BSP status
  */
int32_t BSP_LCD_RegisterMspCallbacks (uint32_t Instance, BSP_LCD_Cb_t *CallBacks)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if(HAL_LTDC_RegisterCallback(&hlcd_ltdc, HAL_LTDC_MSPINIT_CB_ID, CallBacks->pMspLtdcInitCb) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else
    {
      if(HAL_LTDC_RegisterCallback(&hlcd_ltdc, HAL_LTDC_MSPDEINIT_CB_ID, CallBacks->pMspLtdcDeInitCb) != HAL_OK)
      {
        ret = BSP_ERROR_PERIPH_FAILURE;
      }
    }

    Lcd_Ctx[Instance].IsMspCallbacksValid = 1;
  }

  return ret;
}
#endif /*(USE_HAL_LTDC_REGISTER_CALLBACKS == 1) */

/**
  * @brief  LTDC layer configuration.
  * @param  Instance   LCD instance
  * @param  LayerIndex Layer 0 or 1
  * @param  Config     Layer configuration
  * @retval HAL status
  */
int32_t BSP_LCD_ConfigLayer(uint32_t Instance, uint32_t LayerIndex, BSP_LCD_LayerConfig_t *Config)
{
  int32_t ret = BSP_ERROR_NONE;
  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if (Config->PixelFormat ==  LCD_PIXEL_FORMAT_RGB565)
    {
      Config->PixelFormat = LTDC_PIXEL_FORMAT_RGB565;
    }
    else if (Config->PixelFormat ==  LCD_PIXEL_FORMAT_ARGB4444)
    {
      Config->PixelFormat = LTDC_PIXEL_FORMAT_ARGB4444;
    }
    else if (Config->PixelFormat ==  LCD_PIXEL_FORMAT_ARGB8888)
    {
      Config->PixelFormat = LTDC_PIXEL_FORMAT_ARGB8888;
    }
    else
    {
      Config->PixelFormat = LTDC_PIXEL_FORMAT_RGB888;
    }
    if (MX_LTDC_ConfigLayer(&hlcd_ltdc, LayerIndex, Config) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
  }
  return ret;
}

/**
  * @brief  Gets the LCD Active LCD Pixel Format.
  * @param  Instance    LCD Instance
  * @param  PixelFormat Active LCD Pixel Format
  * @retval BSP status
  */
int32_t BSP_LCD_GetPixelFormat(uint32_t Instance, uint32_t *PixelFormat)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    /* Only RGB565 format is supported */
    *PixelFormat =  Lcd_Ctx[Instance].PixelFormat;
  }

  return ret;
}

/**
  * @brief  Set the LCD Active Layer.
  * @param  Instance    LCD Instance
  * @param  LayerIndex  LCD layer index
  * @retval BSP status
  */
int32_t BSP_LCD_SetActiveLayer(uint32_t Instance, uint32_t LayerIndex)
{
  int32_t ret = BSP_ERROR_NONE;
  LTDC_LayerCfgTypeDef *pLayerCfg;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    pLayerCfg = &hlcd_ltdc.LayerCfg[LayerIndex];

    Lcd_Ctx[Instance].ActiveLayer = LayerIndex;
    Lcd_Ctx[Instance].XSize = pLayerCfg->ImageWidth;
    Lcd_Ctx[Instance].YSize = pLayerCfg->ImageHeight;

    if (pLayerCfg->PixelFormat ==  LTDC_PIXEL_FORMAT_RGB565)
    {
      Lcd_Ctx[Instance].BppFactor = 2;
      Lcd_Ctx[Instance].PixelFormat = LCD_PIXEL_FORMAT_RGB565;
    }
    else if (pLayerCfg->PixelFormat ==  LTDC_PIXEL_FORMAT_ARGB4444)
    {
      Lcd_Ctx[Instance].BppFactor = 2;
      Lcd_Ctx[Instance].PixelFormat = LCD_PIXEL_FORMAT_ARGB4444;
    }
    else if (pLayerCfg->PixelFormat ==  LTDC_PIXEL_FORMAT_RGB888)
    {
      Lcd_Ctx[Instance].BppFactor = 3;
      Lcd_Ctx[Instance].PixelFormat = LCD_PIXEL_FORMAT_RGB888;
    }
    else
    {
      Lcd_Ctx[Instance].BppFactor = 4;
      Lcd_Ctx[Instance].PixelFormat = LCD_PIXEL_FORMAT_ARGB8888;
    }
  }

  return ret;
}

/**
  * @brief  Control the LTDC reload
  * @param  Instance    LCD Instance
  * @param  ReloadType can be one of the following values
  *         - BSP_LCD_RELOAD_NONE
  *         - BSP_LCD_RELOAD_IMMEDIATE
  *         - BSP_LCD_RELOAD_VERTICAL_BLANKING
  * @retval BSP status
  */
int32_t BSP_LCD_Reload(uint32_t Instance, uint32_t ReloadType)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if(ReloadType == BSP_LCD_RELOAD_NONE)
  {
    Lcd_Ctx[Instance].ReloadEnable = 0U;
  }
  else if(HAL_LTDC_Reload(&hlcd_ltdc, ReloadType) != HAL_OK)
  {
    ret = BSP_ERROR_PERIPH_FAILURE;
  }
  else
  {
    Lcd_Ctx[Instance].ReloadEnable = 1U;
  }

  return ret;
}

/**
  * @brief  Sets an LCD Layer visible
  * @param  Instance    LCD Instance
  * @param  LayerIndex  Visible Layer
  * @param  State  New state of the specified layer
  *          This parameter can be one of the following values:
  *            @arg  ENABLE
  *            @arg  DISABLE
  * @retval BSP status
  */
int32_t BSP_LCD_SetLayerVisible(uint32_t Instance, uint32_t LayerIndex, FunctionalState State)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if(State == ENABLE)
    {
      __HAL_LTDC_LAYER_ENABLE(&hlcd_ltdc, LayerIndex);
    }
    else
    {
      __HAL_LTDC_LAYER_DISABLE(&hlcd_ltdc, LayerIndex);
    }

    if(Lcd_Ctx[Instance].ReloadEnable == 1U)
    {
      __HAL_LTDC_RELOAD_IMMEDIATE_CONFIG(&hlcd_ltdc);
    }
  }

  return ret;
}

/**
  * @brief  Configures the transparency.
  * @param  Instance      LCD Instance
  * @param  LayerIndex    Layer foreground or background.
  * @param  Transparency  Transparency
  *           This parameter must be a number between Min_Data = 0x00 and Max_Data = 0xFF
  * @retval BSP status
  */
int32_t BSP_LCD_SetTransparency(uint32_t Instance, uint32_t LayerIndex, uint8_t Transparency)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if(Lcd_Ctx[Instance].ReloadEnable == 1U)
    {
      (void)HAL_LTDC_SetAlpha(&hlcd_ltdc, Transparency, LayerIndex);
    }
    else
    {
      (void)HAL_LTDC_SetAlpha_NoReload(&hlcd_ltdc, Transparency, LayerIndex);
    }
  }

  return ret;
}

/**
  * @brief  Sets an LCD layer frame buffer address.
  * @param  Instance    LCD Instance
  * @param  LayerIndex  Layer foreground or background
  * @param  Address     New LCD frame buffer value
  * @retval BSP status
  */
int32_t BSP_LCD_SetLayerAddress(uint32_t Instance, uint32_t LayerIndex, uint32_t Address)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if(Lcd_Ctx[Instance].ReloadEnable == 1U)
    {
      (void)HAL_LTDC_SetAddress(&hlcd_ltdc, Address, LayerIndex);
    }
    else
    {
      (void)HAL_LTDC_SetAddress_NoReload(&hlcd_ltdc, Address, LayerIndex);
    }
  }

  return ret;
}

/**
  * @brief  Sets display window.
  * @param  Instance    LCD Instance
  * @param  LayerIndex  Layer index
  * @param  Xpos   LCD X position
  * @param  Ypos   LCD Y position
  * @param  Width  LCD window width
  * @param  Height LCD window height
  * @retval BSP status
  */
int32_t BSP_LCD_SetLayerWindow(uint32_t Instance, uint16_t LayerIndex, uint16_t Xpos, uint16_t Ypos, uint16_t Width, uint16_t Height)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if(Lcd_Ctx[Instance].ReloadEnable == 1U)
    {
      /* Reconfigure the layer size  and position */
      (void)HAL_LTDC_SetWindowSize(&hlcd_ltdc, Width, Height, LayerIndex);
      (void)HAL_LTDC_SetWindowPosition(&hlcd_ltdc, Xpos, Ypos, LayerIndex);
    }
    else
    {
      /* Reconfigure the layer size and position */
      (void)HAL_LTDC_SetWindowSize_NoReload(&hlcd_ltdc, Width, Height, LayerIndex);
      (void)HAL_LTDC_SetWindowPosition_NoReload(&hlcd_ltdc, Xpos, Ypos, LayerIndex);
    }

    Lcd_Ctx[Instance].XSize = Width;
    Lcd_Ctx[Instance].YSize = Height;
  }

  return ret;
}

/**
  * @brief  Configures and sets the color keying.
  * @param  Instance    LCD Instance
  * @param  LayerIndex  Layer foreground or background
  * @param  Color       Color reference
  * @retval BSP status
  */
int32_t BSP_LCD_SetColorKeying(uint32_t Instance, uint32_t LayerIndex, uint32_t Color)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    (void)HAL_LTDC_ConfigColorKeying(&hlcd_ltdc, Color, LayerIndex);
    if(Lcd_Ctx[Instance].ReloadEnable == 1U)
    {
      /* Configure and Enable the color Keying for LCD Layer */

      (void)HAL_LTDC_EnableColorKeying(&hlcd_ltdc, LayerIndex);
    }
    else
    {
      /* Configure and Enable the color Keying for LCD Layer */
      (void)HAL_LTDC_EnableColorKeying_NoReload(&hlcd_ltdc, LayerIndex);
    }
  }

  return ret;
}

/**
  * @brief  Disables the color keying.
  * @param  Instance    LCD Instance
  * @param  LayerIndex Layer foreground or background
  * @retval BSP status
  */
int32_t BSP_LCD_ResetColorKeying(uint32_t Instance, uint32_t LayerIndex)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if(Lcd_Ctx[Instance].ReloadEnable == 1U)
    {
      /* Disable the color Keying for LCD Layer */
      (void)HAL_LTDC_DisableColorKeying(&hlcd_ltdc, LayerIndex);
    }
    else
    {
      /* Disable the color Keying for LCD Layer */
      (void)HAL_LTDC_DisableColorKeying_NoReload(&hlcd_ltdc, LayerIndex);
    }
  }

  return ret;
}

/**
  * @brief  Gets the LCD X size.
  * @param  Instance  LCD Instance
  * @param  XSize     LCD width
  * @retval BSP status
  */
int32_t BSP_LCD_GetXSize(uint32_t Instance, uint32_t *XSize)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    *XSize = Lcd_Ctx[Instance].XSize;
  }

  return ret;
}

/**
  * @brief  Gets the LCD Y size.
  * @param  Instance  LCD Instance
  * @param  YSize     LCD Height
  * @retval BSP status
  */
int32_t BSP_LCD_GetYSize(uint32_t Instance, uint32_t *YSize)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    *YSize = Lcd_Ctx[Instance].YSize;
  }

  return ret;
}

/**
  * @brief  Switch On the display.
  * @param  Instance    LCD Instance
  * @retval BSP status
  */
int32_t BSP_LCD_DisplayOn(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;
  GPIO_InitTypeDef gpio_init_structure = {0};

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    __HAL_LTDC_ENABLE(&hlcd_ltdc);
    gpio_init_structure.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init_structure.Pull = GPIO_PULLUP;
    gpio_init_structure.Pin  = LCD_DISP_EN_PIN;
    HAL_GPIO_Init(LCD_DISP_EN_GPIO_PORT, &gpio_init_structure);

    /* Assert LCD_DISP_EN pin */
    HAL_GPIO_WritePin(LCD_DISP_EN_GPIO_PORT,LCD_DISP_EN_PIN, GPIO_PIN_SET);
  }

  return ret;
}

/**
  * @brief  Switch Off the display.
  * @param  Instance    LCD Instance
  * @retval BSP status
  */
int32_t BSP_LCD_DisplayOff(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;
  GPIO_InitTypeDef gpio_init_structure = {0};

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    gpio_init_structure.Mode      = GPIO_MODE_OUTPUT_PP;
    gpio_init_structure.Pull      = GPIO_NOPULL;
    gpio_init_structure.Speed     = GPIO_SPEED_FREQ_MEDIUM;
    gpio_init_structure.Pin       = LCD_DISP_EN_PIN;
    HAL_GPIO_Init(LCD_DISP_EN_GPIO_PORT, &gpio_init_structure);

    HAL_GPIO_WritePin(LCD_DISP_EN_GPIO_PORT,LCD_DISP_EN_PIN, GPIO_PIN_RESET);

    __HAL_LTDC_DISABLE(&hlcd_ltdc);

  }

  return ret;
}

/**
  * @brief  Draws a bitmap picture loaded in the internal Flash in currently active layer.
  * @param  Instance LCD Instance
  * @param  Xpos Bmp X position in the LCD
  * @param  Ypos Bmp Y position in the LCD
  * @param  pBmp Pointer to Bmp picture address in the internal Flash.
  * @retval BSP status
  */
int32_t BSP_LCD_DrawBitmap(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint8_t *pBmp)
{
  int32_t ret = BSP_ERROR_NONE;
  uint32_t index;
  uint32_t width;
  uint32_t height;
  uint32_t bit_pixel;
  uint32_t Address;
  uint32_t input_color_mode;
  uint8_t *pbmp;

  /* Get bitmap data address offset */
  index = (uint32_t)pBmp[10] + ((uint32_t)pBmp[11] << 8) + ((uint32_t)pBmp[12] << 16)  + ((uint32_t)pBmp[13] << 24);

  /* Read bitmap width */
  width = (uint32_t)pBmp[18] + ((uint32_t)pBmp[19] << 8) + ((uint32_t)pBmp[20] << 16)  + ((uint32_t)pBmp[21] << 24);

  /* Read bitmap height */
  height = (uint32_t)pBmp[22] + ((uint32_t)pBmp[23] << 8) + ((uint32_t)pBmp[24] << 16)  + ((uint32_t)pBmp[25] << 24);

  /* Read bit/pixel */
  bit_pixel = (uint32_t)pBmp[28] + ((uint32_t)pBmp[29] << 8);

  /* Set the address */
  Address = hlcd_ltdc.LayerCfg[Lcd_Ctx[Instance].ActiveLayer].FBStartAdress + (((Lcd_Ctx[Instance].XSize*Ypos) + Xpos)*Lcd_Ctx[Instance].BppFactor);

  /* Get the layer pixel format */
  if ((bit_pixel/8U) == 4U)
  {
    input_color_mode = DMA2D_INPUT_ARGB8888;
  }
  else if ((bit_pixel/8U) == 2U)
  {
    input_color_mode = DMA2D_INPUT_RGB565;
  }
  else
  {
    input_color_mode = DMA2D_INPUT_RGB888;
  }

  /* Bypass the bitmap header */
  pbmp = pBmp + (index + (width * (height - 1U) * (bit_pixel/8U)));

  /* Convert picture to ARGB8888 pixel format */
  for(index=0; index < height; index++)
  {
    /* Pixel format conversion */
    LL_ConvertLineToRGB(Instance, (uint32_t *)pbmp, (uint32_t *)Address, width, input_color_mode);

    /* Increment the source and destination buffers */
    Address+=  (Lcd_Ctx[Instance].XSize * Lcd_Ctx[Instance].BppFactor);
    pbmp -= width*(bit_pixel/8U);
  }

  return ret;
}

/**
  * @brief  Draw a horizontal line on LCD.
  * @param  Instance LCD Instance.
  * @param  Xpos X position.
  * @param  Ypos Y position.
  * @param  pData Pointer to RGB line data
  * @param  Width Rectangle width.
  * @param  Height Rectangle Height.
  * @retval BSP status.
  */
int32_t BSP_LCD_FillRGBRect(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint8_t *pData, uint32_t Width, uint32_t Height)
{
  uint32_t i;
  uint8_t *pdata = pData;

#if (USE_DMA2D_TO_FILL_RGB_RECT == 1)
  uint32_t  Xaddress;
  for(i = 0; i < Height; i++)
  {
    /* Get the line address */
    Xaddress = hlcd_ltdc.LayerCfg[Lcd_Ctx[Instance].ActiveLayer].FBStartAdress + (Lcd_Ctx[Instance].BppFactor*((Lcd_Ctx[Instance].XSize*(Ypos + i)) + Xpos));

#if (USE_BSP_CPU_CACHE_MAINTENANCE == 1)
    SCB_CleanDCache_by_Addr((uint32_t *)pdata, Lcd_Ctx[Instance].BppFactor*Lcd_Ctx[Instance].XSize);
#endif /* USE_BSP_CPU_CACHE_MAINTENANCE */

    /* Write line */
    if(Lcd_Ctx[Instance].PixelFormat == LCD_PIXEL_FORMAT_RGB565)
    {
      LL_ConvertLineToRGB(Instance, (uint32_t *)pdata, (uint32_t *)Xaddress, Width, DMA2D_INPUT_RGB565);
    }
    else if(Lcd_Ctx[Instance].PixelFormat == LCD_PIXEL_FORMAT_ARGB4444)
    {
      LL_ConvertLineToRGB(Instance, (uint32_t *)pdata, (uint32_t *)Xaddress, Width, DMA2D_INPUT_ARGB4444);
    }
    else if(Lcd_Ctx[Instance].PixelFormat == LCD_PIXEL_FORMAT_RGB888)
    {
      LL_ConvertLineToRGB(Instance, (uint32_t *)pdata, (uint32_t *)Xaddress, Width, DMA2D_INPUT_RGB888);
    }
    else
    {
      LL_ConvertLineToRGB(Instance, (uint32_t *)pdata, (uint32_t *)Xaddress, Width, DMA2D_INPUT_ARGB8888);
    }
    pdata += Lcd_Ctx[Instance].BppFactor*Width;
  }
#else
  uint32_t color;
  uint32_t j;
  for(i = 0; i < Height; i++)
  {
    for(j = 0; j < Width; j++)
    {
      color = (uint32_t)((uint32_t)*pdata | ((uint32_t)(*(pdata + 1U)) << 8U) | ((uint32_t)(*(pdata + 2U)) << 16U) | ((uint32_t)(*(pdata + 3U)) << 24U));
      (void)BSP_LCD_WritePixel(Instance, Xpos + j, Ypos + i, color);
      pdata += Lcd_Ctx[Instance].BppFactor;
    }
  }
#endif /* (USE_DMA2D_TO_FILL_RGB_RECT == 1) */

  return BSP_ERROR_NONE;
}

/**
  * @brief  Draws an horizontal line in currently active layer.
  * @param  Instance   LCD Instance
  * @param  Xpos  X position
  * @param  Ypos  Y position
  * @param  Length  Line length
  * @param  Color RGB color
  * @retval BSP status
  */
int32_t BSP_LCD_DrawHLine(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Length, uint32_t Color)
{
  uint32_t  Xaddress;

  /* Get the line address */
  Xaddress = hlcd_ltdc.LayerCfg[Lcd_Ctx[Instance].ActiveLayer].FBStartAdress + (Lcd_Ctx[Instance].BppFactor*((Lcd_Ctx[Instance].XSize*Ypos) + Xpos));

  /* Write line */
  LL_FillBuffer(Instance, (uint32_t *)Xaddress, Length, 1, 0, Color);

  return BSP_ERROR_NONE;
}

/**
  * @brief  Draws a vertical line in currently active layer.
  * @param  Instance   LCD Instance
  * @param  Xpos  X position
  * @param  Ypos  Y position
  * @param  Length  Line length
  * @param  Color RGB color
  * @retval BSP status
  */
int32_t BSP_LCD_DrawVLine(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Length, uint32_t Color)
{
  uint32_t  Xaddress;

  /* Get the line address */
  Xaddress = (hlcd_ltdc.LayerCfg[Lcd_Ctx[Instance].ActiveLayer].FBStartAdress) + (Lcd_Ctx[Instance].BppFactor*((Lcd_Ctx[Instance].XSize*Ypos) + Xpos));

  /* Write line */
  LL_FillBuffer(Instance, (uint32_t *)Xaddress, 1, Length, (Lcd_Ctx[Instance].XSize - 1U), Color);

  return BSP_ERROR_NONE;
}

/**
  * @brief  Draws a full rectangle in currently active layer.
  * @param  Instance   LCD Instance
  * @param  Xpos X position
  * @param  Ypos Y position
  * @param  Width Rectangle width
  * @param  Height Rectangle height
  * @param  Color RGB color
  * @retval BSP status
  */
int32_t BSP_LCD_FillRect(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Width, uint32_t Height, uint32_t Color)
{
  uint32_t  Xaddress;

  /* Get the rectangle start address */
  Xaddress = (hlcd_ltdc.LayerCfg[Lcd_Ctx[Instance].ActiveLayer].FBStartAdress) + (Lcd_Ctx[Instance].BppFactor*((Lcd_Ctx[Instance].XSize*Ypos) + Xpos));

  /* Fill the rectangle */
  LL_FillBuffer(Instance, (uint32_t *)Xaddress, Width, Height, (Lcd_Ctx[Instance].XSize - Width), Color);

  return BSP_ERROR_NONE;
}

/**
  * @brief  Reads a LCD pixel.
  * @param  Instance    LCD Instance
  * @param  Xpos X position
  * @param  Ypos Y position
  * @param  Color RGB pixel color
  * @retval BSP status
  */
int32_t BSP_LCD_ReadPixel(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t *Color)
{
  if(hlcd_ltdc.LayerCfg[Lcd_Ctx[Instance].ActiveLayer].PixelFormat == LTDC_PIXEL_FORMAT_ARGB8888)
  {
    /* Read data value from RAM memory */
    *Color = *(__IO uint32_t*) (hlcd_ltdc.LayerCfg[Lcd_Ctx[Instance].ActiveLayer].FBStartAdress + (4U*((Ypos*Lcd_Ctx[Instance].XSize) + Xpos)));
  }
  else if(hlcd_ltdc.LayerCfg[Lcd_Ctx[Instance].ActiveLayer].PixelFormat == LTDC_PIXEL_FORMAT_RGB888)
  {
    /* Read data value from RAM memory */
    *Color = *(__IO uint32_t*) (hlcd_ltdc.LayerCfg[Lcd_Ctx[Instance].ActiveLayer].FBStartAdress + (3U*((Ypos*Lcd_Ctx[Instance].XSize) + Xpos)));
    *Color = CONVERTARGB88882RGB888(*Color);
  }
  else /* if((hlcd_ltdc.LayerCfg[layer].PixelFormat == LTDC_PIXEL_FORMAT_RGB565) */
  {
    /* Read data value from RAM memory */
    *Color = *(__IO uint16_t*) (hlcd_ltdc.LayerCfg[Lcd_Ctx[Instance].ActiveLayer].FBStartAdress + (2U*((Ypos*Lcd_Ctx[Instance].XSize) + Xpos)));
  }

  return BSP_ERROR_NONE;
}

/**
  * @brief  Draws a pixel on LCD.
  * @param  Instance    LCD Instance
  * @param  Xpos X position
  * @param  Ypos Y position
  * @param  Color Pixel color
  * @retval BSP status
  */
int32_t BSP_LCD_WritePixel(uint32_t Instance, uint32_t Xpos, uint32_t Ypos, uint32_t Color)
{
  if(hlcd_ltdc.LayerCfg[Lcd_Ctx[Instance].ActiveLayer].PixelFormat == LTDC_PIXEL_FORMAT_ARGB8888)
  {
    /* Write data value to RAM memory */
    *(__IO uint32_t*) (hlcd_ltdc.LayerCfg[Lcd_Ctx[Instance].ActiveLayer].FBStartAdress + (4U*((Ypos*Lcd_Ctx[Instance].XSize) + Xpos))) = Color;
  }
  else if(hlcd_ltdc.LayerCfg[Lcd_Ctx[Instance].ActiveLayer].PixelFormat == LTDC_PIXEL_FORMAT_RGB888)
  {
    /* Write data value to RAM memory */
    *(__IO uint8_t*) (hlcd_ltdc.LayerCfg[Lcd_Ctx[Instance].ActiveLayer].FBStartAdress + ((3U*((Ypos*Lcd_Ctx[Instance].XSize) + Xpos))-3U)) = (uint8_t) (Color);
    *(__IO uint8_t*) (hlcd_ltdc.LayerCfg[Lcd_Ctx[Instance].ActiveLayer].FBStartAdress + ((3U*((Ypos*Lcd_Ctx[Instance].XSize) + Xpos))-2U)) = (uint8_t) (Color>>8U);
    *(__IO uint8_t*) (hlcd_ltdc.LayerCfg[Lcd_Ctx[Instance].ActiveLayer].FBStartAdress + ((3U*((Ypos*Lcd_Ctx[Instance].XSize) + Xpos))-1U)) = (uint8_t) (Color>>16U);
  }
  else
  {
    /* Write data value to RAM memory */
    *(__IO uint16_t*) (hlcd_ltdc.LayerCfg[Lcd_Ctx[Instance].ActiveLayer].FBStartAdress + (2U*((Ypos*Lcd_Ctx[Instance].XSize) + Xpos))) = (uint16_t)Color;
  }

  return BSP_ERROR_NONE;
}

/**
  * @}
  */

/** @defgroup STM32N6570_DK_LCD_Private_Functions LCD Private Functions
  * @{
  */
/**
  * @brief  Fills a buffer.
  * @param  Instance LCD Instance
  * @param  pDst Pointer to destination buffer
  * @param  xSize Buffer width
  * @param  ySize Buffer height
  * @param  OffLine Offset
  * @param  Color RGB color
  */
static void LL_FillBuffer(uint32_t Instance, uint32_t *pDst, uint32_t xSize, uint32_t ySize, uint32_t OffLine, uint32_t Color)
{
  uint32_t output_color_mode;
  uint32_t input_color = Color;

  switch(Lcd_Ctx[Instance].PixelFormat)
  {
  case LCD_PIXEL_FORMAT_RGB565:
    output_color_mode = DMA2D_OUTPUT_RGB565; /* RGB565 */
    input_color = CONVERTRGB5652ARGB8888(Color);
    break;
  case LCD_PIXEL_FORMAT_ARGB4444:
    output_color_mode = DMA2D_OUTPUT_ARGB4444; /* ARGB4444 */
    input_color = CONVERTARGB44442ARGB8888(Color);
    break;
  case LCD_PIXEL_FORMAT_RGB888:
    output_color_mode = DMA2D_OUTPUT_RGB888; /* RGB888 */
    input_color = CONVERTRGB8882ARGB8888(Color);
    break;
  default:
    output_color_mode = DMA2D_OUTPUT_ARGB8888; /* ARGB8888 */
    break;
  }

  /* Register to memory mode with ARGB8888 as color Mode */
  hlcd_dma2d.Init.Mode         = DMA2D_R2M;
  hlcd_dma2d.Init.ColorMode    = output_color_mode;
  hlcd_dma2d.Init.OutputOffset = OffLine;

  hlcd_dma2d.Instance = DMA2D;

  /* DMA2D Initialization */
  if(HAL_DMA2D_Init(&hlcd_dma2d) == HAL_OK)
  {
    if (HAL_DMA2D_Start(&hlcd_dma2d, input_color, (uint32_t)pDst, xSize, ySize) == HAL_OK)
    {
      /* Polling For DMA transfer */
      (void)HAL_DMA2D_PollForTransfer(&hlcd_dma2d, 50);
    }
  }
}

/**
  * @brief  Converts a line to an RGB pixel format.
  * @param  Instance LCD Instance
  * @param  pSrc Pointer to source buffer
  * @param  pDst Output color
  * @param  xSize Buffer width
  * @param  ColorMode Input color mode
  */
static void LL_ConvertLineToRGB(uint32_t Instance, uint32_t *pSrc, uint32_t *pDst, uint32_t xSize, uint32_t ColorMode)
{
  uint32_t output_color_mode;

  switch(Lcd_Ctx[Instance].PixelFormat)
  {
  case LCD_PIXEL_FORMAT_RGB565:
    output_color_mode = DMA2D_OUTPUT_RGB565; /* RGB565 */
    break;
  case LCD_PIXEL_FORMAT_ARGB4444:
    output_color_mode = DMA2D_OUTPUT_ARGB4444; /* ARGB4444 */
    break;
  case LCD_PIXEL_FORMAT_RGB888:
    output_color_mode = DMA2D_OUTPUT_RGB888; /* RGB888 */
    break;
  default:
    output_color_mode = DMA2D_OUTPUT_ARGB8888; /* ARGB8888 */
    break;
  }

  /* Configure the DMA2D Mode, Color Mode and output offset */
  hlcd_dma2d.Init.Mode         = DMA2D_M2M_PFC;
  hlcd_dma2d.Init.ColorMode    = output_color_mode;
  hlcd_dma2d.Init.OutputOffset = 0;

  /* Foreground Configuration */
  hlcd_dma2d.LayerCfg[1].AlphaMode = DMA2D_NO_MODIF_ALPHA;
  hlcd_dma2d.LayerCfg[1].InputAlpha = 0xFF;
  hlcd_dma2d.LayerCfg[1].InputColorMode = ColorMode;
  hlcd_dma2d.LayerCfg[1].InputOffset = 0;

  hlcd_dma2d.Instance = DMA2D;

  /* DMA2D Initialization */
  if(HAL_DMA2D_Init(&hlcd_dma2d) == HAL_OK)
  {
    if(HAL_DMA2D_ConfigLayer(&hlcd_dma2d, 1) == HAL_OK)
    {
      if (HAL_DMA2D_Start(&hlcd_dma2d, (uint32_t)pSrc, (uint32_t)pDst, xSize, 1) == HAL_OK)
      {
        /* Polling For DMA transfer */
        (void)HAL_DMA2D_PollForTransfer(&hlcd_dma2d, 50);
      }
    }
  }
}

/**
  * @brief  Initialize the BSP LTDC Msp.
  * @param  hltdc  LTDC handle
  * @retval None
  */
static void LTDC_MspInit(LTDC_HandleTypeDef *hltdc)
{
  GPIO_InitTypeDef  gpio_init_structure = {0};

  if (hltdc->Instance == LTDC)
  {
    __HAL_RCC_LTDC_CLK_ENABLE();

    __HAL_RCC_LTDC_FORCE_RESET();
    __HAL_RCC_LTDC_RELEASE_RESET();

    /* STM32N6570-DK MB1860A RK050HR18 */
    /* PG0  R0          */
    /* PD9  R1          */
    /* PD15 R2          */
    /* PB4  R3          */ // JTAG
    /* PH4  R4          */
    /* PA15 R5          */ // JTAG JTDI
    /* PG11 R6          */
    /* PD8  R7          */
    /* PG12 G0          */
    /* PG1  G1          */
    /* PA1  G2          */
    /* PA0  G3          */
    /* PB15 G4          */
    /* PB12 G5          */
    /* PB11 G6          */
    /* PG8  G7          */
    /* P15  B0          */
    /* PA7  B1          */
    /* PB2  B2          */
    /* PG6  B3          */
    /* PH3  B4          */
    /* PH6  B5          */
    /* PA8  B6          */
    /* PA2  B7          */
    /*                  */
    /* PG13 LCD_DE      */
    /* PQ3  LCD_ONOFF   */
    /* PB14 LCD_HSYNC   */
    /* PE11 PCD_VSYNC   */
    /* PB13 LCD_CLK     */
    /* PQ4  LCD_INT     */
    /* PQ6  LCD_BL_CTRL */
    /* PE1  NRST        */

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOQ_CLK_ENABLE();

    gpio_init_structure.Mode      = GPIO_MODE_AF_PP;
    gpio_init_structure.Pull      = GPIO_NOPULL;
    gpio_init_structure.Speed     = GPIO_SPEED_FREQ_HIGH;

    /* G3, G2, B7, B1, B6, R5 */
    gpio_init_structure.Pin       = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_15;
    gpio_init_structure.Alternate = GPIO_AF14_LCD;
    HAL_GPIO_Init(GPIOA, &gpio_init_structure);

    /* LCD_CLK, LCD_HSYNC B2, R3, G6, G5, G4 */
    gpio_init_structure.Pin       = GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_2 | GPIO_PIN_4 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_15;
    gpio_init_structure.Alternate = GPIO_AF14_LCD;
    HAL_GPIO_Init(GPIOB, &gpio_init_structure);

    /* R7, R1, R2 */
    gpio_init_structure.Pin       = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_15;
    gpio_init_structure.Alternate = GPIO_AF14_LCD;
    HAL_GPIO_Init(GPIOD, &gpio_init_structure);

    /* LCD_VSYNC */
    gpio_init_structure.Pin       = GPIO_PIN_11;
    gpio_init_structure.Alternate = GPIO_AF14_LCD;
    HAL_GPIO_Init(GPIOE, &gpio_init_structure);

    /* R0, G1, B3, G7, R6, G0 */
    gpio_init_structure.Pin       = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_6 | GPIO_PIN_8 | GPIO_PIN_11 | GPIO_PIN_12 ;
    gpio_init_structure.Alternate = GPIO_AF14_LCD;
    HAL_GPIO_Init(GPIOG, &gpio_init_structure);

    /* B4, R4, B5 */
    gpio_init_structure.Pin       = GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_6;
    gpio_init_structure.Alternate = GPIO_AF14_LCD;
    HAL_GPIO_Init(GPIOH, &gpio_init_structure);

    /* NRST */
    gpio_init_structure.Pin       = GPIO_PIN_1;
    gpio_init_structure.Mode      = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(GPIOE, &gpio_init_structure);

    /* LCD_ONOFF, LCD_BL_CTRL */
    gpio_init_structure.Pin       = GPIO_PIN_3 | GPIO_PIN_6;
    gpio_init_structure.Mode      = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(GPIOQ, &gpio_init_structure);

    /* LCD_DE */
    gpio_init_structure.Pin       = GPIO_PIN_13;
    gpio_init_structure.Mode      = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(GPIOG, &gpio_init_structure);

    HAL_GPIO_WritePin(GPIOQ, GPIO_PIN_3, GPIO_PIN_SET); /* LCD On */ /* PQ3  LCD_ONOFF   */
    HAL_GPIO_WritePin(GPIOG, GPIO_PIN_13, GPIO_PIN_SET); /* Display Enable */ /* PG13 LCD_DE      */
    HAL_GPIO_WritePin(GPIOQ, GPIO_PIN_6, GPIO_PIN_SET); /* 100% Brightness */ /* PQ6  LCD_BL_CTRL */

  }
}

/**
  * @brief  De-Initializes the BSP LTDC Msp
  * @param  hltdc  LTDC handle
  * @retval None
  */
static void LTDC_MspDeInit(LTDC_HandleTypeDef *hltdc)
{
  GPIO_InitTypeDef  gpio_init_structure;

  if(hltdc->Instance == LTDC)
  {
    /* LTDC Pins deactivation */
    /* GPIOA deactivation */
    gpio_init_structure.Pin       = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_8|GPIO_PIN_11 |GPIO_PIN_12| GPIO_PIN_15 | GPIO_PIN_6 | GPIO_PIN_9|GPIO_PIN_10;
    HAL_GPIO_DeInit(GPIOA, gpio_init_structure.Pin);

    /* GPIOB deactivation */
    gpio_init_structure.Pin       = GPIO_PIN_13 | GPIO_PIN_15 | GPIO_PIN_3 |GPIO_PIN_4 | GPIO_PIN_10| GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_14;
    HAL_GPIO_DeInit(GPIOB, gpio_init_structure.Pin);

    /* GPIOE deactivation */
    gpio_init_structure.Pin       = GPIO_PIN_11 | GPIO_PIN_15;
    HAL_GPIO_DeInit(GPIOE, gpio_init_structure.Pin);

    /* GPIOF deactivation */
    gpio_init_structure.Pin       = GPIO_PIN_0 | GPIO_PIN_10|GPIO_PIN_11 | GPIO_PIN_9 | GPIO_PIN_7 | GPIO_PIN_15;
    HAL_GPIO_DeInit(GPIOF, gpio_init_structure.Pin);

    /* GPIOG deactivation */
    gpio_init_structure.Pin       = GPIO_PIN_0 | GPIO_PIN_1| GPIO_PIN_2 | GPIO_PIN_13|GPIO_PIN_14;
    HAL_GPIO_DeInit(GPIOG, gpio_init_structure.Pin);

    /** Force and let in reset state LTDC */
    __HAL_RCC_LTDC_FORCE_RESET();

    __HAL_RCC_LTDC_RELEASE_RESET();

    /** Disable the LTDC */
    __HAL_RCC_LTDC_CLK_DISABLE();
  }
}

/**
  * @brief  Initialize the BSP DMA2D Msp.
  * @param  hdma2d  DMA2D handle
  * @retval None
  */
static void DMA2D_MspInit(DMA2D_HandleTypeDef *hdma2d)
{
  if(hdma2d->Instance == DMA2D)
  {
    /** Enable the DMA2D clock */
    __HAL_RCC_DMA2D_CLK_ENABLE();

    /** Toggle Sw reset of DMA2D IP */
    __HAL_RCC_DMA2D_FORCE_RESET();
    __HAL_RCC_DMA2D_RELEASE_RESET();
  }
}

/**
  * @brief  De-Initializes the BSP DMA2D Msp
  * @param  hdma2d  DMA2D handle
  * @retval None
  */
static void DMA2D_MspDeInit(DMA2D_HandleTypeDef *hdma2d)
{
  if(hdma2d->Instance == DMA2D)
  {
    /** Disable IRQ of DMA2D IP */
    HAL_NVIC_DisableIRQ(DMA2D_IRQn);

    /** Force and let in reset state DMA2D */
    __HAL_RCC_DMA2D_FORCE_RESET();
    __HAL_RCC_DMA2D_RELEASE_RESET();

    /** Disable the DMA2D */
    __HAL_RCC_DMA2D_CLK_DISABLE();
  }
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

