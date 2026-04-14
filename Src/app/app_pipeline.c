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
#include "sysobj_params.h"
#include "app_postprocess.h"
#include "sysobj_cache.h"
#include "sysobj_camera.h"
#include "svc/app_display.h"
#include "svc/app_stats.h"
#include "svc/buffer_queue.h"
#include "stai_od.h"
#include "stai_reid.h"
#include "stm32ipl.h"
#include "svc/app_trackobject.h"
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
#define NN_OD_INPUT_SIZE    STAI_OD_IN_1_SIZE       /* 150528 (224x224x3) */
#define NN_OD_OUTPUT_SIZE   STAI_OD_OUT_1_SIZE      /* 1470 (7x7x30 int8) */
#define NN_REID_INPUT_SIZE  STAI_REID_IN_1_SIZE      /* 98304 (256x128x3) */
#define NN_REID_OUTPUT_SIZE STAI_REID_OUT_1_SIZE      /* 1280 (feature vector) */

#define NN_INPUT_BUFFER_SIZE  NN_OD_INPUT_SIZE      /* camera pipe delivers od-sized frames */

/* Frame passed from nn_thread to dp_thread via nn_output_queue */
typedef struct {
  od_pp_out_t       pp_out;
  od_pp_outBuffer_t boxes[APP_MAX_OBJECT_DETECT];
} nn_dp_frame_t;

#define NN_DP_FRAME_SIZE ALIGN_VALUE(sizeof(nn_dp_frame_t), 32)

/* capture buffers */
static uint8_t capture_buffer[CAPTURE_BUFFER_NB][VENC_MAX_WIDTH * VENC_MAX_HEIGHT * CAPTURE_BPP] ALIGN_32 IN_PSRAM;
static int capture_buffer_disp_idx = 1;
static int capture_buffer_capt_idx = 0;

/* model */
/* STAI network contexts (statically allocated) */
STAI_NETWORK_CONTEXT_DECLARE(network_od_ctx, STAI_OD_CONTEXT_SIZE);
STAI_NETWORK_CONTEXT_DECLARE(network_reid_ctx, STAI_REID_CONTEXT_SIZE);

static uint8_t nn_input_buffers[2][NN_INPUT_BUFFER_SIZE] ALIGN_32 IN_PSRAM;
static bqueue_t nn_input_queue;
static uint8_t nn_output_buffers[2][NN_DP_FRAME_SIZE] ALIGN_32;
static bqueue_t nn_output_queue;

/* STM32Ipl memory pool */
static uint8_t ipl_mem_pool[64 * 1024] ALIGN_32;

/* tasks */
static StaticTask_t nn_thread;
static StackType_t nn_thread_stack[2 * configMINIMAL_STACK_SIZE];
static StaticTask_t dp_thread;
static StackType_t dp_thread_stack[2 *configMINIMAL_STACK_SIZE];
static StaticTask_t isp_thread;
static StackType_t isp_thread_stack[2 *configMINIMAL_STACK_SIZE];
static SemaphoreHandle_t isp_sem;
static StaticSemaphore_t isp_sem_buffer;

/* ---------------------------------------------------------------------------
 * Inference guard mutex — held while Run_Inference() is executing.
 * params pre/post_write_hook take this mutex to ensure the NPU is not
 * accessing NOR flash when MMP mode is disabled for erase/write.
 * ------------------------------------------------------------------------- */
static SemaphoreHandle_t s_inference_mutex;
static StaticSemaphore_t s_inference_mutex_buf;
static uint32_t s_interframe_ms = 33;
static od_yolov2_pp_static_param_t s_od_pp_params = {
  .conf_threshold = 0.5f,
  .iou_threshold = 0.45f,
  .pAnchors = AI_OD_YOLOV2_PP_ANCHORS,
  .nb_anchors = AI_OD_YOLOV2_PP_NB_ANCHORS,
  .nb_classes = AI_OD_YOLOV2_PP_NB_CLASSES,
};

