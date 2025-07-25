/**
  ******************************************************************************
  * @file    face_detection.h
  * @author  STEdgeAI
  * @date    2025-07-26 01:17:42
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
#define LL_ATON_FACE_DETECTION_ORIGIN_MODEL_NAME   "centerface"

/************************** USER ALLOCATED IOs ********************************/
// No user allocated inputs
// No user allocated outputs

/************************** INPUTS ********************************************/
#define LL_ATON_FACE_DETECTION_IN_NUM        (1)    // Total number of input buffers
// Input buffer 1 -- Input_0_out_0
#define LL_ATON_FACE_DETECTION_IN_1_ALIGNMENT   (32)
#define LL_ATON_FACE_DETECTION_IN_1_SIZE_BYTES  (196608)

/************************** OUTPUTS *******************************************/
#define LL_ATON_FACE_DETECTION_OUT_NUM        (4)    // Total number of output buffers
// Output buffer 1 -- Transpose_290_out_0
#define LL_ATON_FACE_DETECTION_OUT_1_ALIGNMENT   (32)
#define LL_ATON_FACE_DETECTION_OUT_1_SIZE_BYTES  (8192)
// Output buffer 2 -- Transpose_282_out_0
#define LL_ATON_FACE_DETECTION_OUT_2_ALIGNMENT   (32)
#define LL_ATON_FACE_DETECTION_OUT_2_SIZE_BYTES  (40960)
// Output buffer 3 -- Transpose_297_out_0
#define LL_ATON_FACE_DETECTION_OUT_3_ALIGNMENT   (32)
#define LL_ATON_FACE_DETECTION_OUT_3_SIZE_BYTES  (4096)
// Output buffer 4 -- Transpose_286_out_0
#define LL_ATON_FACE_DETECTION_OUT_4_ALIGNMENT   (32)
#define LL_ATON_FACE_DETECTION_OUT_4_SIZE_BYTES  (8192)

#endif /* LL_ATON_FACE_DETECTION_H */
