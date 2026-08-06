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
#include "app/app_config.h"
#include "app/app_pipeline.h"
#include "bsp/platform.h"
#include "stm32n6570_discovery.h"
#include "svc/app_stats.h"
#include "svc/face_gallery.h"
#include "sysobj/inc/sysobj_uart.h"
#include "sysobj_params.h"
#include "task.h"
#include "utils.h"

UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_usart1_rx;

void HAL_UART_MspInit(UART_HandleTypeDef *huart) {
  if (huart->Instance == USART1) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    /* NVIC for USART1 to catch RXNE/IDLE/Error */
    HAL_NVIC_SetPriority(USART1_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
  }
}

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

void sysobj_uart_handle_manage_telemetry(uint8_t src_id) {
  stat_info_t copy;
  stat_info_copy(&copy);

  float cpu_load_last;
  app_stats_cpuload_get(&cpu_load_last, NULL, NULL);

  uint8_t payload[4];

  /* CPU Load: 0-100%, fits in 1 uint8 */
  payload[0] = (uint8_t)(cpu_load_last);

  /* Inference time (ms, up to 65535, typical is around tens of ms) */
  uint16_t inf_time = (uint16_t)copy.nn_inference_time.last;
  payload[1] = (uint8_t)(inf_time & 0xFF);
  payload[2] = (uint8_t)((inf_time >> 8) & 0xFF);

  /* Object detection count (fits in 1 uint8) */
  payload[3] = (uint8_t)(copy.nb_detect & 0xFF);

  sysobj_uart_msg_t msg;
  msg.src_id = 0x02; /* MCU */
  msg.dst_id = src_id;
  msg.is_ack = 0;
  msg.need_ack = 0; /* Reply itself is the implicit ack to their query */
  msg.msg_type = SYSOBJ_UART_MSG_TYPE_MANAGE;
  msg.msg_subtype = SYSOBJ_UART_MANAGE_SUBTYPE_TELEMETRY;
  msg.data = payload;
  msg.data_len = sizeof(payload);

  sysobj_uart_send(&msg);
}

/* ---------------------------------------------------------------------------
 * UART CONFIG handlers — persistent parameter read/write
 * ------------------------------------------------------------------------- */

void sysobj_uart_handle_config_param_read(uint8_t src_id, uint16_t param_id)
{
  uint64_t value       = 0;
  bool     was_default = false;
  params_status_t st   = sysobj_params_read(param_id, &value, &was_default);

  /* All current params are U32 */
  uint8_t type = (uint8_t)PARAM_TYPE_U32;

  /* Build response payload (17 bytes) */
  uint8_t payload[17];
  payload[0]  = (uint8_t)st;
  payload[1]  = (uint8_t)(param_id & 0xFFU);
  payload[2]  = (uint8_t)((param_id >> 8) & 0xFFU);
  payload[3]  = type;
  for (int i = 0; i < 8; i++)
    payload[4 + i] = (uint8_t)((value >> (8 * i)) & 0xFFU);

  /* Recalculate entry CRC the same way sysobj_params does internally */
  uint8_t crc_input[11];
  crc_input[0] = payload[1]; crc_input[1] = payload[2]; /* id LE */
  crc_input[2] = type;
  for (int i = 0; i < 8; i++) crc_input[3 + i] = payload[4 + i];
  uint32_t crc = sysobj_uart_calculate_crc32(crc_input, sizeof(crc_input));
  payload[12] = (uint8_t)(crc & 0xFFU);
  payload[13] = (uint8_t)((crc >> 8) & 0xFFU);
  payload[14] = (uint8_t)((crc >> 16) & 0xFFU);
  payload[15] = (uint8_t)((crc >> 24) & 0xFFU);
  payload[16] = was_default ? 1U : 0U;

  sysobj_uart_msg_t msg = {
    .src_id      = 0x02, /* MCU */
    .dst_id      = src_id,
    .is_ack      = 0,
    .need_ack    = 0,
    .msg_type    = SYSOBJ_UART_MSG_TYPE_CONFIG,
    .msg_subtype = SYSOBJ_UART_CONFIG_SUBTYPE_PARAM_READ,
    .data        = payload,
    .data_len    = sizeof(payload),
  };
  sysobj_uart_send(&msg);
}

void sysobj_uart_handle_config_param_write(uint8_t src_id, uint16_t param_id,
                                            uint8_t type, uint64_t value)
{
  (void)type; /* type is informational; sysobj_params uses the table type */
  params_status_t st = sysobj_params_write(param_id, value);

  /* Debug input selection is a live control.  Apply a valid value even if
   * persistence fails; the response still reports the flash error. */
  if (param_id == PARAM_FACE_INPUT_MODE && value <= FACE_INPUT_STATIC)
    app_pipeline_set_face_input_mode((uint32_t)value);

  uint8_t payload[3];
  payload[0] = (uint8_t)st;
  payload[1] = (uint8_t)(param_id & 0xFFU);
  payload[2] = (uint8_t)((param_id >> 8) & 0xFFU);

  sysobj_uart_msg_t msg = {
    .src_id      = 0x02, /* MCU */
    .dst_id      = src_id,
    .is_ack      = 0,
    .need_ack    = 0,
    .msg_type    = SYSOBJ_UART_MSG_TYPE_CONFIG,
    .msg_subtype = SYSOBJ_UART_CONFIG_SUBTYPE_PARAM_WRITE,
    .data        = payload,
    .data_len    = sizeof(payload),
  };
  sysobj_uart_send(&msg);
}

