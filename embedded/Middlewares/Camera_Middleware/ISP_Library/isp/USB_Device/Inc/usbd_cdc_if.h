/**
  ******************************************************************************
  * @file    usbd_cdc_if.h
  * @author  MCD Application Team
  * @brief   Header for usbd_cdc_if.c file.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2020-2021 STMicroelectronics.
  * All rights reserved.
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USBD_CDC_IF_H
#define __USBD_CDC_IF_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "usbd_cdc.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/

/* Define size for the receive and transmit buffer over CDC */
/* It's up to user to redefine and/or remove those define */
#define APP_RX_DATA_SIZE  2048
#define APP_TX_DATA_SIZE  2048

/** CDC Interface callback. */
extern USBD_CDC_ItfTypeDef USBD_CDC_Interface_fops;

/* Exported function prototypes -----------------------------------------------*/
uint8_t CDC_Transmit(uint8_t* Buf, uint32_t Len);
uint32_t USB_CDC_Send_Wrapper_Function(uint8_t* buffer, uint32_t size);


#ifdef __cplusplus
}
#endif

#endif /* __USBD_CDC_IF_H */
