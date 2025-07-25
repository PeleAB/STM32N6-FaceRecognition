/**
  ******************************************************************************
  * @file    operators.h
  * @author  AST Embedded Analytics Research Platform
  * @brief   header file of AI platform operator datatypes
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
#ifndef OPERATORS_H
#define OPERATORS_H
#pragma once

#include "operators_common.h"
#include "operators_binary.h"
#include "operators_conv2d.h"
#include "operators_nl.h"
#include "operators_norm.h"
#include "operators_pool.h"
#include "operators_rnn.h"


AI_API_DECLARE_BEGIN

/*!
 * @defgroup operators Operators
 * Implementation of the compute functions for the operators and the general
 * ai_operator datastructure used to abstract specific operator implementation
 * in the generic compute function definition
 *
 */


/*!
 * @brief Computes the prelu function on a float data array
 * @ingroup layers
 * @param in opaque handler to float, size should be 1
 * @param out opaque handler to float output elem
 * @param size size of the input data in bytes
 * @param slope slope of the current channel
 */
/* WARNING: Implementation not available
 * AI_INTERNAL_API
 * void nl_func_prelu_array_f32(ai_handle out, const ai_handle in, ai_handle slope,
 *                              const ai_size size, const ai_size n_channel_in, const ai_size n_channel_slope);
 */


 /*!
  * @brief Executes a single operator in the operator.
 * @ingroup operators
 * @param op the operator to compute
 * @return pointer to the next operator
 */
AI_INTERNAL_API
ai_bool ai_operator_compute_operator(ai_operator* op);



/*
 *--HELPERS
 */

AI_API_DECLARE_END

#endif /*OPERATORS_H*/
