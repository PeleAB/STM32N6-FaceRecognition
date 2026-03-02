/**
 ******************************************************************************
 * @file    uvcl_stm32_usbd.h
 * @author  MDG Application Team
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

#ifndef _UVCL_STM32_USBD_H_
#define _UVCL_STM32_USBD_H_

#include "uvcl.h"
#include "uvcl_internal.h"

int UVCL_stm32_usbd_init(UVCL_Ctx_t *p_ctx, PCD_HandleTypeDef *pcd_handle, PCD_TypeDef *pcd_instance, UVCL_Conf_t *conf);
#if defined(UVCL_USBD_USE_THREADX) || defined(UVCL_USBD_USE_FREERTOS)
void UVCL_stm32_usbd_IRQHandler();
#endif

#endif

