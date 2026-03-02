 /**
 ******************************************************************************
 * @file    main.c
 * @author  GPM Application Team
 *
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

#include <assert.h>

#include "app/app.h"
#include "bsp/platform.h"
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"

UART_HandleTypeDef huart1;

static StaticTask_t main_thread;
static StackType_t main_thread_stack[configMINIMAL_STACK_SIZE];

static int main_freertos(void);
static void main_thread_fct(void *arg);

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
  BSP_EarlyPlatformInit();

  return main_freertos();
}

static int main_freertos()
{
  TaskHandle_t hdl;

  hdl = xTaskCreateStatic(main_thread_fct, "main", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1,
                          main_thread_stack, &main_thread);
  assert(hdl != NULL);

  vTaskStartScheduler();
  assert(0);

  return -1;
}

static void main_thread_fct(void *arg)
{
  uint32_t preemptPriority;
  uint32_t subPriority;
  IRQn_Type i;

  /* Copy SysTick_IRQn priority set by RTOS and use it as default priorities for IRQs. We are now sure that all irqs
   * have default priority below or equal to configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY.
   */
  HAL_NVIC_GetPriority(SysTick_IRQn, HAL_NVIC_GetPriorityGrouping(), &preemptPriority, &subPriority);
  for (i = PVD_PVM_IRQn; i <= LTDC_UP_ERR_IRQn; i++)
    HAL_NVIC_SetPriority(i, preemptPriority, subPriority);

  BSP_PlatformInit();

  app_run();

  vTaskDelete(NULL);
}

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
  UNUSED(file);
  UNUSED(line);
  __BKPT(0);
  while (1)
  {
  }
}
#endif

/* Allow to debug with cache enable */
__attribute__ ((section (".keep_me"))) void app_clean_invalidate_dbg()
{
  SCB_CleanInvalidateDCache();
}

