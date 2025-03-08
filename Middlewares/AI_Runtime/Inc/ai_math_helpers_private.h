/**
  ******************************************************************************
  * @file    ai_math_helpers_private.h
  * @author  AST Embedded Analytics Research Platform
  * @brief   Math helpers routines header file.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  
  ******************************************************************************
  */
#ifndef AI_MATH_HELPERS_PRIVATE_H
#define AI_MATH_HELPERS_PRIVATE_H
#include "ai_lite_math_helpers.h"

__STATIC_FORCEINLINE ai_i32 Extract_fxp_format_from_float(ai_float align_factor_f)
{
#if defined (__GNUC__)
    #pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif
    /* Get 32 bit description of float value */
    ai_i32  align_factor_u32 = *((ai_i32 *)(&align_factor_f));

    /* Retrieve signed 8 bit exponent value (bits 23 to 30) */
    return(126 - ((align_factor_u32 & 0x7f800000) >> 23));
}
void ai_math_elementwise_sum_int8( ai_i8 * pOut, const ai_i8 * pIn1, const ai_i8 * pIn2, const ai_size loop,
                              const ai_float scale1, const ai_i8 zp1,
                              const ai_float scale2, const ai_i8 zp2,
                              const ai_float scaleout, const ai_i8 zpout);
void ai_math_elementwise_mul_int8( ai_i8 * pOut, const ai_i8 * pIn1, const ai_i8 * pIn2, const ai_size loop,
                              const ai_float scale1, const ai_i8 zp1,
                              const ai_float scale2, const ai_i8 zp2,
                              const ai_float scaleout, const ai_i8 zpout);


#endif /* AI_MATH_HELPERS_PRIVATE_H */