static void params_pre_write_hook(void)
{
  /* Block until any running inference releases the mutex */
  xSemaphoreTake(s_inference_mutex, portMAX_DELAY);
}

static void params_post_write_hook(void)
{
  xSemaphoreGive(s_inference_mutex);
}

static void Run_Inference(stai_network *ctx,
                          stai_return_code (*run)(stai_network *, const stai_run_mode),
                          stai_return_code (*new_inf)(stai_network *))
{
  stai_return_code ret;
  do {
    ret = run(ctx, STAI_MODE_ASYNC);
    if (ret == STAI_RUNNING_WFE)
      LL_ATON_OSAL_WFE();
  } while (ret == STAI_RUNNING_WFE || ret == STAI_RUNNING_NO_WFE);

  ret = new_inf(ctx);
  assert(ret == STAI_SUCCESS);
}

static void app_main_pipe_frame_event(void)
{
  int next_disp_idx = (capture_buffer_disp_idx + 1) % CAPTURE_BUFFER_NB;
  int next_capt_idx = (capture_buffer_capt_idx + 1) % CAPTURE_BUFFER_NB;
  int ret;

  ret = CAM_DisplayPipe_UpdateAddress(capture_buffer[next_capt_idx]);
  assert(ret == 0);
  (void)ret;

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
    (void)ret;
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
  (void)ret;
}

static void dbox_to_roi(const od_pp_outBuffer_t *dbox, rectangle_t *roi,
                        int width, int height)
{
  int xc = (int)(dbox->x_center * width);
  int yc = (int)(dbox->y_center * height);
  int w  = (int)(dbox->width * width);
  int h  = (int)(dbox->height * height);
  int x0 = MAX(0, xc - w / 2);
  int y0 = MAX(0, yc - h / 2);
  roi->x = (int16_t)x0;
  roi->y = (int16_t)y0;
  roi->w = (int16_t)MIN(w, width - x0);
  roi->h = (int16_t)MIN(h, height - y0);
}

