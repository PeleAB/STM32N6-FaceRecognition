/**
  ******************************************************************************
  * @file    operators_nl.h
  * @author  AST Embedded Analytics Research Platform
  * @brief   header file of AI platform nonlinearity operators datatypes
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
#ifndef OPERATORS_NL_H
#define OPERATORS_NL_H
#pragma once

#include "operators_common.h"

/*!
 * @defgroup operators_nl Normalization Operators Definitions
 * @brief definition
 *
 */

AI_API_DECLARE_BEGIN

/*!
 * @struct ai_operator_nl
 * @ingroup operators_nl
 * @brief Nonlinearity operator
 *
 * The type of nonlinearity is handled by the specific forward function.
 * It is a sequential operator. see @ref ai_operator_sequential
 */
typedef ai_operator_sequential ai_operator_nl;

/*!
 * @struct ai_operator_nl_selu
 * @ingroup operators_nl
 * @brief Scaled Exponential Linear Unit (SELU) operator
 *
 * Scaled Exponential Linear Unit (SELU).
 * It is a sequential operator. see @ref ai_operator_sequential
 */
typedef AI_ALIGNED_TYPE(struct, 4) ai_operator_nl_selu_ {
  AI_OPERATOR_SEQUENTIAL_FIELDS_DECLARE
  ai_float alpha;    /*!< normalization exponent */
  ai_float gamma;    /*!< slope */
} ai_operator_nl_selu;


/*!
 * @struct ai_operator_nl_prelu
 * @ingroup operators_nl
 * @brief Parametric Exponential Linear Unit (PRELU) operator
 *
 * Parametric Exponential Linear Unit (PRELU).
 * It is a sequential operator. see @ref ai_operator_sequential
 */
typedef AI_ALIGNED_TYPE(struct, 4) ai_operator_nl_prelu_ {
  AI_OPERATOR_SEQUENTIAL_FIELDS_DECLARE
  ai_tensor* slope;    /*!< slope parameters */

} ai_operator_nl_prelu;

/*!
 * @struct ai_operator_sm
 * @ingroup operators_nl
 * @brief Softmax nonlinear operator
 *
 * The softmax operator is handled separately because it involves a
 * normalization step along the channel axis. It is a sequential operator.
 * See @ref ai_operator_sequential
 */
typedef ai_operator_sequential ai_operator_sm;


/******************************************************************************/
/*  Compute Functions Section                                                 */
/******************************************************************************/

/*!
 * @brief Computes the activations of a ReLU nonlinear operator.
 * @ingroup operators_nl
 * @param op the nonlinear (nl) operator
 */
AI_INTERNAL_API
void compute_relu(ai_operator* op, ai_float arg,ai_handle out);

/*!
 * @brief Computes the activations of a ReLU6 nonlinear operator.
 * @ingroup operators_nl
 * @param op the nonlinear (nl) operator
 */
//TODO
AI_INTERNAL_API
void compute_relu6(ai_operator* op, ai_float arg, ai_handle out);



/*!
 * @brief Computes the activations of a SELU nonlinear operator.
 * @ingroup operators_nl
 * @param op the nonlinear (nl) operator
 */
//TODO
AI_INTERNAL_API
void compute_selu(ai_operator *op, ai_float arg, ai_handle out);

/*!
 * @brief Computes the activations of a binary tanh (sign) nonlinear operator.
 * @ingroup operators
 * @param op the nonlinear (nl) operator
 */
AI_INTERNAL_API
void compute_sign(ai_operator *op, ai_float arg, ai_handle out);

/*!
 * @brief Computes the activations of a sigmoid nonlinear operator.
 * @ingroup operators_nl
 * @param op the nonlinear (nl) operator
 */
AI_INTERNAL_API
void compute_sigmoid(ai_operator* op, ai_float arg, ai_handle out);

/*!
 * @brief Computes the activations of a hard_sigmoid nonlinear operator.
 * @ingroup operators_nl
 * @param op the nonlinear (nl) operator
 */
void compute_hard_sigmoid(ai_operator* op, ai_float arg, ai_handle out);



/*!
 * @brief Computes the activations of a hyperbolic tangent (tanh) operator.
 * @ingroup operators_nl
 * @param op the nonlinear (nl) operator
 */
AI_INTERNAL_API
void compute_tanh(ai_operator* op, ai_float arg, ai_handle out);

/*!
 * @brief Computes the activations of a softmax nonlinear operator.
 * @ingroup operators_nl
 * @param op the softmax (sm) operator
 */
AI_INTERNAL_API
void compute_sm(ai_operator *op, ai_float arg, ai_handle out);


/******************************************************************************/
/* Inline Compute Functions Section                                           */
/******************************************************************************/

/*!
 * @brief Computes the activations of a ReLU nonlinear operator.
 * @ingroup operators_nl
 * @param op the nonlinear (nl) operator
 */
AI_INTERNAL_API
void compute_relu_inline(ai_operator* op, ai_float arg,ai_handle out);

/*!
 * @brief Computes the activations of a ReLU6 nonlinear operator.
 * @ingroup operators_nl
 * @param op the nonlinear (nl) operator
 */
//TODO
AI_INTERNAL_API
void compute_relu6_inline(ai_operator* op, ai_float arg, ai_handle out);



/*!
 * @brief Computes the activations of a SELU nonlinear operator.
 * @ingroup operators_nl
 * @param op the nonlinear (nl) operator
 */
//TODO
AI_INTERNAL_API
void compute_selu_inline(ai_operator *op, ai_float arg, ai_handle out);

/*!
 * @brief Computes the activations of a binary tanh (sign) nonlinear operator.
 * @ingroup operators
 * @param op the nonlinear (nl) operator
 */
AI_INTERNAL_API
void compute_sign_inline(ai_operator *op, ai_float arg, ai_handle out);

/*!
 * @brief Computes the activations of a sigmoid nonlinear operator.
 * @ingroup operators_nl
 * @param op the nonlinear (nl) operator
 */
AI_INTERNAL_API
void compute_sigmoid_inline(ai_operator* op, ai_float arg, ai_handle out);

/*!
 * @brief Computes the activations of a hard_sigmoid nonlinear operator.
 * @ingroup operators_nl
 * @param op the nonlinear (nl) operator
 */
void compute_hard_sigmoid_inline(ai_operator* op, ai_float arg, ai_handle out);



/*!
 * @brief Computes the activations of a hyperbolic tangent (tanh) operator.
 * @ingroup operators_nl
 * @param op the nonlinear (nl) operator
 */
AI_INTERNAL_API
void compute_tanh_inline(ai_operator* op, ai_float arg, ai_handle out);

/*!
 * @brief Computes the activations of a softmax nonlinear operator.
 * @ingroup operators_nl
 * @param op the softmax (sm) operator
 */
AI_INTERNAL_API
void compute_sm_inline(ai_operator *op, ai_float arg, ai_handle out);

/*!
 * @brief Computes the activations of a swish nonlinear operator.
 * @ingroup operators_nl
 * @param op the swish operator
 */
AI_INTERNAL_API
void compute_swish_inline(ai_operator *op, ai_float arg, ai_handle out);

/*!
 * @brief Computes the activations of a hard swish nonlinear operator.
 * @ingroup operators_nl
 * @param op the hard swish operator
 */
AI_INTERNAL_API
void compute_hard_swish_inline(ai_operator *op, ai_float arg, ai_handle out);

AI_API_DECLARE_END

#endif    /*OPERATORS_NL_H*/
