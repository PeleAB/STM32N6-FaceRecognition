/**
  ******************************************************************************
  * @file    core_debug_lib.h
  * @author  AST Embedded Analytics Research Platform
  * @brief   header file common core library debug helpers routines
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
#ifndef CORE_DEBUG_LIB_H
#define CORE_DEBUG_LIB_H


#include "ai_platform_interface.h"
#include "ai_datatypes_defines.h"

/*!
 * @defgroup core_debug_lib Core Library Debigging Functions
 * @brief Common macros, datatypes and routines for library debugging
 * @details This module contains the definitons and implementations of some
 * debugging routines that could be included at compile time to the runtime
 * library. The code of this module coul be included by defining a global
 * #define HAS_DEBUG_LIB in the building system
 */

#if defined HAS_AI_DEBUG || defined HAS_DEBUG_LIB
#define CORE_DEBUG_CHECK_ARRAY(array_ptr_, array_id_, prefix_) \
  { core_debug_check_array(array_ptr_, array_id_, prefix_); }
#else
#define CORE_DEBUG_CHECK_ARRAY(array_ptr_, array_id_, prefix_) \
  AI_WRAP_FUNC(/*CORE_DEBUG_CHECK_ARRAY(...)*/)
#endif

#ifndef HAS_DEBUG_LIB

#define CORE_DEBUG_ARRAY(array_ptr_, offset_, size_, prefix_) \
  AI_WRAP_FUNC(/*CORE_DEBUG_ARRAY(...)*/)

#define CORE_DEBUG_BUFFER(buffer_ptr_, prefix_) \
  AI_WRAP_FUNC(/*CORE_DEBUG_BUFFER(...)*/)

#define CORE_DEBUG_TENSOR(tensor_ptr_, rect_offset_, rect_size_, prefix_) \
  AI_WRAP_FUNC(/*CORE_DEBUG_TENSOR(...)*/)

#define CORE_DEBUG_ARRAY_ARRAY(buf_array_ptr_, size_, prefix_) \
  AI_WRAP_FUNC(/*CORE_DEBUG_ARRAY_ARRAY(...)*/)

#define CORE_DEBUG_BUFFER_ARRAY(buf_array_ptr_, size_, prefix_) \
  AI_WRAP_FUNC(/*CORE_DEBUG_BUFFER_ARRAY(...)*/)

#define CORE_DEBUG_TENSOR_ARRAY(tensor_array_ptr_, size_, prefix_) \
  AI_WRAP_FUNC(/*CORE_DEBUG_TENSOR_ARRAY(...)*/)

#define CORE_DEBUG_TENSOR_STATE_ARRAY(tstate_ptr_, size_, prefix_) \
  AI_WRAP_FUNC(/*CORE_DEBUG_TENSOR_STATE_ARRAY(...)*/)

#define CORE_DEBUG_TENSOR_CHAIN(chain_ptr_, prefix_) \
  AI_WRAP_FUNC(/*CORE_DEBUG_TENSOR_CHAIN(...)*/)

#define CORE_DEBUG_TENSOR_LIST(tlist_ptr_, tsize_, prefix_) \
  AI_WRAP_FUNC(/*CORE_DEBUG_TENSOR_LIST(...)*/)

#define CORE_DEBUG_TENSOR_LIST_SIZES(tlist_ptr_, tsize_, prefix_) \
  AI_WRAP_FUNC(/*CORE_DEBUG_TENSOR_LIST_SIZES(...)*/)

#else     /*HAS_DEBUG_LIB*/

#define CORE_DEBUG_ARRAY(array_ptr_, offset_, size_, prefix_) \
  { core_debug_dump_array(array_ptr_, offset_, size_, prefix_); }

#define CORE_DEBUG_BUFFER(buffer_ptr_, prefix_) \
  { core_debug_dump_buffer(buffer_ptr_, prefix_); }

#define CORE_DEBUG_TENSOR(tensor_ptr_, rect_offset_, rect_size_, prefix_) \
  { core_debug_dump_tensor(tensor_ptr_, rect_offset_, rect_size_, prefix_); }

#define CORE_DEBUG_ARRAY_ARRAY(buf_array_ptr_, size_, prefix_) \
  { core_debug_dump_array_array((const ai_array*)(buf_array_ptr_), (size_), (prefix_)); }

#define CORE_DEBUG_BUFFER_ARRAY(buf_array_ptr_, size_, prefix_) \
  { core_debug_dump_buffer_array((const ai_buffer*)(buf_array_ptr_), (size_), (prefix_)); }

#define CORE_DEBUG_TENSOR_ARRAY(tensor_array_ptr_, size_, prefix_) \
  { core_debug_dump_tensor_array((const ai_tensor*)(tensor_array_ptr_), (size_), (prefix_)); }

#define CORE_DEBUG_TENSOR_STATE_ARRAY(tstate_ptr_, size_, prefix_) \
  { core_debug_dump_tensor_state_array((const ai_tensor_state*)(tstate_ptr_), (size_), (prefix_)); }

#define CORE_DEBUG_TENSOR_CHAIN(chain_ptr_, prefix_) \
  { core_debug_dump_tensor_chain((chain_ptr_), (prefix_)); }

#define CORE_DEBUG_TENSOR_LIST(tlist_ptr_, tsize_, prefix_) \
  { core_debug_dump_tensor_list((const ai_tensor**)(tlist_ptr_), (tsize_), (prefix_)); }

