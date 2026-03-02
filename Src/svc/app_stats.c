/**
 ******************************************************************************
 * @file    app_stats.c
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

#include "svc/app_stats.h"

#include <assert.h>
#include <string.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "stm32n6xx_hal.h"

static SemaphoreHandle_t stat_info_lock;
static StaticSemaphore_t stat_info_lock_buffer;
static stat_info_t stat_info;
static cpuload_info_t cpu_load;

static void cpuload_init(cpuload_info_t *cpu_load_init)
{
  memset(cpu_load_init, 0, sizeof(cpuload_info_t));
}

static void cpuload_update(cpuload_info_t *cpu_load_update)
{
  int i;

  cpu_load_update->history[1] = cpu_load_update->history[0];
  cpu_load_update->history[0].total = portGET_RUN_TIME_COUNTER_VALUE();
  cpu_load_update->history[0].thread = cpu_load_update->history[0].total - ulTaskGetIdleRunTimeCounter();
  cpu_load_update->history[0].tick = HAL_GetTick();

  if (cpu_load_update->history[1].tick - cpu_load_update->history[2].tick < 1000)
    return;

  for (i = 0; i < CPU_LOAD_HISTORY_DEPTH - 2; i++)
    cpu_load_update->history[CPU_LOAD_HISTORY_DEPTH - 1 - i] =
      cpu_load_update->history[CPU_LOAD_HISTORY_DEPTH - 1 - i - 1];
}

static void cpuload_get_info(cpuload_info_t *cpu_load_info, float *cpu_load_last, float *cpu_load_last_second,
                             float *cpu_load_last_five_seconds)
{
  if (cpu_load_last)
    *cpu_load_last = 100.0 * (cpu_load_info->history[0].thread - cpu_load_info->history[1].thread) /
                     (cpu_load_info->history[0].total - cpu_load_info->history[1].total);
  if (cpu_load_last_second)
    *cpu_load_last_second = 100.0 * (cpu_load_info->history[2].thread - cpu_load_info->history[3].thread) /
                     (cpu_load_info->history[2].total - cpu_load_info->history[3].total);
  if (cpu_load_last_five_seconds)
    *cpu_load_last_five_seconds = 100.0 * (cpu_load_info->history[2].thread - cpu_load_info->history[7].thread) /
                     (cpu_load_info->history[2].total - cpu_load_info->history[7].total);
}

void app_stats_init(void)
{
  stat_info_lock = xSemaphoreCreateMutexStatic(&stat_info_lock_buffer);
  assert(stat_info_lock);

  memset(&stat_info, 0, sizeof(stat_info));
  cpuload_init(&cpu_load);
}

stat_info_t *app_stats_state(void)
{
  return &stat_info;
}

void time_stat_update(time_stat_t *p_stat, int value)
{
  int ret;

  ret = xSemaphoreTake(stat_info_lock, portMAX_DELAY);
  assert(ret == pdTRUE);

  p_stat->last = value;
  p_stat->acc += value;
  p_stat->total++;
  p_stat->mean = (float)p_stat->acc / p_stat->total;

  ret = xSemaphoreGive(stat_info_lock);
  assert(ret == pdTRUE);
}

void stat_info_copy(stat_info_t *copy)
{
  int ret;

  ret = xSemaphoreTake(stat_info_lock, portMAX_DELAY);
  assert(ret == pdTRUE);

  *copy = stat_info;

  ret = xSemaphoreGive(stat_info_lock);
  assert(ret == pdTRUE);
}

void app_stats_cpuload_update(void)
{
  cpuload_update(&cpu_load);
}

void app_stats_cpuload_get(float *cpu_load_last, float *cpu_load_last_second, float *cpu_load_last_five_seconds)
{
  cpuload_get_info(&cpu_load, cpu_load_last, cpu_load_last_second, cpu_load_last_five_seconds);
}

