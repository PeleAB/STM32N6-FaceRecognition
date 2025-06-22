/**
  ******************************************************************************
  * @file    face_detection.h
  * @author  STEdgeAI
  * @date    2025-06-22 14:51:43
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
#ifndef LL_ATON_FACE_DETECTION_H
#define LL_ATON_FACE_DETECTION_H

/******************************************************************************/
#define LL_ATON_FACE_DETECTION_C_MODEL_NAME        "face_detection"
#define LL_ATON_FACE_DETECTION_ORIGIN_MODEL_NAME   "face_detection_front_128_integer_quant"

/************************** USER ALLOCATED IOs ********************************/
// No user allocated inputs
// No user allocated outputs

/************************** INPUTS ********************************************/
#define LL_ATON_FACE_DETECTION_IN_NUM        (1)    // Total number of input buffers
// Input buffer 1 -- Input_0_out_0
#define LL_ATON_FACE_DETECTION_IN_1_ALIGNMENT   (32)
#define LL_ATON_FACE_DETECTION_IN_1_SIZE_BYTES  (196608)

/************************** OUTPUTS *******************************************/
#define LL_ATON_FACE_DETECTION_OUT_NUM        (2)    // Total number of output buffers
// Output buffer 1 -- Transpose_259_out_0
#define LL_ATON_FACE_DETECTION_OUT_1_ALIGNMENT   (32)
#define LL_ATON_FACE_DETECTION_OUT_1_SIZE_BYTES  (3584)
// Output buffer 2 -- Transpose_239_out_0
#define LL_ATON_FACE_DETECTION_OUT_2_ALIGNMENT   (32)
#define LL_ATON_FACE_DETECTION_OUT_2_SIZE_BYTES  (57344)

#endif /* LL_ATON_FACE_DETECTION_H */
