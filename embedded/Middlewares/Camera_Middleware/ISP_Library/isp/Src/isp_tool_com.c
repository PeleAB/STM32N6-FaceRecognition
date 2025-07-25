/**
  ******************************************************************************
  * @file    isp_tool_com.c
  * @author  AIS Application Team
  * @brief   Implement the protocol to communicate with the IQ tuning tool
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
#ifdef ISP_MW_TUNING_TOOL_SUPPORT

/* Includes ------------------------------------------------------------------*/
#include "isp_core.h"
#include "isp_tool_com.h"
#include "usbd_cdc_if.h"
#include "usb_device.h"

/* Private types -------------------------------------------------------------*/
typedef struct {
  uint8_t payload[1000];
  uint32_t payload_size;
}
ISP_ToolCom_packet_t;
/* Private constants ---------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
ISP_ToolCom_packet_t received_packet;

/* Private functions ---------------------------------------------------------*/
/**
  * @brief  ISP_ToolCom_Init
  *         Initialize the communication link with the remote tool
  * @param  None
  * @retval None
  */
void ISP_ToolCom_Init(void)
{
  MX_USB_DEVICE_Init();

  /* Clear packet */
  received_packet.payload_size = 0;
}

/**
  * @brief  ISP_ToolCom_ReceivedCb
  *         Callback received when data are received
  * @param  buffer: Pointer to buffer payload
  * @param  buffer_size: Size of buffer payload
  * @retval None
  */
void ISP_ToolCom_ReceivedCb(uint8_t *buffer, uint32_t buffer_size)
{
  received_packet.payload_size = buffer_size;
  memcpy(received_packet.payload, buffer, buffer_size);
}

/**
  * @brief  ISP_ToolCom_SendData
  *         Send a packet (message or/and payload)
  * @param  buffer: Pointer to buffer payload
  * @param  buffer_size: Size of buffer payload
  * @param  message: Pointer to message string
  * @retval None
  */
void ISP_ToolCom_SendData(uint8_t *buffer, uint32_t buffer_size, char *dump_start_msg,  char *dump_stop_msg)
{
  if (dump_start_msg)
  {
	USB_CDC_Send_Wrapper_Function((uint8_t*) dump_start_msg, strlen((char*)dump_start_msg));
  }
  if (buffer)
  {
	USB_CDC_Send_Wrapper_Function(buffer, buffer_size);
  }
  if (dump_stop_msg)
  {
	USB_CDC_Send_Wrapper_Function((uint8_t*) dump_stop_msg, strlen((char*)dump_stop_msg));
  }
}

/**
  * @brief  ISP_ToolCom_CheckCommandReceived
  *         Check if a packet payload was received
  * @param  block_cmd: Pointer to the received packet payload
  * @retval Size of the received packet payload, or 0 if nothing was received
  */
uint32_t ISP_ToolCom_CheckCommandReceived(uint8_t **block_cmd)
{
  uint32_t payload_size = 0;

  if (received_packet.payload_size > 0)
  {
    *block_cmd = (uint8_t*)received_packet.payload;
    payload_size = received_packet.payload_size;
  }

  return payload_size;
}

/**
  * @brief  ISP_ToolCom_PrepareNextCommand
  *         Get prepared to receive a new packet, which more or less consists in releasing
  *         the last received buffer.
  * @param  none
  * @retval none
  */
void ISP_ToolCom_PrepareNextCommand()
{
  /* Received message has been handled so clear it */
  received_packet.payload_size = 0;
}

#endif /* ISP_MW_TUNING_TOOL_SUPPORT */
