/**
 ******************************************************************************
 * @file    stm32n6xx_it.c
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

/* Includes ------------------------------------------------------------------*/
#include "bsp/stm32n6xx_it.h"
#include "stm32n6xx_hal.h"

#include "cmw_camera.h"
#include "uvcl.h"

extern DMA_HandleTypeDef hdma_usart1_rx;

/**
 * @brief   This function handles NMI exception.
 * @param  None
 * @retval None
 */
void NMI_Handler(void) {}

/**
 * @brief  This function handles Memory Manage exception.
 * @param  None
 * @retval None
 */
void MemManage_Handler(void) {
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1) {
  }
}

/**
 * @brief  This function handles Bus Fault exception.
 * @param  None
 * @retval None
 */
void BusFault_Handler(void) {
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1) {
  }
}

/**
 * @brief  This function handles Secure Fault exception.
 * @param  None
 * @retval None
 */
void SecureFault_Handler(void) {
  /* Go to infinite loop when Secure Fault exception occurs */
  while (1) {
  }
}

/**
 * @brief  This function handles Debug Monitor exception.
 * @param  None
 * @retval None
 */
void DebugMon_Handler(void) {
  while (1) {
  }
}

/******************************************************************************/
/*                 STM32N6xx Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32n6xx.s).                                               */
/******************************************************************************/

void CSI_IRQHandler(void) {
  HAL_DCMIPP_CSI_IRQHandler(CMW_CAMERA_GetDCMIPPHandle());
}

void DCMIPP_IRQHandler(void) {
  HAL_DCMIPP_IRQHandler(CMW_CAMERA_GetDCMIPPHandle());
}

void GPDMA1_Channel0_IRQHandler(void) { HAL_DMA_IRQHandler(&hdma_usart1_rx); }

extern UART_HandleTypeDef huart1;
void USART1_IRQHandler(void) { HAL_UART_IRQHandler(&huart1); }

void USB1_OTG_HS_IRQHandler(void) { UVCL_IRQHandler(); }
