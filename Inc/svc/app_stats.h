/**
 ******************************************************************************
 * @file    app_stats.h
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

#ifndef APP_STATS_H
#define APP_STATS_H

#include <stdint.h>

#include "FreeRTOS.h"

#define CPU_LOAD_HISTORY_DEPTH 8

typedef struct {
  int last;
  int total;
  uint64_t acc;
  float mean;
} time_stat_t;

typedef struct {
  time_stat_t nn_total_time;
  time_stat_t nn_inference_time;
  time_stat_t disp_total_time;
  time_stat_t nn_pp_time;
  time_stat_t disp_display_time;
  time_stat_t disp_enc_time;
  int nb_detect;
} stat_info_t;

typedef struct {
  uint64_t current_total;
  uint64_t current_thread_total;
  uint64_t prev_total;
  uint64_t prev_thread_total;
  struct {
    uint64_t total;
    uint64_t thread;
    uint32_t tick;
  } history[CPU_LOAD_HISTORY_DEPTH];
} cpuload_info_t;

void app_stats_init(void);
stat_info_t *app_stats_state(void);
void time_stat_update(time_stat_t *p_stat, int value);
void stats_detect_update(int nb_detect);
void stat_info_copy(stat_info_t *copy);
void app_stats_cpuload_update(void);
void app_stats_cpuload_get(float *cpu_load_last, float *cpu_load_last_second,
                           float *cpu_load_last_five_seconds);

#endif
