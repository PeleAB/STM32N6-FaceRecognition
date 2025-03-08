/**
  ******************************************************************************
  * @file    isp_tool_com.h
  * @author  AIS Application Team
  * @brief   Header file for isp_tool_com.c
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __ISP_TOOL_COM_H
#define __ISP_TOOL_COM_H
/* Includes ------------------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void ISP_ToolCom_Init(void);
void ISP_ToolCom_ReceivedCb(uint8_t *buffer, uint32_t buffer_size);
void ISP_ToolCom_SendData(uint8_t *buffer, uint32_t buffer_size, char *dump_start_msg,  char *dump_stop_msg);
uint32_t ISP_ToolCom_CheckCommandReceived(uint8_t **block_cmd);
void ISP_ToolCom_PrepareNextCommand();

#endif
