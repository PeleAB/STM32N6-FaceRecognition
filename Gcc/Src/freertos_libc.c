 /**
 ******************************************************************************
 * @file    freertos_libc.c
 * @author  GPM Application Team
 *
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

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#include "cmsis_compiler.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#define IS_IRQ_MODE()     (__get_IPSR() != 0U)

static int is_tx_rt_init_done;
static SemaphoreHandle_t libc_lock;
static StaticSemaphore_t libc_lock_buffer;

void freertos_libc_init()
{
  libc_lock = xSemaphoreCreateMutexStatic(&libc_lock_buffer);
  assert(libc_lock);

  is_tx_rt_init_done = 1;
}

void __malloc_lock (struct _reent *reent)
{
  int ret;

  if (!is_tx_rt_init_done)
    return ;

  assert(is_tx_rt_init_done);
  assert(!IS_IRQ_MODE());

  ret = xSemaphoreTake(libc_lock, portMAX_DELAY);
  assert(ret == pdTRUE);
}

void __malloc_unlock (struct _reent *reent)
{
  int ret;

  if (!is_tx_rt_init_done)
    return ;

  assert(is_tx_rt_init_done);
  assert(!IS_IRQ_MODE());

  ret = xSemaphoreGive(libc_lock);
  assert(ret == pdTRUE);
}

