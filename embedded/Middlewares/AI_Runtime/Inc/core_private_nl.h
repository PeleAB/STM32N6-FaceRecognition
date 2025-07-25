/**
  ******************************************************************************
  * @file    core_private_nl.h
  * @author  AST Embedded Analytics Research Platform
  * @brief   private header file of common nonlinear activations routines
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
#ifndef CORE_PRIVATE_NL_H
#define CORE_PRIVATE_NL_H

#include "ai_platform.h"
#include "ai_datatypes_internal.h"
#include "core_log.h"
#include "core_private.h"

#define CORE_NL_GET_PARAMS(tensor, array) \
  core_nl_get_params(tensor, array)

#define CORE_NL_GET_PARAMS_PTR(tensor, array) \
  core_nl_get_params_ptr(tensor, array)

#define CORE_NL_GET_PARAMS_STRIDE(tensor) \
  core_nl_get_params_stride(tensor)

#define CORE_NL_GET_PARAMS_BYTE_SIZE(tensor) \
  core_nl_get_params_byte_size(tensor)


AI_DECLARE_STATIC
ai_handle core_nl_clone_params(
  const ai_tensor* t, const ai_array* a)
{
  static ai_array s_array = AI_STRUCT_INIT;
  static ai_tensor s_tensor = AI_STRUCT_INIT;
  if (t && t->data) {
    AI_ASSERT(a==NULL)
    // bind to cloned tensor instance obj
    s_tensor = *t;
    s_array = *(t->data);
    // bind to cloned array instance obj
    s_tensor.data = &s_array;

    AI_LOG_TRACE("[core_nl_clone_params] tensor(%p) from %p", &s_tensor, t)
    return AI_HANDLE_PTR(&s_tensor);
  }

  AI_ASSERT(a)
  s_array = *a;
  AI_LOG_TRACE("[core_nl_clone_params] array(%p) from %p", &s_array, a)
  return AI_HANDLE_PTR(&s_array);
}

AI_DECLARE_STATIC
ai_handle core_nl_get_params(
  const ai_tensor* t, const ai_array* a)
{
  if (t && t->data) {
    AI_ASSERT(a==NULL)

    AI_LOG_TRACE("[core_nl_get_params] tensor(%p)", t)
    return AI_HANDLE_PTR(t);
  }

  AI_LOG_TRACE("[core_nl_get_params] array(%p)", a)
  return AI_HANDLE_PTR(a);
}

AI_DECLARE_STATIC
ai_ptr_const core_nl_get_params_ptr(
  const ai_tensor* t, const ai_array* a)
{
  AI_ASSERT( (!a && (t && t->data)) || (!t && (a && a->data)) || (!(t || a)) )
  const ai_array* out = (t && t->data) ? t->data : a;
  AI_LOG_TRACE("[core_nl_get_params_ptr]: t(%p) a(%p) out(%p) data(%p)",
    t, a, out, (out) ? out->data : NULL)
  return (out) ? AI_PTR_CONST(out->data) : NULL;
}

AI_DECLARE_STATIC
ai_ptr_offset core_nl_get_params_stride(const ai_tensor* t)
{
  AI_ASSERT( (t && AI_STRIDE_W(AI_TENSOR_STRIDE(t))!=0) || (NULL==t))
  // return (t && t->data) ? AI_STRIDE_W(AI_TENSOR_STRIDE(t)) : 0;
  /* Prelu pre-padded params bugfix */
  return (t && t->data) ? AI_ARRAY_OBJ_BYTE_SIZE(t->data) : 0;
}

AI_DECLARE_STATIC
ai_ptr_offset core_nl_get_params_byte_size(const ai_tensor* t)
{
  AI_ASSERT( (t && t->data) || (NULL==t))
  return (t && t->data) ? AI_ARRAY_GET_BYTE_SIZE(t->data->format, t->data->size) : 1;
}

#endif  /*CORE_PRIVATE_NL_H*/