static void gallery_send(uint8_t dst_id, uint8_t subtype,
                         const uint8_t *payload, uint8_t payload_len)
{
  sysobj_uart_msg_t msg = {
    .src_id = 0x02, .dst_id = dst_id, .is_ack = 0, .need_ack = 0,
    .msg_type = SYSOBJ_UART_MSG_TYPE_CONFIG, .msg_subtype = subtype,
    .data = payload, .data_len = payload_len,
  };
  (void)sysobj_uart_send(&msg);
}

static void gallery_send_status(uint8_t dst_id, uint8_t subtype,
                                face_gallery_status_t result)
{
  face_gallery_enroll_status_t status;
  FaceGallery_GetStatus(&status);
  uint8_t name_len = (uint8_t)strlen(status.name);
  uint8_t payload[6 + FACE_GALLERY_NAME_MAX];
  payload[0] = (uint8_t)result;
  payload[1] = status.active;
  payload[2] = status.samples;
  payload[3] = status.required_samples;
  payload[4] = status.count;
  payload[5] = name_len;
  memcpy(&payload[6], status.name, name_len);
  gallery_send(dst_id, subtype, payload, (uint8_t)(6 + name_len));
}

void sysobj_uart_handle_config_enroll(uint8_t src_id, const uint8_t *name,
                                      uint8_t name_len)
{
  face_gallery_status_t st = FaceGallery_StartEnrollment((const char *)name,
                                                          name_len);
  gallery_send_status(src_id, SYSOBJ_UART_CONFIG_SUBTYPE_ENROLL, st);
}

void sysobj_uart_handle_config_commit_enroll(uint8_t src_id)
{
  face_gallery_status_t st = app_pipeline_gallery_commit();
  gallery_send_status(src_id, SYSOBJ_UART_CONFIG_SUBTYPE_COMMIT_ENROLL, st);
}

void sysobj_uart_handle_config_clear_embeddings(uint8_t src_id)
{
  face_gallery_status_t st = app_pipeline_gallery_clear();
  gallery_send_status(src_id, SYSOBJ_UART_CONFIG_SUBTYPE_CLEAR_EMBEDDINGS, st);
}

void sysobj_uart_handle_config_gallery_status(uint8_t src_id)
{
  gallery_send_status(src_id, SYSOBJ_UART_CONFIG_SUBTYPE_GALLERY_STATUS,
                      FACE_GALLERY_OK);
}

void sysobj_uart_handle_config_gallery_list(uint8_t src_id)
{
  uint8_t payload[2 + FACE_GALLERY_MAX_ENTRIES *
                        (2 + FACE_GALLERY_NAME_MAX)];
  uint8_t pos = 2;
  uint8_t count = 0;
  payload[0] = FACE_GALLERY_OK;
  for (uint8_t slot = 0; slot < FACE_GALLERY_MAX_ENTRIES; slot++) {
    char name[FACE_GALLERY_NAME_MAX + 1];
    if (FaceGallery_GetName(slot, name, sizeof(name)) != 0) continue;
    uint8_t len = (uint8_t)strlen(name);
    payload[pos++] = slot;
    payload[pos++] = len;
    memcpy(&payload[pos], name, len);
    pos += len;
    count++;
  }
  payload[1] = count;
  gallery_send(src_id, SYSOBJ_UART_CONFIG_SUBTYPE_GALLERY_LIST, payload, pos);
}

void sysobj_uart_handle_config_gallery_delete(uint8_t src_id, uint8_t slot)
{
  face_gallery_status_t st = app_pipeline_gallery_delete(slot);
  uint8_t payload[3] = {(uint8_t)st, slot, FaceGallery_Count()};
  gallery_send(src_id, SYSOBJ_UART_CONFIG_SUBTYPE_GALLERY_DELETE, payload,
               sizeof(payload));
}

void sysobj_uart_handle_config_gallery_import_q7(uint8_t src_id,
                                                 const uint8_t *data,
                                                 uint8_t data_len)
{
  face_gallery_status_t st = FACE_GALLERY_ERR_NAME;
  if (data != NULL && data_len >= 1) {
    uint8_t name_len = data[0];
    if (name_len > 0 && name_len <= FACE_GALLERY_NAME_MAX &&
        data_len == (uint8_t)(1 + name_len + FACE_GALLERY_EMBEDDING_SIZE)) {
      st = app_pipeline_gallery_import_q7(
          (const char *)&data[1], name_len,
          (const int8_t *)&data[1 + name_len]);
    }
  }
  gallery_send_status(src_id, SYSOBJ_UART_CONFIG_SUBTYPE_GALLERY_IMPORT_Q7, st);
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

  /* A debugger RAM launch is not guaranteed to reset NVIC external-interrupt
   * state. Do not inherit enabled/pending IRQs from ROM, a previous image, or
   * an interrupted debug session. BSP/HAL initialization below re-enables
   * every interrupt the application actually uses. */
  for (uint32_t bank = 0; bank <
       (sizeof(NVIC->ICER) / sizeof(NVIC->ICER[0])); bank++) {
    NVIC->ICER[bank] = 0xffffffffUL;
    NVIC->ICPR[bank] = 0xffffffffUL;
  }
  __DSB();
  __ISB();

  HAL_NVIC_GetPriority(SysTick_IRQn, HAL_NVIC_GetPriorityGrouping(),
                       &preemptPriority, &subPriority);
  for (i = PVD_PVM_IRQn; i <= LTDC_UP_ERR_IRQn; i++)
    HAL_NVIC_SetPriority(i, preemptPriority, subPriority);

  BSP_PlatformInit();

  sysobj_uart_init(&huart1);

  /* Use larger stack for UART task just in case */
  xTaskCreate(sysobj_uart_task_func, "sysobj_uart",
              configMINIMAL_STACK_SIZE * 2, NULL, tskIDLE_PRIORITY + 2, NULL);

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
