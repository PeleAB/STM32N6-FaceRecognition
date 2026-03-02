/**
 ******************************************************************************
 * @file    freertos_platform.c
 * @author  GPM Application Team
 *
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

#include <assert.h>
#include <stdint.h>

#include "bsp/freertos_platform.h"

#include "cmsis_compiler.h"
#include "FreeRTOS.h"
#include "task.h"

#define IS_IRQ_MODE()     (__get_IPSR() != 0U)

static TIM_HandleTypeDef tim4;

uint32_t HAL_GetTick()
{
  return xTaskGetTickCount() * portTICK_PERIOD_MS;
}

void HAL_Delay(uint32_t Delay)
{
  if (IS_IRQ_MODE())
    assert(0);

  vTaskDelay(Delay / portTICK_PERIOD_MS);
}

HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
  (void) TickPriority;

  return HAL_OK;
}

void TIM4_Config(void)
{
  const uint32_t tmr_clk_freq = 100000;
  int ret;

  __HAL_RCC_TIM4_CLK_ENABLE();

  tim4.Instance = TIM4;
  tim4.Init.Period = ~0;
  tim4.Init.Prescaler = (HAL_RCC_GetPCLK1Freq() / tmr_clk_freq) - 1;
  tim4.Init.ClockDivision = 0;
  tim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  ret = HAL_TIM_Base_Init(&tim4);
  assert(ret == HAL_OK);

  ret = HAL_TIM_Base_Start(&tim4);
  assert(ret == HAL_OK);
}

uint32_t TIM4_Get_Value(void)
{
  return __HAL_TIM_GET_COUNTER(&tim4);
}

#if (configSUPPORT_STATIC_ALLOCATION == 1)
void vApplicationGetIdleTaskMemory (StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize) {
  /* Idle task control block and stack */
  static StaticTask_t Idle_TCB;
  static StackType_t  Idle_Stack[configMINIMAL_STACK_SIZE];

  *ppxIdleTaskTCBBuffer   = &Idle_TCB;
  *ppxIdleTaskStackBuffer = &Idle_Stack[0];
  *pulIdleTaskStackSize   = (uint32_t)configMINIMAL_STACK_SIZE;
}

void vApplicationGetTimerTaskMemory (StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize) {
  /* Timer task control block and stack */
  static StaticTask_t Timer_TCB;
  static StackType_t  Timer_Stack[configTIMER_TASK_STACK_DEPTH];

  *ppxTimerTaskTCBBuffer   = &Timer_TCB;
  *ppxTimerTaskStackBuffer = &Timer_Stack[0];
  *pulTimerTaskStackSize   = (uint32_t)configTIMER_TASK_STACK_DEPTH;
}
#endif