static void nn_thread_fct(void *arg)
{
  static od_pp_outBuffer_t nn_od_boxes[APP_MAX_OBJECT_DETECT];
  static uint8_t reid_features_all[APP_MAX_OBJECT_DETECT][NN_REID_OUTPUT_SIZE] ALIGN_32;
  static uint8_t od_raw_buf[NN_OD_OUTPUT_SIZE] ALIGN_32;
  stat_info_t *stats = app_stats_state();
  uint32_t nn_period[2];
  uint32_t total_ts, ts;
  uint32_t nn_period_ms;
  (void)nn_period_ms;

  nn_period[1] = HAL_GetTick();

  uint8_t *nn_pipe_dst = bqueue_get_free(&nn_input_queue, 0);
  assert(nn_pipe_dst);
  CAM_NNPipe_Start(nn_pipe_dst, CMW_MODE_CONTINUOUS);

  /* Get reid input buffer — statically allocated by model, address is fixed */
  stai_ptr reid_input_ptrs[1];
  stai_size n_reid_in;
  stai_reid_get_inputs(network_reid_ctx, reid_input_ptrs, &n_reid_in);
  uint8_t *reid_input_buf = (uint8_t *)reid_input_ptrs[0];

  while (1)
  {
    uint8_t *capture_buffer_local;
    uint8_t *output_buffer;
    nn_dp_frame_t *frame;
    od_pp_out_t pp_out;

    nn_period[0] = nn_period[1];
    nn_period[1] = HAL_GetTick();
    nn_period_ms = nn_period[1] - nn_period[0];

    capture_buffer_local = bqueue_get_ready(&nn_input_queue);
    assert(capture_buffer_local);
    output_buffer = bqueue_get_free(&nn_output_queue, 1);
    assert(output_buffer);
    frame = (nn_dp_frame_t *)output_buffer;

    total_ts = HAL_GetTick();
    ts = HAL_GetTick();

    /* 1. OD inference */
    stai_ptr od_inputs[1] = { capture_buffer_local };
    stai_od_set_inputs(network_od_ctx, od_inputs, 1);
    xSemaphoreTake(s_inference_mutex, portMAX_DELAY);
    Run_Inference(network_od_ctx, stai_od_run, stai_ext_od_new_inference);
    xSemaphoreGive(s_inference_mutex);

    /* Copy OD output before reid inference overwrites NPU AXISRAM */
    stai_ptr od_outputs[STAI_OD_OUT_NUM];
    stai_size n_od_out;
    stai_od_get_outputs(network_od_ctx, od_outputs, &n_od_out);
    SYSOBJ_CacheInvalidate(od_outputs[0], NN_OD_OUTPUT_SIZE);
    memcpy(od_raw_buf, od_outputs[0], NN_OD_OUTPUT_SIZE);

    /* 2. OD postprocess inline */
    pp_out.pOutBuff = nn_od_boxes;
    pp_out.nb_detect = 0;
    (void)app_postprocess_run((void *[]){ od_raw_buf }, 1, &pp_out, &s_od_pp_params);

    /* 3. Reid per detected object */
    int nb_reid = MIN(pp_out.nb_detect, APP_MAX_OBJECT_DETECT);
    for (int i = 0; i < nb_reid; i++) {
      image_t src_img, dst_img;
      rectangle_t roi;

      dbox_to_roi(&pp_out.pOutBuff[i], &roi, NN_WIDTH, NN_HEIGHT);
      STM32Ipl_Init(&src_img, NN_WIDTH, NN_HEIGHT, IMAGE_BPP_RGB888,
                    capture_buffer_local);
      STM32Ipl_Init(&dst_img, STAI_REID_IN_1_WIDTH, STAI_REID_IN_1_HEIGHT,
                    IMAGE_BPP_RGB888, reid_input_buf);
      STM32Ipl_Resize_Roi(&src_img, &roi, &dst_img, NULL, RESIZE_BILINEAR);
      SCB_CleanDCache_by_Addr((uint32_t *)reid_input_buf,
                              ALIGN_VALUE(NN_REID_INPUT_SIZE, 32));

      stai_ptr reid_out[1] = { reid_features_all[i] };
      stai_reid_set_outputs(network_reid_ctx, reid_out, 1);
      xSemaphoreTake(s_inference_mutex, portMAX_DELAY);
      Run_Inference(network_reid_ctx, stai_reid_run, stai_ext_reid_new_inference);
      xSemaphoreGive(s_inference_mutex);
      SYSOBJ_CacheInvalidate(reid_features_all[i], NN_REID_OUTPUT_SIZE);
    }

    /* 4. Update tracking */
    TrackObject_UpdateAll(&pp_out, reid_features_all);

    time_stat_update(&stats->nn_inference_time, HAL_GetTick() - ts);

    /* Hand off postprocessed frame to dp_thread */
    frame->pp_out.nb_detect = pp_out.nb_detect;
    memcpy(frame->boxes, nn_od_boxes,
           pp_out.nb_detect * sizeof(od_pp_outBuffer_t));
    frame->pp_out.pOutBuff = frame->boxes;

    bqueue_put_free(&nn_input_queue);
    bqueue_put_ready(&nn_output_queue);

    time_stat_update(&stats->nn_total_time, HAL_GetTick() - total_ts);
  }
}

static void dp_thread_fct(void *arg)
{
  stat_info_t *stats = app_stats_state();
  uint32_t total_ts;

  while (1)
  {
    uint8_t *output_buffer;
    nn_dp_frame_t *frame;
    int is_dp_done;

    output_buffer = bqueue_get_ready(&nn_output_queue);
    assert(output_buffer);
    total_ts = HAL_GetTick();

    frame = (nn_dp_frame_t *)output_buffer;
    /* Restore intra-buffer pointer (pOutBuff was serialised as an offset) */
    frame->pp_out.pOutBuff = frame->boxes;

    app_stats_cpuload_update();

    is_dp_done = app_display_render(capture_buffer[capture_buffer_disp_idx],
                                    &frame->pp_out);

    if (is_dp_done)
      time_stat_update(&stats->disp_total_time, HAL_GetTick() - total_ts);

    bqueue_put_free(&nn_output_queue);
  }
}

