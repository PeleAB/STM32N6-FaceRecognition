/**
  ******************************************************************************
  * @file    core_benchmark.h
  * @author  AST Embedded Analytics Research Platform
  * @brief   header file of common benchmark timers & routines
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  
  ******************************************************************************
  */

#ifndef CORE_BENCHMARK_H_
#define CORE_BENCHMARK_H_

#include "ai_platform.h"
#include "ai_datatypes_defines.h"

#define CORE_TIMER_DECLARE(timer_) \
  core_timer timer_;

#define CORE_TIMER_INIT(timer_, description_) \
  { core_timer_init(timer_, description_); }

#define CORE_TIMER_RESET(timer_) \
  { core_timer_reset(timer_); }

#define CORE_TIMER_START(timer_) \
  { core_timer_start(timer_); }

#define CORE_TIMER_STOP(timer_) \
  { core_timer_stop(timer_); }

#define CORE_TIMER_NAME(timer_) \
  ( (timer_)->name )

#define CORE_TIMER_ELAPSED_S(timer_) \
  ( (ai_float)(((timer_)->elapsed) * 0.000001f) )

#define CORE_TIMER_ELAPSED_MS(timer_) \
  ( (ai_float)(((timer_)->elapsed) * 0.001f) )

#define CORE_TIMER_ELAPSED_US(timer_) \
  (((timer_)->elapsed))

typedef ai_u32      core_timer_ts;

typedef struct core_timer_s {
  const char*       name;
  core_timer_ts     start;
  core_timer_ts     elapsed;
} core_timer;

AI_API_DECLARE_BEGIN

AI_INTERNAL_API
ai_bool core_timer_init(core_timer* t, const char* name);

AI_INTERNAL_API
ai_bool core_timer_reset(core_timer* t);

AI_INTERNAL_API
ai_bool core_timer_start(core_timer* t);

AI_INTERNAL_API
core_timer_ts core_timer_stop(core_timer* t);

#if defined HAS_STM32
void core_timer_init_(void);
core_timer_ts core_timer_ticks_start_(void);
core_timer_ts core_timer_ticks_(void);
#endif

AI_API_DECLARE_END

#endif    /*CORE_BENCHMARK_H_*/
