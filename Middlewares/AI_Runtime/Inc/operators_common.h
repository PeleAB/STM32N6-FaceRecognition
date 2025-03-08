/**
  ******************************************************************************
  * @file    operators_common.h
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

#ifndef OPERATORS_COMMON_H
#define OPERATORS_COMMON_H

#include "ai_platform.h"
#include "ai_platform_interface.h"

#include "core_common.h"

/* optimizations */
#define AI_OPTIM_FUNC_MP_ARRAY_F32      (0)

/* common operations */
#define AI_OPERATOR_TYPE(type_) \
  ((ai_operator_type)((ai_u32)(type_)&0xFFFF))

#define AI_OPERATOR_TYPE_NAME(type_) \
  ai_operator_type_name(AI_OPERATOR_TYPE(type_))

#define AI_OPERATOR_OBJ(ptr_) \
  ((ai_operator*)(ptr_)) /*!< type cast to generic layer datastruct */

#define AI_OPERATOR_IS_LAST(o_) \
  ((AI_OPERATOR_OBJ(o_)==AI_OPERATOR_OBJ(o_)->next) || \
   (AI_OPERATOR_OBJ(o_)->next==NULL))

#define AI_FOR_EACH_OPERATOR_DO(o_, operators_) \
  for ( ai_operator* o_ = AI_OPERATOR_OBJ(operators_); (o_); \
        o_ = (AI_OPERATOR_IS_LAST(o_) ? NULL : (o_)->next) )

/* trigger recursion */
#define ACTIVATE_REC_CHAIN(o_, buff_, out_) \
  if ((o_)->next!=(o_)) { \
    (o_)->next->compute((o_)->next, (buff_), NULL); \
  }

/*
 * unlike recursion which has the partial result inside it, we need to
 * propagate it back to the caller who can pass it to the next op
 */
#ifndef NOITERATING

#define AI_PROPAGATE_ITR_CHAIN(o_, buff_, out_) { \
  ai_float* ptr_ = (ai_float*)(out_); \
  *ptr_= (ai_float*)(buff_); \
  return true; \
}

#else

#define AI_PROPAGATE_ITR_CHAIN(o_,buff_,out_) AI_NOP

#endif

/* trigger compute function until last op reached */
#define ACTIVATE_MAIN_CHAIN(o_, buff_) \
    ai_float val_; \
    ai_operator* op_ = AI_OPERATOR_OBJ(o_); \
    val_ = (buff_); \
    while ( op_->next!=op_ ) { \
        op_->next->compute(op_->next, val_, AI_PTR(&val_)); \
        op_ = op_->next; \
    }

#define AI_PROCESS_ELEMENT(o_, el_, out_) \
    ai_float* pointer = (ai_float*)(out_);\
    if ((o_)->next != (o_)) { \
        (o_)->next->compute((o_)->next, el_, out_); \
        pointer = (ai_float*)&(el_); /* to suppress write out*/ \
    } else { \
      *pointer = el_; \
    }


#define RECURSIVE  /* recursive execution - comment to enable iteration */

#ifdef RECURSIVE
  #define AI_ACTIVATE_CHAIN(o_, buff_, out_) \
    AI_PROCESS_ELEMENT(o_, buff_, out_)
#else   /* RECURSIVE */
  #define AI_ACTIVATE_CHAIN(o_, buff_, out_) \
    AI_PROPAGATE_ITR_CHAIN(o_, buff_, out_)
#endif  /* RECURSIVE */

#define AI_OPERATOR_COMMON_FIELDS_DECLARE \
  ai_node_type type;      /*!< operator type id (see @ref ai_node_type) */ \
  ai_id_obj id;           /*!< operator object instance id (see @ref ai_id_obj) */ \
  ai_flags flags;                   /*!< operator object flags */ \
  struct ai_operator_* next;       /*!< the next operator in the sequence */ \
  ai_node* layer;           /*!<  */ \
  operator_compute_func compute;   /*!< compute function for the operator */ \
  ai_klass_obj klass;       /*!< opaque handler to specific operator implementations */ \
  ai_u32 state;             /*!< operator internal counter */\
  const ai_tensor_chain* tensors; /*!< pointer to node tensor chain */



