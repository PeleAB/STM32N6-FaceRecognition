/**
  ******************************************************************************
  * @file    layers_private.h
  * @author  AST Embedded Analytics Research Platform
  * @brief   private header file of AI platform layers datatypes
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
#ifndef LAYERS_PRIVATE_H
#define LAYERS_PRIVATE_H

#include "ai_platform.h"
#include "ai_platform_interface.h"
#include "ai_datatypes_internal.h"
#include "ai_lite_interface.h"

#include "core_private.h"

#define AI_LAYER_IS_STATEFUL        (0x1U << 24)

#define AI_LAYER_HAS_INIT_FUNC      (0x1U << 0)
#define AI_LAYER_HAS_UPDATE_FUNC    (0x1U << 1)
#define AI_LAYER_HAS_DESTROY_FUNC   (0x1U << 2)


/* Basic sanity checks for generic layer datastructs */
#define ASSERT_LAYER_SANITY_LITE(l) \
  do { \
    AI_ASSERT((l)->tensors && (l)->tensors->chain) \
  } while (0);

#define ASSERT_LAYER_SANITY(l) \
  do { \
    AI_ASSERT((l)->tensors && (l)->tensors->chain) \
    ASSERT_TENSOR_DATA_SANITY(GET_TENSOR_IN((l)->tensors, 0)) \
    ASSERT_TENSOR_DATA_SANITY(GET_TENSOR_OUT((l)->tensors, 0)) \
    AI_ASSERT(CORE_TENSOR_GET_SHAPE_SIZE(GET_TENSOR_IN((l)->tensors, 0)) <= \
              AI_ARRAY_OBJ_SIZE(GET_TENSOR_IN((l)->tensors, 0)->data))  \
    AI_ASSERT(CORE_TENSOR_GET_SHAPE_SIZE(GET_TENSOR_OUT((l)->tensors, 0)) <= \
              AI_ARRAY_OBJ_SIZE(GET_TENSOR_OUT((l)->tensors, 0)->data)) \
  } while (0);


#define AI_LAYER_CAST(name_, cast_type_, ptr_) \
  AI_ASSERT(ptr_) \
  AI_CONCAT(ai_layer_, cast_type_)* name_ = \
    (AI_CONCAT(ai_layer_, cast_type_)*)(ptr_);

#define AI_LAYER_ACQUIRE_INIT(name_, cast_type_, ptr_) \
  AI_LAYER_CAST(name_, cast_type_, ptr_) \
  AI_LOG_TRACE("Initialize Layer %s (ID :%u) (PTR: %p) (TYPE: %u) (STRUCT: %s) (FLAGS: 0x%08x)", \
               AI_LAYER_TYPE_NAME((name_)->type), (name_)->id, (name_), \
               (name_)->type, AI_STRINGIFY(AI_CONCAT(ai_layer_, cast_type_)), \
               (name_)->flags) \
  ASSERT_LAYER_SANITY_LITE(name_)

#define AI_LAYER_ACQUIRE(name_, cast_type_, ptr_) \
  AI_LAYER_CAST(name_, cast_type_, ptr_) \
  AI_LOG_TRACE("Entering Layer %s (ID :%u) (PTR: %p) (TYPE: %u) (STRUCT: %s) (FLAGS: 0x%08x)", \
               AI_LAYER_TYPE_NAME((name_)->type), (name_)->id, (name_), \
               (name_)->type, AI_STRINGIFY(AI_CONCAT(ai_layer_, cast_type_)), \
               (name_)->flags) \
  ASSERT_LAYER_SANITY(name_)

#define AI_LAYER_RELEASE(name_, cast_type_) \
  AI_LOG_TRACE("Exiting Layer %s (ID :%u) (PTR: %p) (TYPE: %u) (STRUCT: %s) (FLAGS: 0x%08x)", \
               AI_LAYER_TYPE_NAME((name_)->type), (name_)->id, (name_), \
               (name_)->type, AI_STRINGIFY(AI_CONCAT(ai_layer_, cast_type_)), \
               (name_)->flags)

#define AI_LAYER_TYPE_IS_VALID(type_) \
  ai_layer_type_is_valid(AI_LAYER_TYPE(type_))


AI_API_DECLARE_BEGIN

/*!
 * @brief Initialize network layers.
 * @ingroup layers_private
 * @param net the network to initialize
 * @return the number of initialized layers
 */
AI_INTERNAL_API
ai_size ai_layers_init_all(ai_network* net);


/*!
 * @brief Initialize network layers (post call).
 * @ingroup layers_private
 * @param net the network to initialize
 * @return the number of initialized layers
 */
AI_INTERNAL_API
ai_size ai_layers_post_init_all(ai_network* net);

/*!
 * @brief Computes the ouptut of the network given the input.
 * @ingroup layers_private
 *
 * Given a network with the input pre-loaded in the net &rarr; in tensor,
 * computes the output by calling the forward functions of each layer and
 * selecting the next layer. When the layer has no successor or it's in a
 * loop-back configuration (layer &rarr; next is again layer), the function
 * stops. The result is stored in net &rarr; out.
 *
 * @param net the network to process
 * @return the number of executed layers
 */
AI_INTERNAL_API
ai_size ai_layers_forward_all(ai_network* net);

/*!
 * @brief Destroy network layers.
 * @ingroup layers_private
 * @param net the network to destroy
 * @return the number of destroyed layers
 */
AI_INTERNAL_API
ai_size ai_layers_destroy_all(ai_network* net);


AI_API_DECLARE_END

#endif    /*LAYERS_PRIVATE_H*/
