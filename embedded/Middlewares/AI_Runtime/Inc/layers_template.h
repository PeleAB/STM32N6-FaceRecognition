/**
  ******************************************************************************
  * @file    layers_template.h
  * @author  STMicroelectronics
  * @brief   header file of generic template layers
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
#ifndef LAYERS_TEMPLATE_H
#define LAYERS_TEMPLATE_H

#include "layers_common.h"

/*!
 * @defgroup layers_template Generic Template Layer Definition
 * @brief definition
 *
 */

AI_API_DECLARE_BEGIN

/*!
 * @struct ai_layer_template
 * @ingroup layers_template
 * @brief Specific instance of  template layer
 */
/* Case 1a) : Simple Layer (stateless): layer struct inherits from ai_layer_base */
// typedef ai_layer_base ai_layer_template;

/* Case 1b) : Simple Layer (stateful): layer struct inherits from ai_layer_stateful */
// typedef ai_layer_stateful ai_layer_template;

/* Case 2a) : Complex Layer (stateless): layer struct defined from scratch
              keeping layer required datastructure on top of the structure */
/*
typedef AI_ALIGNED_TYPE(struct, 4) ai_layer_template_ {
  AI_LAYER_COMMON_FIELDS_DECLARE
  ai_u32    my_attribute_u32;
  ai_float  my_attribute_float;
  ai_array* my_attribute_arrays;
} ai_layer_template;
*/

/* Case 2b) : Complex Layer (Stateful): layer struct defined from scratch
              keeping layer required datastructure on top of the structure */
typedef AI_ALIGNED_TYPE(struct, 4) ai_layer_template_ {
  AI_LAYER_STATEFUL_FIELDS_DECLARE
  ai_u32    my_attribute_u32;
  ai_float  my_attribute_float;
  ai_array* my_attribute_arrays;
} ai_layer_template;


/*****************************************************************************/
/*  Forward Functions Section                                                */
/*****************************************************************************/

/*!
 * @brief (Required only by stateful layers) init function for generic stateful template layer.
 * @ingroup layers_template
 * @param layer template layer as an opaque pointer
 */
AI_INTERNAL_API
void init_template(ai_layer* layer);

/*!
 * @brief (Required only by stateful layers) destroy function for generic stateful template layer.
 * @ingroup layers_template
 * @param layer template layer as an opaque pointer
 */
AI_INTERNAL_API
void destroy_template(ai_layer* layer);


/*!
 * @brief (REQUIRED) Forward function for generic template layer.
 * @ingroup layers_template
 * @param layer template layer as an opaque pointer
 */
AI_INTERNAL_API
void forward_template(ai_layer* layer);


AI_API_DECLARE_END

#endif    /*LAYERS_TEMPLATE_H*/
