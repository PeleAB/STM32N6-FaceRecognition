/**
  ******************************************************************************
  * @file    core_utils.h
  * @author  AST Embedded Analytics Research Platform
  * @brief   header file of core utils routines
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

#ifndef __CORE_UTILS_H_
#define __CORE_UTILS_H_


#include "ai_platform.h"
#include "ai_platform_interface.h"

#include "core_common.h"

AI_API_DECLARE_BEGIN

/*!
 * @defgroup core_utils Core Utils Routines
 * @brief Implementation of core utils such has checksums algorithms, etc.
 */


/*!
 * @brief Computes 32bit checksum from a buffer array of bytes
 * @ingroup core_utils
 * @param[in] buffer in an opaque handler to the buffer we want to compute crc code
 * @param[in] byte_size the size in byte of the buffer provided
 */
AI_INTERNAL_API
ai_u32    core_utils_compute_crc32(
  const ai_handle buffer, const ai_size byte_size);

/*!
 * @brief Computes network signature given a network context
 * @ingroup core_utils
 * @param[in] net_ctx a pointer to the network context to be signed
 * @param[out] signature a pointer to the checksum signature computed
 * from the network context
 * @return false if failed to compute the signature, true otherwise
 */
AI_INTERNAL_API
ai_bool    core_utils_generate_network_signature(
  const ai_network* net_ctx, ai_signature* signature);

/*!
 * @brief Computes the broadcasted shape between two tensors
 * @ingroup core_utils
 * @param[out] out the broadcasted shape
 * @param[in] x shape of the first tensor
 * @param[in] y shape of the second tensor
 */
AI_INTERNAL_API
void core_get_broadcasted_shape(
  ai_shape* out, const ai_shape* x, const ai_shape* y);

/*!
 * @brief Computes the offsets need to broadcast a tensor
 * @ingroup core_utils
 * @param[out] out the per-dimension offsets
 * @param[in] in_shape the shape of the tensor to be broadcasted
 * @param[in] out_shape the target shape for broadcasting
 * @param[in] elem_size the size of the element to be broadcasted
 */
AI_INTERNAL_API
void core_compute_offsets(
  ai_stride* out,
  const ai_shape* in_shape, const ai_shape* out_shape,
  const ai_i32 elem_size);

/*!
 * @brief Clone a tensor to another tensor struct preserving shapes and strides
 * @details this routine clone a tensor datastruct copying all fields
 * including the shape and stride data
 * @ingroup core_utils
 * @param[out] dst the destination tensor
 * @param[in] src the input tensor to clone
 */
AI_INTERNAL_API
void core_tensor_clone(
  ai_tensor* dst, const ai_tensor* src);

/*!
 * @brief Clone an ai_array to another ai_array struct
 * @details this routine clone an array datastruct copying all fields
 * @ingroup core_utils
 * @param[out] dst the destination array
 * @param[in] src the input array to clone
 */
AI_INTERNAL_API
void core_array_clone(
  ai_array* dst, const ai_array* src);

AI_API_DECLARE_END

#endif    /*__CORE_UTILS_H_*/
