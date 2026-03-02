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

#include <assert.h>
#include <errno.h>

extern UART_HandleTypeDef huart1;

int remove(const char *pathname)
{
  assert(0);

  return -1;
}

int __close(int fd)
{
  assert(0);

  return -1;
}

long __lseek(int fd, long offet, int whence)
{
  assert(0);

  return -1;
}

size_t __write(int file, const unsigned char *ptr, size_t len)
{
  HAL_StatusTypeDef status;

  status = HAL_UART_Transmit(&huart1, (uint8_t*)ptr, len, ~0);

  return (status == HAL_OK ? len : 0);
}

