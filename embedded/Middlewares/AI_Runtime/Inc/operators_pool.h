/**
  ******************************************************************************
  * @file    operators_pool.h
  * @author  AST Embedded Analytics Research Platform
  * @brief   header file of AI platform pooling operators datatypes
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
#ifndef OPERATORS_POOL_H
#define OPERATORS_POOL_H
#pragma once

#include "operators_common.h"

/*!
 * @defgroup operators_pool Pooling Layers Definitions
 * @brief definition
 *
 */

AI_API_DECLARE_BEGIN

/*!
 * @struct ai_operator_pool
 * @ingroup operators_pool
 * @brief Pooling operator
 *
 * The type of pooling function is handled by the specific forward function
 * @ref compute_func
 */
typedef AI_ALIGNED_TYPE(struct, 4) ai_operator_pool_ {
  AI_OPERATOR_SEQUENTIAL_FIELDS_DECLARE
  ai_shape_2d pool_size;    /*!< pooling size */
  ai_shape_2d pool_stride;  /*!< pooling stride */
  ai_shape_2d pool_pad;     /*!< pooling pad, y,x border sizes */
  ai_size ch_out;           /*!< output channel size */
} ai_operator_pool;


/*!
 * @brief Average Pooling on a float element - Inline (Seq)
 * @ingroup operators_pool
 * @param o  opaque handler to operator
 * @param arg float value input elem
 * @param out opaque handler to float output elem
 *
 * This function assumes the elements are received in a sequential order
 * e.g. Normal convolution
 */
void compute_inline_ap_seq(ai_operator *o, ai_float arg,ai_handle out);


/*!
 * @brief Average Pooling on a float element - Inline (Grp)
 * @ingroup operators_pool
 * @param o  opaque handler to operator
 * @param arg float value input elem
 * @param out opaque handler to float output elem
 *
 * This function assumes the elements are received in a pre-grouped order
 * e.g. Pool region-driven convolution
 */
void compute_inline_ap_grp(ai_operator *o, ai_float arg,ai_handle out);


/*!
 * @brief Max Pooling on a float element - Inline (Seq)
 * @ingroup operators_pool
 * @param o  opaque handler to operator
 * @param arg float value input elem
 * @param out opaque handler to float output elem
 *
 * This function assumes the elements are received in a sequential order
 * e.g. Normal convolution
 */
void compute_inline_mp_seq(ai_operator *o, ai_float arg,ai_handle out);


/*!
 * @brief Average Pooling on a float element - Inline (Grp)
 * @ingroup operators_pool
 * @param o  opaque handler to operator
 * @param arg float value input elem
 * @param out opaque handler to float output elem
 *
 * This function assumes the elements are received in a pre-grouped order
 * e.g. Pool region-driven convolution
 */
void compute_inline_mp_grp(ai_operator *o, ai_float arg,ai_handle out);


/******************************************************************************/
/* Forward Functions Section                                                  */
/******************************************************************************/

/*!
 * @brief Computes the activations of a max pooling operator.
 * @ingroup operators_pool
 * @param operator the pooling (pool) operator
 */
AI_INTERNAL_API
void compute_mp(ai_operator *o, ai_float arg,ai_handle out);
/*!
 * @brief Computes the activations of an average pooling operator.
 * @ingroup operators_pool
 * @param operator the pooling (pool) operator
 */
AI_INTERNAL_API
void compute_ap(ai_operator *o, ai_float arg,ai_handle out);


AI_API_DECLARE_END

#endif    /*OPERATORS_POOL_H*/
