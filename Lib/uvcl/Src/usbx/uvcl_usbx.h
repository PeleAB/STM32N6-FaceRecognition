/**
 ******************************************************************************
 * @file    uvcl_usbx.h
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

#ifndef _UVCL_USBX_H_
#define _UVCL_USBX_H_

#include "uvcl.h"
#include "uvcl_internal.h"

int UVCL_usbx_init(UVCL_Ctx_t *p_ctx, PCD_HandleTypeDef *pcd_handle, PCD_TypeDef *pcd_instance, UVCL_Conf_t *conf);
void UVCL_stm32_usbx_IRQHandler(void);

#endif

