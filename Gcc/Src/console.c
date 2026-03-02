 /**
 ******************************************************************************
 * @file    console.c
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

#include "main.h"

#include <errno.h>
#include <unistd.h>

extern UART_HandleTypeDef huart1;

int _write(int file, char *ptr, int len)
{
  HAL_StatusTypeDef status;

  if ((file != STDOUT_FILENO) && (file != STDERR_FILENO)) {
      errno = EBADF;
      return -1;
  }

  status = HAL_UART_Transmit(&huart1, (uint8_t*)ptr, len, ~0);

  return (status == HAL_OK ? len : 0);
}