#define CORE_DEBUG_TENSOR_LIST_SIZES(tlist_ptr_, tsize_, prefix_) \
  { core_debug_tensor_list_sizes((const ai_tensor**)(tlist_ptr_), (tsize_), (prefix_)); }

/******************************************************************************/

AI_API_DECLARE_BEGIN

/*!
 * @brief @ref ai_array array sanity check routine (compute array crc)
 * @ingroup core_debug_lib
 * @param[in] a is a pointer to the ai_array to check
 * @param[in] a_id is a unique array id
 * @param[in] prefix is a pointer to a prefix string to insert at each line
 */
AI_INTERNAL_API
void core_debug_check_array(
  const ai_array* a,
  const ai_i32 a_id,
  const char* prefix);

/*!
 * @brief @ref ai_array array debug inspection routine
 * @ingroup core_debug_lib
 * @param[in] a is a pointer to the ai_array to inspect
 * @param[in] offset is the offset of the data to dump
 * @param[in] size is size of the data to plot (0=no data plot)
 * @param[in] prefix is a pointer to a prefix string to insert at each line
 */
AI_INTERNAL_API
void core_debug_dump_array(
  const ai_array* a,
  const ai_size offset, const ai_size size,
  const char* prefix);

/*!
 * @brief @ref ai_buffer array debug inspection routine
 * @ingroup core_debug_lib
 * @param[in] b is a pointer to the ai_buffer to inspect
 * @param[in] prefix is a pointer to a prefix string to insert at each line
 */
void core_debug_dump_buffer(
  const ai_buffer* b,
  const char* prefix);

/*!
 * @brief @ref ai_tensor debug inspection routine
 * @ingroup core_debug_lib
 * @param[in] t is a pointer to the ai_tensor to inspect
 * @param[in] rect_offset is the start of the ai_tensor data to dump
 * @param[in] rect_size is the size of the ai_tensor data dump (NULL=no data dump)
 * @param[in] prefix is a pointer to a prefix string to insert at each line
 */
AI_INTERNAL_API
void core_debug_dump_tensor(
  const ai_tensor* t,
  const ai_shape* rect_offset, const ai_shape* rect_size,
  const char* prefix);

/*!
 * @brief @ref ai_array array debug inspection routine
 * @ingroup core_debug_lib
 * @param[in] barray is a pointer to the ai_array array to inspect
 * @param[in] bcount is the size of the ai_array array
 * @param[in] prefix is a pointer to a prefix string to insert at each line
 */
AI_INTERNAL_API
void core_debug_dump_array_array(
  const ai_array* barray, const ai_size bcount, const char* prefix);

/*!
 * @brief @ref ai_buffer array debug inspection routine
 * @ingroup core_debug_lib
 * @param[in] barray is a pointer to the ai_buffer array to inspect
 * @param[in] bcount is the size of the ai_buffer array
 * @param[in] prefix is a pointer to a prefix string to insert at each line
 */
AI_INTERNAL_API
void core_debug_dump_buffer_array(
  const ai_buffer* barray, const ai_size bcount, const char* prefix);

/*!
 * @brief @ref ai_tensor debug inspection routine
 * @ingroup core_debug_lib
 * @param[in] tarray is a pointer to the ai_tensor array to inspect
 * @param[in] tcount is the size of the ai_tensor array
 * @param[in] prefix is a pointer to a prefix string to insert at each line
 */
AI_INTERNAL_API
void core_debug_dump_tensor_array(
  const ai_tensor* tarray, const ai_size tcount, const char* prefix);

/*!
 * @brief @ref ai_tensor_state array debug inspection routine
 * @ingroup core_debug_lib
 * @param[in] state is a pointer to the ai_tensor_state array to inspect
 * @param[in] scount is the size of the ai_tensor_state array
 * @param[in] prefix is a pointer to a prefix string to insert at each line
 */
AI_INTERNAL_API
void core_debug_dump_tensor_state_array(
  const ai_tensor_state* state, const ai_size scount, const char* prefix);

/*!
 * @brief @ref ai_tensor_chain debug inspection routine
 * @ingroup core_debug_lib
 * @param[in] c is a pointer to the tensor chain to inspect
 * @param[in] prefix is a pointer to a prefix string to insert at each line
 */
AI_INTERNAL_API
void core_debug_dump_tensor_chain(
  const ai_tensor_chain* c, const char* prefix);

/*!
 * @brief @ref ai_tensor**  debug inspection routine
 * @ingroup core_debug_lib
 * @param[in] tlist is a pointer to the tensor chain to inspect
 * @param[in] prefix is a pointer to a prefix string to insert at each line
 */
AI_INTERNAL_API
ai_size core_debug_dump_tensor_list(
  const ai_tensor** tlist, const ai_size tcount, const char* prefix);

/*!
 * @brief @ref ai_tensor** debug inspection routine for sizes checking
 * @ingroup core_debug_lib
 * @param[in] tlist is a pointer to the tensor chain to inspect
 * @param[in] prefix is a pointer to a prefix string to insert at each line
 */
AI_INTERNAL_API
ai_size core_debug_tensor_list_sizes(
  const ai_tensor** tlist, const ai_size tcount, const char* prefix);

AI_API_DECLARE_END

#endif      /*HAS_DEBUG_LIB*/

#endif      /*CORE_DEBUG_LIB_H*/
