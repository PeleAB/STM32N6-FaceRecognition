/**
  ******************************************************************************
  * @file    face_recognition.h
  * @author  STEdgeAI
  * @date    2025-07-26 01:17:26
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
#ifndef LL_ATON_FACE_RECOGNITION_H
#define LL_ATON_FACE_RECOGNITION_H

/******************************************************************************/
#define LL_ATON_FACE_RECOGNITION_C_MODEL_NAME        "face_recognition"
#define LL_ATON_FACE_RECOGNITION_ORIGIN_MODEL_NAME   "mobilefacenet_int8_faces"

/************************** USER ALLOCATED IOs ********************************/
// No user allocated inputs
// No user allocated outputs

/************************** INPUTS ********************************************/
#define LL_ATON_FACE_RECOGNITION_IN_NUM        (1)    // Total number of input buffers
// Input buffer 1 -- Input_0_out_0
#define LL_ATON_FACE_RECOGNITION_IN_1_ALIGNMENT   (32)
#define LL_ATON_FACE_RECOGNITION_IN_1_SIZE_BYTES  (150528)

/************************** OUTPUTS *******************************************/
#define LL_ATON_FACE_RECOGNITION_OUT_NUM        (1)    // Total number of output buffers
// Output buffer 1 -- BatchNormalization_289_out_0
#define LL_ATON_FACE_RECOGNITION_OUT_1_ALIGNMENT   (32)
#define LL_ATON_FACE_RECOGNITION_OUT_1_SIZE_BYTES  (512)

#endif /* LL_ATON_FACE_RECOGNITION_H */
