/**
  ******************************************************************************
  * @file    operators_norm.h
  * @author  AST Embedded Analytics Research Platform
  * @brief   header file of AI platform normalization operators datatypes
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
#ifndef OPERATORS_NORM_H
#define OPERATORS_NORM_H
#pragma once

#include "operators_common.h"

/*!
 * @defgroup operators_norm Normalization Operators Definitions
 * @brief definition
 *
 */

AI_API_DECLARE_BEGIN

/*!
 * @struct ai_operator_bn
 * @ingroup operators_norm
 * @brief Batch normalization (scale with bias) operator
 */
typedef AI_ALIGNED_TYPE(struct, 4) ai_operator_bn_ {
  AI_OPERATOR_SEQUENTIAL_FIELDS_DECLARE
  ai_tensor* scale; /*!< operator scale factor, shape: (n_output,) */
  ai_tensor* bias;  /*!< operator bias, shape: (n_output,) */
} ai_operator_bn;

/*!
 * @struct ai_operator_lrn
 * @ingroup operators_norm
 * @brief Local Response Normalization operator
 *
 * Divides each element by a scale factor computed
 */
typedef AI_ALIGNED_TYPE(struct, 4) ai_operator_lrn_ {
  AI_OPERATOR_SEQUENTIAL_FIELDS_DECLARE
  ai_u32 local_size;   /*!< size of the normalization window */
  ai_float k;          /*!< bias term */
  ai_float alpha;      /*!< input scale */
  ai_float beta;       /*!< scale exponent */
} ai_operator_lrn;

/*!
 * @struct ai_operators_norm
 * @ingroup operatorss_norm
 * @brief Lp Normalization operators
 *
 * Normalizes the tensor along the 'axis' direction using the Lp norm.
 * Optionally divides the result by the number of the elements.
 */
typedef AI_ALIGNED_TYPE(struct, 4) ai_operators_norm_ {
  AI_OPERATOR_SEQUENTIAL_FIELDS_DECLARE
  ai_shape_idx axis;    /*! normalization axis */
  ai_float exponent;     /*!< normalization exponent p */
  ai_bool scale;    /*!< multiplies by the pth root of the number of elements */
} ai_operator_norm;


/*!
 * @brief Local response normalization computed on a float array
 * @ingroup operatorss_norm
 * @param out opaque handler to float output channel
 * @param in  opaque handler to float input channel
 * @param pad amount of padding for the channels
 */
//AI_DECLARE_STATIC
//void func_lrn_array_f32(ai_handle out, const ai_handle in,
//                        const ai_size in_size, const ai_size channel_size,
//                        const ai_i32 pad, const ai_float k,
//                        const ai_float alpha, const ai_float beta);

///*!
// * @brief Lp normalization computed on a float array
// * @ingroup operatorss_norm
// * @param out opaque handler to float output channel
// * @param in  opaque handler to float input channel
// * @param exponent p exponent for the Lp normalization
// * @param axis_stride stride (in array elements) of the normalization axis
// * @param axis_size size of the normalization axis
// * @param outer_size number of tensor slices (including the normalization axis)
// *   on which compute the normalization
// */
//AI_DECLARE_STATIC
//void func_norm_array_f32(ai_handle out, const ai_handle in,
//                         const ai_float exponent,
//                         const ai_float norm,
//                         const ai_size axis_stride,
//                         const ai_size axis_size,
//                         const ai_size outer_size);

/******************************************************************************/
/* Forward Functions Section                                                  */
/******************************************************************************/

/*!
 * @brief Computes the activations of a batchnorm (scale + bias) operators.
 * @ingroup operatorss_norm
 * @param operators the batch normalization (bn) operators
 */
AI_INTERNAL_API
void compute_bn(ai_operator* op, ai_float arg,ai_handle out);

/*!
 * @brief Computes the activations of a Local Response Normalization Layer.
 * @ingroup operatorss_norm
 * @param operators the local response normalization (lrn) operators
 */
AI_INTERNAL_API
void compute_lrn(ai_operator* op, ai_float arg,ai_handle out);

/*!
 * @brief Computes the activations of a normalization operators.
 * @ingroup operatorss_norm
 * @param operators the normalization (norm) operators
 */
AI_INTERNAL_API
void compute_norm(ai_operator* op, ai_float arg,ai_handle out);


AI_API_DECLARE_END

#endif    /*OPERATORS_NORM_H*/
