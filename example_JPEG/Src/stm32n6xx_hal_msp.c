/**
  ******************************************************************************
  * @file         stm32n6xx_hal_msp.c
  * @brief        This file provides code for the MSP Initialization
  *               and de-Initialization codes.
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
#include "main.h"
extern DMA_HandleTypeDef handle_HPDMA1_Channel1;

extern DMA_HandleTypeDef handle_HPDMA1_Channel0;

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* External functions --------------------------------------------------------*/
/**
  * Initializes the Global MSP.
  */
void HAL_MspInit(void)
{

  /* System interrupt init*/

  HAL_PWREx_EnableVddIO4();

}

/**
* @brief JPEG MSP Initialization
* This function configures the hardware resources used in this example
* @param hjpeg: JPEG handle pointer
* @retval None
*/
void HAL_JPEG_MspInit(JPEG_HandleTypeDef* hjpeg)
{
  DMA_IsolationConfigTypeDef IsolationConfiginput;
  if(hjpeg->Instance==JPEG)
  {

    /* Peripheral clock enable */
    __HAL_RCC_JPEG_CLK_ENABLE();

    /* JPEG DMA Init */
    /* HPDMA1_REQUEST_JPEG_TX Init */
    handle_HPDMA1_Channel1.Instance = HPDMA1_Channel1;
    handle_HPDMA1_Channel1.Init.Request = HPDMA1_REQUEST_JPEG_TX;
    handle_HPDMA1_Channel1.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    handle_HPDMA1_Channel1.Init.Direction = DMA_PERIPH_TO_MEMORY;
    handle_HPDMA1_Channel1.Init.SrcInc = DMA_SINC_FIXED;
    handle_HPDMA1_Channel1.Init.DestInc = DMA_DINC_INCREMENTED;
    handle_HPDMA1_Channel1.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_WORD;
    handle_HPDMA1_Channel1.Init.DestDataWidth = DMA_DEST_DATAWIDTH_WORD;
    handle_HPDMA1_Channel1.Init.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
    handle_HPDMA1_Channel1.Init.SrcBurstLength = 8;
    handle_HPDMA1_Channel1.Init.DestBurstLength = 8;
    handle_HPDMA1_Channel1.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT1|DMA_DEST_ALLOCATED_PORT0;
    handle_HPDMA1_Channel1.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    handle_HPDMA1_Channel1.Init.Mode = DMA_NORMAL;
    if (HAL_DMA_Init(&handle_HPDMA1_Channel1) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(hjpeg, hdmaout, handle_HPDMA1_Channel1);

    if (HAL_DMA_ConfigChannelAttributes(&handle_HPDMA1_Channel1, DMA_CHANNEL_PRIV|DMA_CHANNEL_SEC
                              |DMA_CHANNEL_SRC_SEC|DMA_CHANNEL_DEST_SEC) != HAL_OK)
    {
      Error_Handler();
    }

    IsolationConfiginput.CidFiltering = DMA_ISOLATION_ON;
    IsolationConfiginput.StaticCid = DMA_CHANNEL_STATIC_CID_1;
    if (HAL_DMA_SetIsolationAttributes(&handle_HPDMA1_Channel1, &IsolationConfiginput) != HAL_OK)
    {
      Error_Handler();
    }

    /* HPDMA1_REQUEST_JPEG_RX Init */
    handle_HPDMA1_Channel0.Instance = HPDMA1_Channel0;
    handle_HPDMA1_Channel0.Init.Request = HPDMA1_REQUEST_JPEG_RX;
    handle_HPDMA1_Channel0.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    handle_HPDMA1_Channel0.Init.Direction = DMA_MEMORY_TO_PERIPH;
    handle_HPDMA1_Channel0.Init.SrcInc = DMA_SINC_INCREMENTED;
    handle_HPDMA1_Channel0.Init.DestInc = DMA_DINC_FIXED;
    handle_HPDMA1_Channel0.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_WORD;
    handle_HPDMA1_Channel0.Init.DestDataWidth = DMA_DEST_DATAWIDTH_WORD;
    handle_HPDMA1_Channel0.Init.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
    handle_HPDMA1_Channel0.Init.SrcBurstLength = 8;
    handle_HPDMA1_Channel0.Init.DestBurstLength = 8;
    handle_HPDMA1_Channel0.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0|DMA_DEST_ALLOCATED_PORT1;
    handle_HPDMA1_Channel0.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    handle_HPDMA1_Channel0.Init.Mode = DMA_NORMAL;
    if (HAL_DMA_Init(&handle_HPDMA1_Channel0) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(hjpeg, hdmain, handle_HPDMA1_Channel0);

    if (HAL_DMA_ConfigChannelAttributes(&handle_HPDMA1_Channel0, DMA_CHANNEL_PRIV|DMA_CHANNEL_SEC
                              |DMA_CHANNEL_SRC_SEC|DMA_CHANNEL_DEST_SEC) != HAL_OK)
    {
      Error_Handler();
    }

    IsolationConfiginput.CidFiltering = DMA_ISOLATION_ON;
    IsolationConfiginput.StaticCid = DMA_CHANNEL_STATIC_CID_1;
    if (HAL_DMA_SetIsolationAttributes(&handle_HPDMA1_Channel0, &IsolationConfiginput) != HAL_OK)
    {
      Error_Handler();
    }

    /* JPEG interrupt Init */
    HAL_NVIC_SetPriority(JPEG_IRQn, 8, 0);
    HAL_NVIC_EnableIRQ(JPEG_IRQn);

  }

}

/**
* @brief JPEG MSP De-Initialization
* This function freeze the hardware resources used in this example
* @param hjpeg: JPEG handle pointer
* @retval None
*/
void HAL_JPEG_MspDeInit(JPEG_HandleTypeDef* hjpeg)
{
  if(hjpeg->Instance==JPEG)
  {
    /* Peripheral clock disable */
    __HAL_RCC_JPEG_CLK_DISABLE();

    /* JPEG DMA DeInit */
    HAL_DMA_DeInit(hjpeg->hdmaout);
    HAL_DMA_DeInit(hjpeg->hdmain);

    /* JPEG interrupt DeInit */
    HAL_NVIC_DisableIRQ(JPEG_IRQn);
  }

}