#define AI_OPERATOR_COMMON_INIT(type_, compute_, next_, node_, klass_) \
  .type = AI_CONCAT(AI_OPERATOR_, type_), \
  .id = 0x0, \
  .flags = AI_FLAG_NONE, \
  .next = AI_OPERATOR_OBJ(next_), \
  .layer = AI_NODE_OBJ(node_), \
  .compute = compute_, \
  .klass = AI_KLASS_OBJ(klass_), \
  .state = 0x0

#define AI_OPERATOR_SEQUENTIAL_FIELDS_DECLARE \
  AI_OPERATOR_COMMON_FIELDS_DECLARE  /*!< Operator common attributes */ \


#define AI_OPERATOR_SEQUENTIAL_INIT( \
          type_, compute_, next_, node_, tensors_, ...) { \
  AI_OPERATOR_COMMON_INIT(type_, compute_, next_, node_, NULL), \
  ## __VA_ARGS__ }

#define AI_OPERATOR_INIT(type_, node_, next_, compute_, ...) { \
  AI_OPERATOR_COMMON_INIT(type_, compute_, next_, node_, NULL), \
  ## __VA_ARGS__ }


AI_API_DECLARE_BEGIN

/*!
 * @defgroup operators_common Operators Common
 * @brief Implementation of the common operators datastructures
 * This header enumerates the operators specific definition implemented in the
 * library toghether with the macros and datatypes used to manipulate them.
 */

/*!
 * @enum ai_operator_type
 * @ingroup operators_common
 * @brief ai_tools supported operators type id
 */
typedef enum {
  AI_OPERATOR_CUSTOM_TYPE   = 0x0,   /*!< custom operator */
  AI_OPERATOR_CONV2D_TYPE,           /*!< 2D Convolutional operator */
  AI_OPERATOR_BN_TYPE,               /*!< Batch normalization operator */
  AI_OPERATOR_LRN_TYPE,              /*!< Local Response Normalization operator */
  AI_OPERATOR_NL_TYPE,               /*!< Nonlinearity operator */
  AI_OPERATOR_POOL_TYPE,             /*!< Pooling operator */
  AI_OPERATOR_SM_TYPE,               /*!< Softmax operator */
  AI_OPERATOR_SPLIT_TYPE,            /*!< Slice operator */
  AI_OPERATOR_ADD_TYPE,              /*!< Elementwise addition operator */
  AI_OPERATOR_PERMUTE_TYPE,          /*!< Permute Tensor operator */
  AI_OPERATOR_DENSE_TYPE,
  AI_OPERATOR_LSTM_TYPE,
  AI_OPERATOR_GRU_TYPE
} ai_operator_type;

/* forward types declarations */
struct ai_operator_;


/*!
 * @typedef void (*operator_compute_func)(struct ai_operator_* op, ai_float arg, ai_handle out)
 * @ingroup operators_common
 * @brief Callback signatures for all forward functions
 *
 */
typedef void (*operator_compute_func)(struct ai_operator_* op, ai_float arg,
                                      ai_handle out);

/*!
 * @struct ai_operator
 * @ingroup operators_common
 * @brief Structure encoding a operator in the network
 *
 * The operator struct includes information about the layer container it
 * belongs to, the next operator in the sequence and the compute function.
 * The compute functions are implemented in the @ref operators module.
 * An union stores the different parameters of the supported operator types.
 */
typedef AI_ALIGNED_TYPE(struct, 4) ai_operator_ {
  AI_OPERATOR_COMMON_FIELDS_DECLARE
} ai_operator;


/*!
 * @struct ai_operator_base
 * @ingroup operators_common
 * @brief Structure encoding a base operator in the network, it is a simple alias
 * for @ref ai_operator
 */
typedef ai_operator ai_operator_base;

/*!
 * @struct ai_operator_sequential
 * @ingroup operators_common
 * @brief Sequential operator type
 * Abstract datatype definition to cluster together all operator having a
 * single input tensor and a single output tensor.
 * see for example @ref ai_operator_sm, @ref ai_operator_nl
 */
typedef AI_ALIGNED_TYPE(struct, 4) ai_operator_sequential_ {
    AI_OPERATOR_COMMON_FIELDS_DECLARE
} ai_operator_sequential;


/*!
 * @brief Helper API to retrieve a human readable operator type from enum
 * @ingroup operators_common
 * @param type in type of operator
 * @return string defining the type of the operator
 */
AI_INTERNAL_API
const char* ai_operator_type_name(const ai_operator_type type);

AI_API_DECLARE_END

#endif /*OPERATORS_COMMON_H*/
