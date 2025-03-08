/**
  ******************************************************************************
  * @file    c_functions_lib.h
  * @author  AIS
  * @brief   deprecated old fucntions list file
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
#ifndef _C_FUNCTIONS_LIB_H
#define _C_FUNCTIONS_LIB_H

#include "ai_platform.h"

AI_API_DECLARE_BEGIN

ai_i32 ailite_dense_f32(
	const ai_float *w, const ai_float *a, const ai_float *bias, const ai_i32 nk, const ai_i32 klen, ai_float *out);


void ailite_dense_f32_bn(const ai_float *w, const ai_float *a, const ai_float *bias, const ai_i32 nk, const ai_i32 klen,
    const ai_float *bna, const ai_float *bnb, ai_float *out);


void dense_f32_bn_outq_f32(const float *w, const float *a, const float *bias, const int nk, const int klen,
    const float *bna, const float *bnb, float *out);


void dense_f32_bn_outq_f32_bin(const uint32_t *w, const float *a, const float *bias, const int nk, const int klen,
    const float *bna, const float *bnb, float *out);


void dense_f32_bn_outq_bin(const uint32_t *w, const float *a, const float *bias, const int nk, const int klen,
    const float *bna, const float *bnb, uint32_t *out);


void dense_bn_bin(const uint32_t *w, const uint32_t  *a, const float *bias, const int nk, const int klen,
    const float *bna, const float *bnb, float *out);


void dense_bn_bin_no_outq(const uint32_t *w, const uint32_t  *a, const float *bias, const int nk, const int klen,
    const float *bna, const float *bnb, float *out);


/*
 * W0 = np.floor(bias - avg_mean + (avg_var / gamma * beta))
 */
void dense_bn_bin_bin_fast(
	const ai_u32 *w, const ai_u32  *a, const ai_i32 *W0, const ai_i32 nk, const ai_i32 klen,  ai_u32 *out);


void dense_bn_bin_bin(
	const ai_u32 *w, const ai_u32 *a, const ai_float *bias, const ai_i32 nk, const ai_i32 klen,
    const ai_float *bna, const ai_float *bnb, ai_u32 *out);

AI_API_DECLARE_END

#endif    /*_C_FUNCTIONS_LIB_H*/
