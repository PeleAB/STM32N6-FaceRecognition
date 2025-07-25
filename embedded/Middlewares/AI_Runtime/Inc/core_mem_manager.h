/**
  ******************************************************************************
  * @file    core_mem_manager.h
  * @author  AST Embedded Analytics Research Platform
  * @brief   Generic implementation of library memory handler routines
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
#ifndef CORE_MEM_MANAGER_H_
#define CORE_MEM_MANAGER_H_

#include "ai_platform.h"
#include "ai_platform_interface.h"
#include "core_common.h"

#define CORE_MEM_ALLOC(size) \
  core_mem_alloc(size)
#define CORE_MEM_FREE(ptr) \
  core_mem_free((ai_handle)(ptr))
#define CORE_MEM_REALLOC(ptr, size) \
  core_mem_realloc((ai_handle)(ptr),(size))


AI_API_DECLARE_BEGIN

#ifdef HAS_CORE_MEM_BENCHMARK

#define CORE_MEM_GET_STAT(field) \
  core_mem_get_stat()->field

typedef struct core_mem_stat_s {
  ai_u32       id;
  ai_i32       curr;
  ai_i32       min;
  ai_i32       max;
  ai_i32       smallest;
  ai_i32       biggest;
  ai_i32       realloc_count;
  ai_i32       alloc_count;
  ai_i32       free_count;
  //ai_i32       bytes_static;
} core_mem_stat;

core_mem_stat* core_mem_get_stat(void);

#endif    /*HAS_CORE_MEM_BENCHMARK*/


AI_INTERNAL_API
ai_ptr         core_mem_alloc(const ai_size size);

AI_INTERNAL_API
ai_ptr         core_mem_free(ai_ptr ptr);

AI_INTERNAL_API
ai_ptr         core_mem_realloc(ai_ptr ptr, const ai_size size);

#ifdef HAS_AI_BUILTIN_UNIT_TESTS
AI_INTERNAL_API
int            ai_builtin_unit_tests_core_mem(int argc, char* argv[]);
#endif  /*HAS_AI_BUILTIN_UNIT_TESTS*/

AI_API_DECLARE_END

#endif  /*CORE_MEM_MANAGER_H_*/
