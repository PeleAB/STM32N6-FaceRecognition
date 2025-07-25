/**
  ******************************************************************************
  * @file    ai_memory_manager.h
  * @author  AST Embedded Analytics Research Platform
  * @brief   AI Library Memory Management Wrappers
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2018 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  
  ******************************************************************************
  */

#ifndef AI_MEMORY_MANAGER_H
#define AI_MEMORY_MANAGER_H
#pragma once

#include <string.h>  /* memcpy */
#include <stdlib.h>

#include "core_mem_manager.h"

/*!
 * @section MemoryManager
 * @ingroup ai_memory_manager
 * Macros to handle memory allocation and management as generic wrappers.
 * Dynamic allocations, freeing, clearing and copy are provided.
 * @{
 */

#define AI_MEM_ALLOC(size_, type_) \
          ((type_*)CORE_MEM_ALLOC((size_)*sizeof(type_)))

#define AI_MEM_FREE(ptr_) \
          { CORE_MEM_FREE((ai_handle)(ptr_)); }

#define AI_MEM_CLEAR(ptr_, size_) \
          AI_MEM_FILL(ptr_, 0, size_)

// #ifdef HAS_REISC
#if 0
#define AI_MEM_FILL(ptr_, value_, size_) \
          { memset((void*)(ptr_), (int)(value_), (size_t)(size_)); }

#define AI_MEM_COPY(dst_, src_, size_) \
          { memcpy((void*)(dst_), (const void*)(src_), (size_)); }

#define AI_MEM_MOVE(dst_, src_, size_) \
          { memmove((void*)(dst_), (const void*)(src_), (size_)); }
#else

#include "st_int8_nnsupportfunctions.h"

#define AI_MEM_FILL(ptr_, value_, size_) \
          { st_int8_fill((int8_t)(value_), (int8_t*)(ptr_), (uint32_t)(size_)); }

#define AI_MEM_COPY(dst_, src_, size_) \
          { st_int8_copy((int8_t*)(src_), (int8_t*)(dst_), (uint32_t)(size_)); }

#define AI_MEM_MOVE(dst_, src_, size_) \
          { st_int8_copy((int8_t*)(src_), (int8_t*)(dst_), (uint32_t)(size_)); }

#endif

/*! @} */

#endif    /*AI_MEMORY_MANAGER_H*/
