/**
  ******************************************************************************
  * @file    operators_binary.h
  * @author  AST Embedded Analytics Research Platform
  * @brief   header file of binary operator datatypes
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
#ifndef OPERATORS_BINARY_H
#define OPERATORS_BINARY_H
#pragma once

#include "operators_common.h"


AI_API_DECLARE_BEGIN

//typedef ai_float (*func_binary)(const ai_float a, const ai_float b);
typedef void (*func_binary)(ai_handle out,const ai_handle a, const ai_handle b);

typedef AI_ALIGNED_TYPE(struct, 4) ai_operator_binary_ {
  AI_OPERATOR_COMMON_FIELDS_DECLARE
  func_binary func;
} ai_operator_binary;

void compute_binary_op(ai_operator *o, ai_float arg, ai_handle out);

void compute_unary_op(ai_operator *o, ai_float arg, ai_handle out);

AI_API_DECLARE_END

#endif /*OPERATORS_BINARY_H*/