static void isp_thread_fct(void *arg)
{
  while (1) {
    if (xSemaphoreTake(isp_sem, portMAX_DELAY) == pdTRUE) {
      CAM_IspUpdate();
    }
  }
}

/* ---------------------------------------------------------------------------
 * Persistent parameter table
 * Descriptor order must match app_param_id_t enum in app_config.h.
 * ------------------------------------------------------------------------- */
static const param_descriptor_t s_param_table[PARAM_ID_COUNT] = {
  /* id                    type            default   min    max   */
  { PARAM_CONF_THRESHOLD, PARAM_TYPE_U32,  50ULL,   0ULL,  100ULL },
  { PARAM_BRIGHTNESS,     PARAM_TYPE_U32,  80ULL,   0ULL,  100ULL },
  { PARAM_TARGET_FPS,     PARAM_TYPE_U32,  30ULL,   1ULL,   60ULL },
};

void app_pipeline_init(void)
{
  /* Setup persistent parameter configuration */
  static const params_cfg_t app_pcfg = {
    .flash_base_addr  = PARAM_FLASH_BASE,
    .xspi_instance    = PARAM_XSPI_INST,
    .mmp_base_addr    = XSPI2_BASE,
    .pre_write_hook   = params_pre_write_hook,
    .post_write_hook  = params_post_write_hook,
    .table            = s_param_table,
    .table_count      = PARAM_ID_COUNT,
  };

  /* Create the inference guard mutex (must exist before sysobj_params_init) */
  s_inference_mutex = xSemaphoreCreateMutexStatic(&s_inference_mutex_buf);

  /* Initialize persistent parameters from flash */
  params_status_t pret = sysobj_params_init(&app_pcfg);
  (void)pret; 

  /* Initialize LL_ATON runtime — creates the DAO mutex required by
   * APP_HAS_PARALLEL_NETWORKS=1 before any stai_*_init() or inference call. */
  LL_ATON_RT_RuntimeInit();

  /* Initialize STAI networks */
  stai_return_code ret_stai;
  ret_stai = stai_od_init(network_od_ctx);
  assert(ret_stai == STAI_SUCCESS);
  (void)ret_stai;
  ret_stai = stai_reid_init(network_reid_ctx);
  assert(ret_stai == STAI_SUCCESS);
  (void)ret_stai;

  /* Post-processing initialization */
  app_postprocess_init(&s_od_pp_params, network_od_ctx);

  /* Set target FPS to drive the camera capture */
  uint64_t target_fps = 30;
  sysobj_params_read(PARAM_TARGET_FPS, &target_fps, NULL);
  s_interframe_ms = 1000 / (uint32_t)target_fps;

  /* Initialize queues */
  int ret;
  ret = bqueue_init(&nn_input_queue, 2, (uint8_t *[2]){nn_input_buffers[0], nn_input_buffers[1]});
  assert(ret == 0);
  (void)ret;
  ret = bqueue_init(&nn_output_queue, 2, (uint8_t *[2]){nn_output_buffers[0], nn_output_buffers[1]});
  assert(ret == 0);
  (void)ret;

  isp_sem = xSemaphoreCreateCountingStatic(1, 0, &isp_sem_buffer);
  assert(isp_sem);

  STM32Ipl_InitLib(ipl_mem_pool, sizeof(ipl_mem_pool));
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
  (void)hdl;
  hdl = xTaskCreateStatic(dp_thread_fct, "dp", configMINIMAL_STACK_SIZE * 2, NULL, dp_priority, dp_thread_stack,
                          &dp_thread);
  assert(hdl != NULL);
  (void)hdl;
  hdl = xTaskCreateStatic(isp_thread_fct, "isp", configMINIMAL_STACK_SIZE * 2, NULL, isp_priority, isp_thread_stack,
                          &isp_thread);
  assert(hdl != NULL);
  (void)hdl;
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




