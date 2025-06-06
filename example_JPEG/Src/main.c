/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "jpeg_utils_conf.h"
#include "image_320_240_rgb.h"
#include "jpeg_utils.h"
#include "encode_dma.h"
#include "image_320_240_rgb.h"
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

JPEG_HandleTypeDef hjpeg;
DMA_HandleTypeDef handle_HPDMA1_Channel1;
DMA_HandleTypeDef handle_HPDMA1_Channel0;

uint32_t jpgBuffer[320*240] = {0};
extern __IO uint32_t Jpeg_HWEncodingEnd;
uint32_t RGB_ImageAddress = {0};
uint32_t tickstart;

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_HPDMA1_Init(void);
static void MX_JPEG_Init(void);

/* Private user code ---------------------------------------------------------*/


/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  uint32_t jpeg_encode_processing_end = 0;

#if (JPEG_RGB_FORMAT == JPEG_ARGB8888)
  RGB_ImageAddress = (uint32_t)Image_ARGB8888;

#elif(JPEG_RGB_FORMAT == JPEG_RGB888)
  RGB_ImageAddress = (uint32_t)Image_RGB888;

#elif(JPEG_RGB_FORMAT == JPEG_RGB565)
  RGB_ImageAddress = (uint32_t)Image_RGB565;

#endif /* JPEG_RGB_FORMAT */

  /* MCU Configuration--------------------------------------------------------*/
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* Initialize LD1 and LD2*/
  BSP_LED_Init(LED_GREEN);
  BSP_LED_Init(LED_RED);


  /* Initialize all configured peripherals */

  MX_HPDMA1_Init();
  MX_JPEG_Init();

  /* Init The JPEG Color Look Up Tables used for YCbCr to RGB conversion   */
  JPEG_InitColorTables();

  /*JPEG Encoding with DMA (Not Blocking ) Method */
  JPEG_Encode_DMA(&hjpeg, RGB_ImageAddress, RGB_IMAGE_SIZE, jpgBuffer);

  /* Wait till end of JPEG encoding and perform Input/Output Processing in BackGround  */
  tickstart = HAL_GetTick();
  do
  {
    JPEG_EncodeInputHandler(&hjpeg);
    jpeg_encode_processing_end = JPEG_EncodeOutputHandler(&hjpeg);
  }while((jpeg_encode_processing_end == 0) && ((HAL_GetTick() - tickstart) < 5000));

  if (Jpeg_HWEncodingEnd != 0)
  {
    /* test ends properly */
    BSP_LED_On(LED_GREEN);
  }
  else
  {
    /* test error */
    BSP_LED_On(LED_RED);
  }


  /* Infinite loop */
  while (1)
  {

  }

}


/**
  * @brief HPDMA1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_HPDMA1_Init(void)
{

  /* Peripheral clock enable */
  __HAL_RCC_HPDMA1_CLK_ENABLE();

  /* HPDMA1 interrupt Init */
    HAL_NVIC_SetPriority(HPDMA1_Channel0_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(HPDMA1_Channel0_IRQn);
    HAL_NVIC_SetPriority(HPDMA1_Channel1_IRQn, 7, 0);
    HAL_NVIC_EnableIRQ(HPDMA1_Channel1_IRQn);

    HAL_RIF_RISC_SetSlaveSecureAttributes(RIF_RCC_PERIPH_INDEX_HPDMA1, RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV);
}

/**
  * @brief JPEG Initialization Function
  * @param None
  * @retval None
  */
static void MX_JPEG_Init(void)
{

  HAL_RIF_RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_JPEG, RIF_ATTRIBUTE_PRIV | RIF_ATTRIBUTE_SEC);
  hjpeg.Instance = JPEG;
  if (HAL_JPEG_Init(&hjpeg) != HAL_OK)
  {
    Error_Handler();
  }
}

void RGB_GetInfo(JPEG_ConfTypeDef *pInfo)
{
  /* Read Images Sizes */
  pInfo->ImageWidth         = RGB_IMAGE_WIDTH;
  pInfo->ImageHeight        = RGB_IMAGE_HEIGHT;

  /* Jpeg Encoding Setting to be set by users */
  pInfo->ChromaSubsampling  = JPEG_CHROMA_SAMPLING;
  pInfo->ColorSpace         = JPEG_COLOR_SPACE;
  pInfo->ImageQuality       = JPEG_IMAGE_QUALITY;

  /*Check if Image Sizes meets the requirements */
  if (((pInfo->ImageWidth % 8) != 0 ) || ((pInfo->ImageHeight % 8) != 0 ) || \
      (((pInfo->ImageWidth % 16) != 0 ) && (pInfo->ColorSpace == JPEG_YCBCR_COLORSPACE) && (pInfo->ChromaSubsampling != JPEG_444_SUBSAMPLING)) || \
      (((pInfo->ImageHeight % 16) != 0 ) && (pInfo->ColorSpace == JPEG_YCBCR_COLORSPACE) && (pInfo->ChromaSubsampling == JPEG_420_SUBSAMPLING)))
  {
    Error_Handler();
  }
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
    BSP_LED_On(LED_RED);
  }
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* Infinite loop */
  while (1)
  {
  }
}
#endif /* USE_FULL_ASSERT */
