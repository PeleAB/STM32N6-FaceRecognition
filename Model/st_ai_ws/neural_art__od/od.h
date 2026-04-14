/**
  ******************************************************************************
  * @file    od.h
  * @author  STEdgeAI
  * @date    2026-04-14 00:02:22
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
#ifndef LL_ATON_OD_H
#define LL_ATON_OD_H

/******************************************************************************/
#define LL_ATON_OD_C_MODEL_NAME        "od"
#define LL_ATON_OD_ORIGIN_MODEL_NAME   "quantized_tiny_yolo_v2_224_"

/************************** USER ALLOCATED IOs ********************************/
#define LL_ATON_OD_USER_ALLOCATED_INPUTS   (1)  // Number of input buffers not allocated by the compiler
// No user allocated outputs

/************************** INPUTS ********************************************/
#define LL_ATON_OD_IN_NUM        (1)    // Total number of input buffers
// Input buffer 1 -- Input_0_out_0
#define LL_ATON_OD_IN_1_ALIGNMENT   (32)
#define LL_ATON_OD_IN_1_SIZE_BYTES  (150528)

/************************** OUTPUTS *******************************************/
#define LL_ATON_OD_OUT_NUM        (1)    // Total number of output buffers
// Output buffer 1 -- Transpose_54_out_0
#define LL_ATON_OD_OUT_1_ALIGNMENT   (32)
#define LL_ATON_OD_OUT_1_SIZE_BYTES  (1470)

#endif /* LL_ATON_OD_H */
