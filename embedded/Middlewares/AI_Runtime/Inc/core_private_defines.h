/**
  ******************************************************************************
  * @file    core_private_defines.h
  * @author  AST Embedded Analytics Research Platform
  * @brief   private header file of common private core module defines
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

#ifndef CORE_PRIVATE_DEFINES_H
#define CORE_PRIVATE_DEFINES_H
#include "core_assert.h"

/*****************************************************************************/
#define CORE_ASSERT_QUANTIZATION_PARAMS(in_scale, out_scale, w_scale, w_count) { \
  CORE_ASSERT((in_scale) > 0) \
  CORE_ASSERT((out_scale) > 0) \
  CORE_ASSERT((w_count) > 0) \
  CORE_ASSERT(w_scale) \
}


#endif    /* CORE_PRIVATE_DEFINES_H */
