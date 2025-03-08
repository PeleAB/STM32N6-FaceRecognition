/**
  ******************************************************************************
  * @file    core_ring_buffer.h
  * @author  AST Embedded Analytics Research Platform
  * @brief   Generic implementation of circular buffers datastructs
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
#ifndef CORE_RING_BUFFER_H
#define CORE_RING_BUFFER_H


#include "ai_platform.h"
#include "ai_platform_interface.h"

#define CORE_RING_BUFFER_MAX_CAPACITY \
  ((core_ring_buffer_reg)(0x1<<((8*sizeof(core_ring_buffer_reg))-1)))

#define CORE_RING_BUFFER_OBJ_INIT(_capacity, _data_ptr) { \
  .data = AI_PTR(_data_ptr), \
  .stride = sizeof((_data_ptr)[0]), \
  .capacity = (_capacity), \
  .read_index = 0x0, \
  .write_index = 0x0 \
}

#define CORE_RING_BUFFER_OBJ_DECLARE(_name, _capacity, _data_ptr) \
  core_ring_buffer _name = CORE_RING_BUFFER_OBJ_INIT( \
    _capacity, _data_ptr);

#define CORE_RING_BUFFER_ITEM_GET_FRONT(_buffer, _elem_pos, _type) \
  ( (_type*)core_ring_buffer_get((_buffer), (_elem_pos)) )

#define CORE_RING_BUFFER_ITEM_GET_BACK(_buffer, _elem_pos, _type) \
  ( (_type*)core_ring_buffer_get((_buffer), \
                                 core_ring_buffer_size(_buffer)-1-(_elem_pos)) )

/*** Ring Buffer Typedefs ******************************************************/
typedef ai_u16   core_ring_buffer_reg;

typedef struct core_ring_buffer_s {
  ai_ptr                   data;
  ai_u16                   stride;
  core_ring_buffer_reg     capacity;
  core_ring_buffer_reg     read_index;
  core_ring_buffer_reg     write_index;
} core_ring_buffer;


/*** Public APIs Declarations *************************************************/

AI_API_DECLARE_BEGIN

AI_INTERNAL_API
ai_u32 core_ring_buffer_init(
    core_ring_buffer* b, const core_ring_buffer_reg capacity,
    const ai_u16 stride, const ai_ptr data);

AI_INTERNAL_API
ai_bool core_ring_buffer_destroy(
  core_ring_buffer* b);

AI_INTERNAL_API
void core_ring_buffer_clear(
  core_ring_buffer* b);

AI_INTERNAL_API
ai_u16 core_ring_buffer_stride(
  const core_ring_buffer* b);

AI_INTERNAL_API
ai_u32 core_ring_buffer_capacity(
  const core_ring_buffer* b);

AI_INTERNAL_API
ai_u32 core_ring_buffer_size(
  const core_ring_buffer* b);

AI_INTERNAL_API
ai_u32 core_ring_buffer_free_size(
  const core_ring_buffer* b);

AI_INTERNAL_API
ai_bool core_ring_buffer_empty(
  const core_ring_buffer* b);

AI_INTERNAL_API
ai_bool core_ring_buffer_full(
  const core_ring_buffer* b);

AI_INTERNAL_API
ai_bool core_ring_buffer_push_force(
  core_ring_buffer* b, const ai_handle elem);

AI_INTERNAL_API
ai_bool  core_ring_buffer_push(
  core_ring_buffer* b, const ai_handle elem);

AI_INTERNAL_API
ai_bool core_ring_buffer_pop(
  core_ring_buffer* b, ai_handle elem);

AI_INTERNAL_API
ai_handle core_ring_buffer_get_base_address(
  core_ring_buffer* b);

AI_INTERNAL_API
ai_handle core_ring_buffer_get_end_address(
  core_ring_buffer* b);

AI_INTERNAL_API
ai_handle core_ring_buffer_get(
  core_ring_buffer* b, const ai_u32 idx);

/*!
 * @brief inplace with swapping ring buffer linearization.
 * @ingroup core_ring_buffer
 * @param b pointer to the ring buffer
 * @param temp_swap a temporary swap buffer sized to contain a single element of
 * the buffer. e.g. for a buffer of float elements a pointer to a single float
 * element is needed. NOTE: when temp_buffer is NULL, no data swapping is performed
 * but only buffer internal state is updated in order to linearize the buffer.
 * useful when we wants to linearize a buffer without actually moving elements.
 * @return a pointer to 1st element on the linearized buffer
 */
AI_INTERNAL_API
ai_handle core_ring_buffer_linearize(
  core_ring_buffer* b, ai_handle temp_swap);

AI_INTERNAL_API
ai_bool core_ring_buffer_clone(
  core_ring_buffer* dst,
  const core_ring_buffer* src);

#ifdef HAS_AI_BUILTIN_UNIT_TESTS
AI_INTERNAL_API
int ai_builtin_unit_tests_core_ring_buffer(
  int argc, char* argv[]);
#endif  /*HAS_AI_BUILTIN_UNIT_TESTS*/


AI_API_DECLARE_END

#endif    /*CORE_RING_BUFFER_H*/
