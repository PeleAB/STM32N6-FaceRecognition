/**
 ******************************************************************************
 * @file    app_pipeline.c
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

#include "app/app_pipeline.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "app/app.h"
#include "app/app_config.h"
#include "app_postprocess.h"
#include "sysobj_cache.h"
#include "sysobj_camera.h"
#include "svc/app_display.h"
#include "svc/app_stats.h"
#include "svc/buffer_queue.h"
#include "svc/nn_service.h"
#include "isp_api.h"
#include "network.h"
#include "stm32n6xx_hal.h"
#include "utils.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#define FREERTOS_PRIORITY(p) ((UBaseType_t)((int)tskIDLE_PRIORITY + configMAX_PRIORITIES / 2 + (p)))

#define ALIGN_VALUE(_v_,_a_) (((_v_) + (_a_) - 1) & ~((_a_) - 1))

#define CAPTURE_BUFFER_NB (CAPTURE_DELAY + 2)
#define VENC_MAX_WIDTH 1280
#define VENC_MAX_HEIGHT 720

/* Model Related Info */
#define NN_INPUT_BUFFER_SIZE (NN_WIDTH * NN_HEIGHT * NN_BPP)
#define NN_OUTPUT_BUFFER_SIZE LL_ATON_DEFAULT_OUT_1_SIZE_BYTES
#define NN_OUTPUT_BUFFER_SIZE_ALIGN ALIGN_VALUE(NN_OUTPUT_BUFFER_SIZE, 32)

/* capture buffers */
static uint8_t capture_buffer[CAPTURE_BUFFER_NB][VENC_MAX_WIDTH * VENC_MAX_HEIGHT * CAPTURE_BPP] ALIGN_32 IN_PSRAM;
static int capture_buffer_disp_idx = 1;
static int capture_buffer_capt_idx = 0;

/* model */
LL_ATON_DECLARE_NAMED_NN_INSTANCE_AND_INTERFACE(Default);
static uint8_t nn_input_buffers[2][NN_INPUT_BUFFER_SIZE] ALIGN_32 IN_PSRAM;
static bqueue_t nn_input_queue;
static uint8_t nn_output_buffers[2][NN_OUTPUT_BUFFER_SIZE_ALIGN] ALIGN_32;
static bqueue_t nn_output_queue;
static nn_service_handle_t nn_model_handle = NN_SERVICE_INVALID_HANDLE;
static const nn_service_model_t *nn_model;

/* tasks */
static StaticTask_t nn_thread;
static StackType_t nn_thread_stack[2 * configMINIMAL_STACK_SIZE];
static StaticTask_t dp_thread;
static StackType_t dp_thread_stack[2 *configMINIMAL_STACK_SIZE];
static StaticTask_t isp_thread;
static StackType_t isp_thread_stack[2 *configMINIMAL_STACK_SIZE];
static SemaphoreHandle_t isp_sem;
static StaticSemaphore_t isp_sem_buffer;

static void app_main_pipe_frame_event(void)
{
  int next_disp_idx = (capture_buffer_disp_idx + 1) % CAPTURE_BUFFER_NB;
  int next_capt_idx = (capture_buffer_capt_idx + 1) % CAPTURE_BUFFER_NB;
  int ret;

  ret = CAM_DisplayPipe_UpdateAddress(capture_buffer[next_capt_idx]);
  assert(ret == 0);

  capture_buffer_disp_idx = next_disp_idx;
  capture_buffer_capt_idx = next_capt_idx;
}

static void app_ancillary_pipe_frame_event(void)
{
  uint8_t *next_buffer;
  int ret;

  next_buffer = bqueue_get_free(&nn_input_queue, 0);
  if (next_buffer) {
    ret = CAM_NNPipe_UpdateAddress(next_buffer);
    assert(ret == 0);
    bqueue_put_ready(&nn_input_queue);
  }
}

