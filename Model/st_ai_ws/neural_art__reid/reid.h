/**
  ******************************************************************************
  * @file    reid.h
  * @author  STEdgeAI
  * @date    2026-04-14 00:02:32
  * @brief   Minimal description of the generated c-implemention of the network
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  ******************************************************************************
  */
#ifndef LL_ATON_REID_H
#define LL_ATON_REID_H

/******************************************************************************/
#define LL_ATON_REID_C_MODEL_NAME        "reid"
#define LL_ATON_REID_ORIGIN_MODEL_NAME   "mobilenetv2_a100_256_128_fft_int8"

/************************** USER ALLOCATED IOs ********************************/
// No user allocated inputs
#define LL_ATON_REID_USER_ALLOCATED_OUTPUTS  (1)  // Number of output buffers not allocated by the compiler

/************************** INPUTS ********************************************/
#define LL_ATON_REID_IN_NUM        (1)    // Total number of input buffers
// Input buffer 1 -- Input_0_out_0
#define LL_ATON_REID_IN_1_ALIGNMENT   (32)
#define LL_ATON_REID_IN_1_SIZE_BYTES  (98304)

/************************** OUTPUTS *******************************************/
#define LL_ATON_REID_OUT_NUM        (1)    // Total number of output buffers
// Output buffer 1 -- Quantize_249_out_0
#define LL_ATON_REID_OUT_1_ALIGNMENT   (32)
#define LL_ATON_REID_OUT_1_SIZE_BYTES  (1280)

#endif /* LL_ATON_REID_H */
