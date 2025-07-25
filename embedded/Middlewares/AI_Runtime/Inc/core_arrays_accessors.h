/**
  ******************************************************************************
  * @file    core_arrays_accessors.h
  * @author  AST Embedded Analytics Research Platform
  * @brief   header file of common core datatypes
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  
  ******************************************************************************
  */

#ifndef CORE_ARRAYS_ACCESSORS_H
#define CORE_ARRAYS_ACCESSORS_H


#include "ai_platform.h"
#include "ai_platform_interface.h"
#include "ai_datatypes_defines.h"
#include "ai_datatypes_format.h"

#define ARRAY_HANDLE_GET_AS_FLOAT(handle_ptr_, array_fmt_) \
  _array_handle_get_as_float(handle_ptr_, array_fmt_)

#define ARRAY_ANY_PTR_GET_AS_FLOAT(array_any_ptr_, array_fmt_) \
  _array_any_ptr_get_as_float(array_any_ptr_, array_fmt_)

#define ARRAY_ANY_PTR_GET_AS_FLOAT_INC(array_any_ptr_, array_fmt_) \
  _array_any_ptr_get_as_float_inc(&(array_any_ptr_), array_fmt_)


#define ARRAY_HANDLE_SET_FROM_FLOAT(handle_ptr_, array_fmt_, value_) \
  _array_handle_set_from_float(handle_ptr_, array_fmt_, value_)


AI_DECLARE_STATIC
ai_float _array_handle_get_as_float(const ai_handle ptr, const ai_array_format fmt)
{
  ai_float value = 0.0f;
  switch (fmt) {
    case AI_ARRAY_FORMAT_BOOL:    value = (*AI_CAST(ai_u8*, ptr)) ? 1.0f : 0.0f; break;
    case AI_ARRAY_FORMAT_FLOAT:   value = *AI_CAST(ai_float*, ptr); break;
    case AI_ARRAY_FORMAT_S8:      value = *AI_CAST(ai_i8*, ptr); break;
    case AI_ARRAY_FORMAT_U8:      value = *AI_CAST(ai_u8*, ptr); break;
    case AI_ARRAY_FORMAT_S16:     value = *AI_CAST(ai_i16*, ptr); break;
    case AI_ARRAY_FORMAT_U16:     value = *AI_CAST(ai_u16*, ptr); break;
    case AI_ARRAY_FORMAT_S32:     value = *AI_CAST(ai_i32*, ptr); break;
    case AI_ARRAY_FORMAT_U32:     value = *AI_CAST(ai_u32*, ptr); break;
    case AI_ARRAY_FORMAT_S64:     value = *AI_CAST(ai_i64*, ptr); break;
    case AI_ARRAY_FORMAT_U64:     value = *AI_CAST(ai_u64*, ptr); break;
    default: AI_ASSERT(0) break;
  }
  // AI_DEBUG_PRINT("## [_array_handle_get_as_float] ptr(%p) fmt(%s) value(%f)\n", ptr, AI_ARRAY_FMT_NAME(fmt), value)
  return value;
}


AI_DECLARE_STATIC
ai_float _array_any_ptr_get_as_float(const ai_any_ptr ptr, const ai_array_format fmt)
{
  ai_float value = 0.0f;
  switch (fmt) {
    case AI_ARRAY_FORMAT_BOOL:    value = (*ptr.u8) ? 1.0f : 0.0f; break;
    case AI_ARRAY_FORMAT_FLOAT:   value = *ptr.float32; break;
    case AI_ARRAY_FORMAT_S8:      value = *ptr.s8; break;
    case AI_ARRAY_FORMAT_U8:      value = *ptr.u8; break;
    case AI_ARRAY_FORMAT_S16:     value = *ptr.s16; break;
    case AI_ARRAY_FORMAT_U16:     value = *ptr.u16; break;
    case AI_ARRAY_FORMAT_S32:     value = *ptr.s32; break;
    case AI_ARRAY_FORMAT_U32:     value = *ptr.u32; break;
    case AI_ARRAY_FORMAT_S64:     value = *ptr.s64; break;
    case AI_ARRAY_FORMAT_U64:     value = *ptr.u64; break;
    default: AI_ASSERT(0) break;
  }
  // AI_DEBUG_PRINT("## [_array_any_ptr_get_as_float] ptr(%p) fmt(%s) value(%f)\n", ptr.handle, AI_ARRAY_FMT_NAME(fmt), value)
  return value;
}


