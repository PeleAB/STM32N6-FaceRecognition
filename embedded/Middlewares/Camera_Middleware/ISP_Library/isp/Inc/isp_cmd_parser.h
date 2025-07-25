/**
  ******************************************************************************
  * @file    isp_cmd_parser.h
  * @author  AIS Application Team
  * @brief   Header file for isp_cmd_parser.c
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
#ifndef __ISP_CMD_PARSER_H
#define __ISP_CMD_PARSER_H

/* Includes ------------------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
ISP_StatusTypeDef ISP_CmdParser_ProcessCommand(ISP_HandleTypeDef *hIsp, uint8_t *cmd);

#endif
