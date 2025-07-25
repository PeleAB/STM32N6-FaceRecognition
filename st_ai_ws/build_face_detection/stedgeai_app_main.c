/**
 ******************************************************************************
 * @file    stedgeai_app_main.c
 * @author  MCD/AIS Team
 * @brief   Minimal main template to use the STM AI generated c-model for n6
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2019,2021 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software is licensed under terms that can be found in the LICENSE file in
 * the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
#include <stdio.h>
#include <string.h>

#include "ll_aton.h"
#include "ll_aton_runtime.h"
#include "ai_platform.h"

LL_ATON_DECLARE_NAMED_NN_INSTANCE_AND_INTERFACE(face_detection) // Defines NN_Instance_face_detection and NN_Interface_face_detection with network.c info

AI_ALIGNED(32)
static ai_u8 activations_1[0];
AI_ALIGNED(32)
static ai_u8 activations_2[393216];
AI_ALIGNED(32)
static ai_u8 activations_3[98304];
AI_ALIGNED(32)
static ai_u8 activations_4[0];
AI_ALIGNED(32)
static ai_u8 activations_5[0];
AI_ALIGNED(32)
static ai_u8 activations_6[0];
AI_ALIGNED(32)
static ai_u8 activations_7[0];
AI_ALIGNED(32)
static ai_u8 activations_8[0];
AI_ALIGNED(32)
static ai_u8 activations_9[0];

AI_ALIGNED(32)
static ai_u8 weights_1[0];
AI_ALIGNED(32)
static ai_u8 weights_2[0];
AI_ALIGNED(32)
static ai_u8 weights_3[0];
AI_ALIGNED(32)
static ai_u8 weights_4[0];
AI_ALIGNED(32)
static ai_u8 weights_5[0];
AI_ALIGNED(32)
static ai_u8 weights_6[0];
AI_ALIGNED(32)
static ai_u8 weights_7[0];
AI_ALIGNED(32)
static ai_u8 weights_8[0];
AI_ALIGNED(32)
static ai_u8 weights_9[2455409];

/* 
 * Example of system initialization function
 */
void SystemInit(void)
{

}

/* 
 * Example of main application function
 */
int main(int argc, char* argv[])
{
  SystemInit();
  while(1)
  {
    LL_ATON_RT_Main(&NN_Instance_face_detection);
  }
}
