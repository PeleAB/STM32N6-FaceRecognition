/**
  ******************************************************************************
  * @file    operators_conv2d.h
  * @author  AST Embedded Analytics Research Platform
  * @brief   header file of AI platform conv2d operators datatypes
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
#ifndef OPERATORS_CONV2D_H
#define OPERATORS_CONV2D_H
#pragma once
#include "operators_common.h"
#include "operators_nl.h"
#include "operators_pool.h"


#define AI_OPERATOR_CONV2D_FIELDS_DECLARE \
  AI_OPERATOR_SEQUENTIAL_FIELDS_DECLARE \
  ai_u32      groups;  /*!< groups for separable convolution */ \
  ai_shape_2d filter_stride;  /*!< filter stride, how much the filter moves */ \
  ai_shape_2d filter_pad;     /*!< filter pad, y,x border sizes */ \
  ai_shape_2d filter_pad_r;  /*!< right filter pad, for asymmetric padding */\
  ai_shape_2d dilation;

/*!
 * @defgroup operators_conv2d Convolutive Layers Definitions
 * @brief definition
 *
 */

AI_API_DECLARE_BEGIN

/*!
 * @struct ai_operator_conv2d
 * @ingroup operators_conv2d
 * @brief 2D convolutional operator with strides and pads
 */
typedef AI_ALIGNED_TYPE(struct, 4) ai_operator_conv2d_ {
  AI_OPERATOR_CONV2D_FIELDS_DECLARE
} ai_operator_conv2d;

typedef AI_ALIGNED_TYPE(struct, 4) ai_operator_conv2d_grp_ {
  AI_OPERATOR_CONV2D_FIELDS_DECLARE \

    ai_shape_2d pool_size;    /*!< pooling size */
    ai_shape_2d pool_stride;  /*!< pooling stride */
    ai_shape_2d pool_pad;     /*!< pooling pad */

} ai_operator_conv2d_grp;


typedef AI_ALIGNED_TYPE(struct, 4) ai_operator_dense_ {
  AI_OPERATOR_SEQUENTIAL_FIELDS_DECLARE
}ai_operator_dense;


/******************************************************************************/
/* Forward Functions Section                                                  */
/******************************************************************************/

/*!
 * @brief Computes the activations of a 2D convolutional layer.
 * @ingroup operators_conv2d
 * @param op the convolutional (conv) operator
 */
AI_INTERNAL_API
void compute_conv2d_seq(ai_operator *op, ai_float arg, ai_handle out);

/*!
 * @brief Computes the activations of a 2D convolutional layer in a pool-region
 * driven schema.
 * To be used with pool_grp compute functions.
 * @ingroup operators_conv2d
 * @param op the convolutional (conv) operator
 */
AI_INTERNAL_API
void compute_conv2d_grp(ai_operator *op, ai_float arg, ai_handle out);

/*!
 * @brief Computes the activations of a dense (fully connected) operator inline.
 * @ingroup operators_conv2d
 * @param op the dense operator
 * @param arg the argument to process
 * @param out opaque pointer to output buffer
 */
AI_INTERNAL_API
void compute_dense_inline(ai_operator* op, ai_float arg, ai_handle out);

/*!
 * @brief Computes the activations of a dense (fully connected) operator.
 * @ingroup operators_conv2d
 * @param op the dense operator
 */
AI_INTERNAL_API
void compute_dense(ai_operator* op, ai_float arg, ai_handle out);

AI_API_DECLARE_END

#endif    /*OPERATORS_CONV2D_H*/