AI_DECLARE_STATIC
ai_float _array_any_ptr_get_as_float_inc(ai_any_ptr* ptr, const ai_array_format fmt)
{
  ai_float value = 0.0f;
  switch (fmt) {
    case AI_ARRAY_FORMAT_BOOL:    value = (*ptr->u8++) ? 1.0f : 0.0f; break;
    case AI_ARRAY_FORMAT_FLOAT:   value = *ptr->float32++; break;
    case AI_ARRAY_FORMAT_S8:      value = *ptr->s8++; break;
    case AI_ARRAY_FORMAT_U8:      value = *ptr->u8++; break;
    case AI_ARRAY_FORMAT_S16:     value = *ptr->s16++; break;
    case AI_ARRAY_FORMAT_U16:     value = *ptr->u16++; break;
    case AI_ARRAY_FORMAT_S32:     value = *ptr->s32++; break;
    case AI_ARRAY_FORMAT_U32:     value = *ptr->u32++; break;
    case AI_ARRAY_FORMAT_S64:     value = *ptr->s64++; break;
    case AI_ARRAY_FORMAT_U64:     value = *ptr->u64++; break;
    default: AI_ASSERT(0) break;
  }
  // AI_DEBUG_PRINT("## [_array_any_ptr_get_as_float_inc] ptr(%p) fmt(%s) value(%f)\n", ptr->handle, AI_ARRAY_FMT_NAME(fmt), value)
  return value;
}


AI_DECLARE_STATIC
void _array_handle_set_from_float(ai_handle ptr, const ai_array_format fmt, const ai_float value)
{
  switch (fmt) {
    case AI_ARRAY_FORMAT_BOOL:    *AI_CAST(ai_u8*, ptr) = AI_CAST(ai_bool, value); break;
    case AI_ARRAY_FORMAT_FLOAT:   *AI_CAST(ai_float*, ptr) = AI_CAST(ai_float, value); break;
    case AI_ARRAY_FORMAT_S8:      *AI_CAST(ai_i8*, ptr) = AI_CAST(ai_i8, value); break;
    case AI_ARRAY_FORMAT_U8:      *AI_CAST(ai_u8*, ptr) = AI_CAST(ai_u8, value); break;
    case AI_ARRAY_FORMAT_S16:     *AI_CAST(ai_i16*, ptr) = AI_CAST(ai_i16, value); break;
    case AI_ARRAY_FORMAT_U16:     *AI_CAST(ai_u16*, ptr) = AI_CAST(ai_u16, value); break;
    case AI_ARRAY_FORMAT_S32:     *AI_CAST(ai_i32*, ptr) = AI_CAST(ai_i32, value); break;
    case AI_ARRAY_FORMAT_U32:     *AI_CAST(ai_u32*, ptr) = AI_CAST(ai_u32, value); break;
    case AI_ARRAY_FORMAT_S64:     *AI_CAST(ai_i64*, ptr) = AI_CAST(ai_i64, value); break;
    case AI_ARRAY_FORMAT_U64:     *AI_CAST(ai_u64*, ptr) = AI_CAST(ai_u64, value); break;
    default: AI_ASSERT(0) break;
  }
  AI_DEBUG_PRINT("## [_array_handle_set_as_float] ptr(%p) fmt(%s) value(%f)\n", ptr, AI_ARRAY_FMT_NAME(fmt), value)
}

#endif    /*CORE_ARRAYS_ACCESSORS_H*/
