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

#include "main.h"
#include "FreeRTOS.h"
#include "app/app.h"
#include "bsp/platform.h"
#include "stm32n6570_discovery.h"
#include "sysobj/inc/sysobj_uart.h"
#include "task.h"

UART_HandleTypeDef huart1;

static StaticTask_t main_thread;
static StackType_t main_thread_stack[configMINIMAL_STACK_SIZE * 2];

static int main_freertos(void);
static void main_thread_fct(void *arg);

void sysobj_uart_handle_manage_set_led(uint8_t led_id, uint8_t state) {
  /* Board has GREEN and RED LEDs */
  if (led_id == 1) {
    if (state)
      BSP_LED_On(LED_GREEN);
    else
      BSP_LED_Off(LED_GREEN);
  } else if (led_id == 2) {
    if (state)
      BSP_LED_On(LED_RED);
    else
      BSP_LED_Off(LED_RED);
  }
}

static uint8_t g_rx_buffer[SYSOBJ_UART_MAX_PAYLOAD_SIZE + 12];
static uint16_t g_rx_idx = 0;

static void uart_rx_task(void *pvParameters) {
  (void)pvParameters;
  uint8_t rx_byte;
  sysobj_uart_msg_t msg;

  while (1) {
    /* Poll with 0 timeout to avoid blocking other tasks that might need UART
     * Transmit */
    while (HAL_UART_Receive(&huart1, &rx_byte, 1, 0) == HAL_OK) {
      if (g_rx_idx == 0 && rx_byte != SYSOBJ_UART_SOF) {
        continue;
      }

      g_rx_buffer[g_rx_idx++] = rx_byte;

      if (g_rx_idx >= 2) {
        uint8_t payload_len = g_rx_buffer[1];
        uint16_t total_expected = 3 + payload_len + 4;

        if (g_rx_idx >= total_expected) {
          if (sysobj_uart_parse(g_rx_buffer, g_rx_idx, &msg) ==
              SYSOBJ_UART_ERROR_NONE) {
            sysobj_uart_dispatch_msg(&msg);
          }
          g_rx_idx = 0;
        } else if (g_rx_idx >= sizeof(g_rx_buffer)) {
          g_rx_idx = 0;
        }
      }
    }
    /* Yield to other tasks */
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

/**
 * @brief  Main program
 * @param  None
 * @retval None
 */
int main(void) {
  BSP_EarlyPlatformInit();
  return main_freertos();
}

static int main_freertos() {
  TaskHandle_t hdl;
  hdl = xTaskCreateStatic(
      main_thread_fct, "main", sizeof(main_thread_stack) / sizeof(StackType_t),
      NULL, tskIDLE_PRIORITY + 1, main_thread_stack, &main_thread);
  configASSERT(hdl != NULL);

  vTaskStartScheduler();
  configASSERT(0);
  return -1;
}

static void main_thread_fct(void *arg) {
  uint32_t preemptPriority;
  uint32_t subPriority;
  IRQn_Type i;

  HAL_NVIC_GetPriority(SysTick_IRQn, HAL_NVIC_GetPriorityGrouping(),
                       &preemptPriority, &subPriority);
  for (i = PVD_PVM_IRQn; i <= LTDC_UP_ERR_IRQn; i++)
    HAL_NVIC_SetPriority(i, preemptPriority, subPriority);

  BSP_PlatformInit();

  /* Use larger stack for UART task just in case */
  xTaskCreate(uart_rx_task, "uart_rx", configMINIMAL_STACK_SIZE * 2, NULL,
              tskIDLE_PRIORITY + 2, NULL);

  app_run();
  vTaskDelete(NULL);
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) {
  UNUSED(file);
  UNUSED(line);
  __BKPT(0);
  while (1) {
  }
}
#endif

__attribute__((section(".keep_me"))) void app_clean_invalidate_dbg() {
  SCB_CleanInvalidateDCache();
}
