
/**
  ******************************************************************************
  * @file    core_signals.h
  * @author  AMS OEM solutions
  * @brief   header file of common signal definitions
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

#ifndef CORE_SIGNALS_H_
#define CORE_SIGNALS_H_

#if META_UTILS
#include "meta_utils.h"
#endif

#if REISCL
#define TIME_START_TAG(TIME_TAG) __attribute__((noipa,optimize("O0"))) static void TIME_TAG ## _start() { __asm__("nop");};
#define TIME_STOP_TAG(TIME_TAG) __attribute__((noipa,optimize("O0"))) static void TIME_TAG ## _stop() { __asm__("nop"); __asm__("nop");};
#else
#define TIME_START_TAG(TIME_TAG) static void TIME_TAG ## _start() {};
#define TIME_STOP_TAG(TIME_TAG) static void TIME_TAG ## _stop() {};
#endif

#define MAKE_TIME_TAG(TIME_TAG)\
TIME_START_TAG(TIME_TAG)\
TIME_STOP_TAG(TIME_TAG)

#define ORDERED_TIME_TAG(N, TIME_TAG) MAKE_TIME_TAG(TIME_TAG ## _ ## N)

#if META_UTILS
#define MAKE_TIME_TAGS(TAG_NAME, N) META_MAP(ORDERED_TIME_TAG, N, TAG_NAME)
#endif

#endif    /*CORE_SIGNALS_H_*/
