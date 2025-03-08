/**
  ******************************************************************************
  * @file    operators_private.h
  * @author  AST Embedded Analytics Research Platform
  * @brief   private header file of AI platform operators datatypes
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
#ifndef OPERATORS_PRIVATE_H
#define OPERATORS_PRIVATE_H
#pragma once
#include "ai_platform.h"
#include "ai_datatypes_internal.h"
#include "ai_lite_interface.h"

#include "core_private.h"

#define AI_OPERATOR_ACQUIRE(name_, cast_type_, ptr_) \
  AI_ASSERT(ptr_) \
  AI_CONCAT(ai_operator_, cast_type_)* name_ = \
    (AI_CONCAT(ai_operator_, cast_type_)*)(ptr_); \
  AI_LOG_TRACE("Entering Operator %s (ID :%u) (PTR: %p) (TYPE: %u) (STRUCT: %s)", \
               AI_OPERATOR_TYPE_NAME((name_)->type), (name_)->id, (name_), \
               (name_)->type, AI_STRINGIFY(AI_CONCAT(ai_operator_, cast_type_))) \

#define AI_OPERATOR_RELEASE(name_, cast_type_) \
  AI_LOG_TRACE("Exiting Operator %s (ID :%u) (PTR: %p) (TYPE: %u) (STRUCT: %s) ", \
               AI_OPERATOR_TYPE_NAME((name_)->type), (name_)->id, (name_), \
               (name_)->type, AI_STRINGIFY(AI_CONCAT(ai_operator_, cast_type_)))

#endif    /*OPERATORS_PRIVATE_H*/