static void app_main_pipe_vsync_event(void)
{
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  int ret;

  ret = xSemaphoreGiveFromISR(isp_sem, &xHigherPriorityTaskWoken);
  if (ret == pdTRUE)
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static void nn_thread_fct(void *arg)
{
  stat_info_t *stats = app_stats_state();
  uint32_t nn_period_ms;
  uint32_t nn_period[2];
  uint8_t *nn_pipe_dst;
  uint32_t nn_out_len;
  uint32_t nn_in_len;
  uint32_t total_ts;
  uint32_t ts;
  int ret;

  (void) nn_period_ms;

  assert(nn_model);
  nn_in_len = nn_model->user_input_size;
  nn_out_len = nn_model->user_output_size;

  nn_period[1] = HAL_GetTick();

  nn_pipe_dst = bqueue_get_free(&nn_input_queue, 0);
  assert(nn_pipe_dst);
  CAM_NNPipe_Start(nn_pipe_dst, CMW_MODE_CONTINUOUS);
  while (1)
  {
    uint8_t *capture_buffer_local;
    uint8_t *output_buffer;

    nn_period[0] = nn_period[1];
    nn_period[1] = HAL_GetTick();
    nn_period_ms = nn_period[1] - nn_period[0];

    capture_buffer_local = bqueue_get_ready(&nn_input_queue);
    assert(capture_buffer_local);
    output_buffer = bqueue_get_free(&nn_output_queue, 1);
    assert(output_buffer);

    total_ts = HAL_GetTick();
    ts = HAL_GetTick();
    SYSOBJ_CacheInvalidate(output_buffer, nn_out_len);
    ret = nn_service_prepare_io(capture_buffer_local, nn_in_len, output_buffer, nn_out_len);
    assert(ret == NN_SERVICE_OK);
    Run_Inference(nn_model->instance);
    time_stat_update(&stats->nn_inference_time, HAL_GetTick() - ts);

    bqueue_put_free(&nn_input_queue);
    bqueue_put_ready(&nn_output_queue);

    time_stat_update(&stats->nn_total_time, HAL_GetTick() - total_ts);
  }
}

static void dp_thread_fct(void *arg)
{
  od_yolov2_pp_static_param_t pp_params;
  od_pp_out_t pp_output;
  stat_info_t *stats = app_stats_state();
  const nn_service_model_t *model = nn_model;
  uint32_t total_ts;
  void *pp_input;
  int is_dp_done;
  uint32_t ts;
  int ret;

  assert(model);
  app_postprocess_init(&pp_params, model->instance);
  while (1)
  {
    uint8_t *output_buffer;

    output_buffer = bqueue_get_ready(&nn_output_queue);
    assert(output_buffer);
    total_ts = HAL_GetTick();

    ts = HAL_GetTick();
    pp_input = (void *) output_buffer;
    pp_output.pOutBuff = NULL;
    ret = app_postprocess_run((void * []){pp_input}, 1, &pp_output, &pp_params);
    assert(ret == AI_OD_POSTPROCESS_ERROR_NO);
    time_stat_update(&stats->nn_pp_time, HAL_GetTick() - ts);
    app_stats_cpuload_update();

    is_dp_done = app_display_render(capture_buffer[capture_buffer_disp_idx], &pp_output);

    if (is_dp_done)
      time_stat_update(&stats->disp_total_time, HAL_GetTick() - total_ts);

    bqueue_put_free(&nn_output_queue);
  }
}

static void isp_thread_fct(void *arg)
{
  int ret;

  while (1) {
    ret = xSemaphoreTake(isp_sem, portMAX_DELAY);
    assert(ret == pdTRUE);

    CAM_IspUpdate();
  }
}

void app_pipeline_init(void)
{
  nn_service_model_cfg_t nn_cfg = {
    .name = "default",
    .instance = &NN_Instance_Default,
    .postprocess_type = POSTPROCESS_TYPE,
  };
  int ret;

  ret = nn_service_init();
  assert(ret == NN_SERVICE_OK);
  ret = nn_service_register(&nn_cfg, &nn_model_handle);
  assert(ret == NN_SERVICE_OK);
  ret = nn_service_select(nn_model_handle);
  assert(ret == NN_SERVICE_OK);
  nn_model = nn_service_active();
  assert(nn_model);
  assert(nn_model->user_input_size <= sizeof(nn_input_buffers[0]));
  assert(nn_model->user_output_size <= sizeof(nn_output_buffers[0]));
  assert(nn_model->postprocess_type == POSTPROCESS_TYPE);

  ret = bqueue_init(&nn_input_queue, 2, (uint8_t *[2]){nn_input_buffers[0], nn_input_buffers[1]});
  assert(ret == 0);
  ret = bqueue_init(&nn_output_queue, 2, (uint8_t *[2]){nn_output_buffers[0], nn_output_buffers[1]});
  assert(ret == 0);

  isp_sem = xSemaphoreCreateCountingStatic(1, 0, &isp_sem_buffer);
  assert(isp_sem);
}

void app_pipeline_start(void)
{
  UBaseType_t isp_priority = FREERTOS_PRIORITY(2);
  UBaseType_t dp_priority = FREERTOS_PRIORITY(-2);
  UBaseType_t nn_priority = FREERTOS_PRIORITY(1);
  TaskHandle_t hdl;

  CAM_DisplayPipe_Start(capture_buffer[0], CMW_MODE_CONTINUOUS);

  hdl = xTaskCreateStatic(nn_thread_fct, "nn", configMINIMAL_STACK_SIZE * 2, NULL, nn_priority, nn_thread_stack,
                          &nn_thread);
  assert(hdl != NULL);
  hdl = xTaskCreateStatic(dp_thread_fct, "dp", configMINIMAL_STACK_SIZE * 2, NULL, dp_priority, dp_thread_stack,
                          &dp_thread);
  assert(hdl != NULL);
  hdl = xTaskCreateStatic(isp_thread_fct, "isp", configMINIMAL_STACK_SIZE * 2, NULL, isp_priority, isp_thread_stack,
                          &isp_thread);
  assert(hdl != NULL);
}

int CMW_CAMERA_PIPE_FrameEventCallback(uint32_t pipe)
{
  if (pipe == DCMIPP_PIPE1)
    app_main_pipe_frame_event();
  else if (pipe == DCMIPP_PIPE2)
    app_ancillary_pipe_frame_event();

  return HAL_OK;
}

int CMW_CAMERA_PIPE_VsyncEventCallback(uint32_t pipe)
{
  if (pipe == DCMIPP_PIPE1)
    app_main_pipe_vsync_event();

  return HAL_OK;
}




