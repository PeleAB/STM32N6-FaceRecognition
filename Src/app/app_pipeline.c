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
#include <stdio.h>
#include <string.h>

#include "app/app.h"
#include "app/app_config.h"
#include "app/face_debug_image.h"
#include "sysobj_params.h"
#include "app_postprocess.h"
#include "sysobj_cache.h"
#include "sysobj_camera.h"
#include "svc/app_display.h"
#include "svc/app_stats.h"
#include "svc/buffer_queue.h"
#include "svc/face_detect.h"
#include "stai_fd.h"
#include "stai_faceid.h"
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

/* Model Related Info — CenterFace (fd) + MobileFaceNet (faceid) */
#define NN_FD_INPUT_SIZE      STAI_FD_IN_1_SIZE      /* 128*128*3 = 49152 */
#define NN_FACEID_INPUT_SIZE  STAI_FACEID_IN_1_SIZE   /* 112*112*3 = 37632 */
#define NN_FACEID_OUTPUT_SIZE STAI_FACEID_OUT_1_SIZE   /* 128 */

#define NN_INPUT_BUFFER_SIZE  NN_FD_INPUT_SIZE        /* camera pipe delivers fd-sized frames */

/* Latest inference result. Display consumes a snapshot without waiting for NN. */
typedef struct {
  od_pp_out_t       pp_out;
  od_pp_outBuffer_t boxes[APP_MAX_OBJECT_DETECT];
  float             landmarks[APP_MAX_OBJECT_DETECT][FACE_NB_LANDMARKS * 2];
} nn_dp_frame_t;

/* capture buffers */
static uint8_t capture_buffer[CAPTURE_BUFFER_NB][VENC_MAX_WIDTH * VENC_MAX_HEIGHT * CAPTURE_BPP] ALIGN_32 IN_PSRAM;
static int capture_buffer_disp_idx = 1;
static int capture_buffer_capt_idx = 0;

/* STAI network contexts (statically allocated) */
STAI_NETWORK_CONTEXT_DECLARE(network_fd_ctx, STAI_FD_CONTEXT_SIZE);
STAI_NETWORK_CONTEXT_DECLARE(network_faceid_ctx, STAI_FACEID_CONTEXT_SIZE);

static uint8_t nn_input_buffers[2][NN_INPUT_BUFFER_SIZE] ALIGN_32;
static uint8_t fd_input_buffer[NN_FD_INPUT_SIZE] ALIGN_32;
static bqueue_t nn_input_queue;
static nn_dp_frame_t s_latest_frame;
static nn_dp_frame_t s_display_frame;
static SemaphoreHandle_t s_latest_frame_mutex;
static StaticSemaphore_t s_latest_frame_mutex_buf;
static SemaphoreHandle_t s_display_sem;
static StaticSemaphore_t s_display_sem_buf;

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
static centerface_pp_static_param_t s_fd_pp_params;
static volatile uint32_t s_face_input_mode = FACE_INPUT_CAMERA;

void app_pipeline_set_face_input_mode(uint32_t mode)
{
  if (mode <= FACE_INPUT_STATIC)
    s_face_input_mode = mode;
}

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
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  int next_disp_idx = (capture_buffer_disp_idx + 1) % CAPTURE_BUFFER_NB;
  int next_capt_idx = (capture_buffer_capt_idx + 1) % CAPTURE_BUFFER_NB;
  int ret;

  ret = CAM_DisplayPipe_UpdateAddress(capture_buffer[next_capt_idx]);
  assert(ret == 0);
  (void)ret;

  capture_buffer_disp_idx = next_disp_idx;
  capture_buffer_capt_idx = next_capt_idx;

  /* A binary semaphore deliberately coalesces frames if display/USB is busy.
   * That bounds latency: stale display work is dropped instead of queued. */
  if (xPortIsInsideInterrupt()) {
    if (xSemaphoreGiveFromISR(s_display_sem, &xHigherPriorityTaskWoken) == pdTRUE)
      portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  } else {
    (void)xSemaphoreGive(s_display_sem);
  }
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

/* Fit a 2D similarity transform from detected landmarks to MobileFaceNet's
 * canonical 112x112 template, then write the model's planar RGB input. */
static int face_align_to_chw(const uint8_t *src_data,
                             const float landmarks[FACE_NB_LANDMARKS * 2],
                             uint8_t *dst)
{
  static const float canonical[FACE_NB_LANDMARKS][2] = {
    {38.2946f, 51.6963f}, {73.5318f, 51.5014f},
    {56.0252f, 71.7366f}, {41.5493f, 92.3655f},
    {70.7299f, 92.2041f}
  };
  float src_mean_x = 0.0f, src_mean_y = 0.0f;
  float dst_mean_x = 0.0f, dst_mean_y = 0.0f;
  float dot = 0.0f, cross = 0.0f, norm = 0.0f;

  for (int i = 0; i < FACE_NB_LANDMARKS; i++) {
    src_mean_x += landmarks[2 * i] * NN_WIDTH;
    src_mean_y += landmarks[2 * i + 1] * NN_HEIGHT;
    dst_mean_x += canonical[i][0];
    dst_mean_y += canonical[i][1];
  }
  src_mean_x /= FACE_NB_LANDMARKS;
  src_mean_y /= FACE_NB_LANDMARKS;
  dst_mean_x /= FACE_NB_LANDMARKS;
  dst_mean_y /= FACE_NB_LANDMARKS;

  for (int i = 0; i < FACE_NB_LANDMARKS; i++) {
    float sx = landmarks[2 * i] * NN_WIDTH - src_mean_x;
    float sy = landmarks[2 * i + 1] * NN_HEIGHT - src_mean_y;
    float dx = canonical[i][0] - dst_mean_x;
    float dy = canonical[i][1] - dst_mean_y;
    dot += sx * dx + sy * dy;
    cross += sx * dy - sy * dx;
    norm += sx * sx + sy * sy;
  }
  if (norm < 1.0f)
    return -1;

  float a = dot / norm;
  float b = cross / norm;
  float tx = dst_mean_x - a * src_mean_x + b * src_mean_y;
  float ty = dst_mean_y - b * src_mean_x - a * src_mean_y;
  float transform_norm = a * a + b * b;
  if (transform_norm < 1.0e-6f)
    return -1;

  const rgb888_t *src = (const rgb888_t *)src_data;
  const int plane_size = STAI_FACEID_IN_1_WIDTH * STAI_FACEID_IN_1_HEIGHT;
  for (int y = 0; y < STAI_FACEID_IN_1_HEIGHT; y++) {
    for (int x = 0; x < STAI_FACEID_IN_1_WIDTH; x++) {
      float ux = (float)x - tx;
      float uy = (float)y - ty;
      float src_x = (a * ux + b * uy) / transform_norm;
      float src_y = (-b * ux + a * uy) / transform_norm;
      int out_idx = y * STAI_FACEID_IN_1_WIDTH + x;

      if (src_x < 0.0f || src_y < 0.0f ||
          src_x > (float)(NN_WIDTH - 1) || src_y > (float)(NN_HEIGHT - 1)) {
        dst[out_idx] = 0;
        dst[plane_size + out_idx] = 0;
        dst[2 * plane_size + out_idx] = 0;
        continue;
      }

      int x0 = (int)src_x;
      int y0 = (int)src_y;
      int x1 = MIN(x0 + 1, NN_WIDTH - 1);
      int y1 = MIN(y0 + 1, NN_HEIGHT - 1);
      float wx = src_x - (float)x0;
      float wy = src_y - (float)y0;
      const rgb888_t p00 = src[y0 * NN_WIDTH + x0];
      const rgb888_t p10 = src[y0 * NN_WIDTH + x1];
      const rgb888_t p01 = src[y1 * NN_WIDTH + x0];
      const rgb888_t p11 = src[y1 * NN_WIDTH + x1];
#define BILERP_CHANNEL(channel) \
      ((1.0f - wy) * ((1.0f - wx) * p00.channel + wx * p10.channel) + \
       wy * ((1.0f - wx) * p01.channel + wx * p11.channel))
      dst[out_idx] = (uint8_t)(BILERP_CHANNEL(r) + 0.5f);
      dst[plane_size + out_idx] = (uint8_t)(BILERP_CHANNEL(g) + 0.5f);
      dst[2 * plane_size + out_idx] = (uint8_t)(BILERP_CHANNEL(b) + 0.5f);
#undef BILERP_CHANNEL
    }
  }
  return 0;
}

/* DCMIPP RGB888_YUV444_1 is stored as packed B,G,R bytes (rgb888_t), while
 * this CenterFace artifact exposes a contiguous N,C,H,W input tensor. */
static void camera_bgr_to_rgb_chw(const uint8_t *src_data, uint8_t *dst)
{
  const rgb888_t *src = (const rgb888_t *)src_data;
  const int plane_size = NN_WIDTH * NN_HEIGHT;

  for (int i = 0; i < plane_size; i++) {
    dst[i] = src[i].r;
    dst[plane_size + i] = src[i].g;
    dst[2 * plane_size + i] = src[i].b;
  }
}

static void load_static_face_fixture(uint8_t *dst_data)
{
  rgb888_t *dst = (rgb888_t *)dst_data;

  for (int i = 0; i < FACE_DEBUG_IMAGE_WIDTH * FACE_DEBUG_IMAGE_HEIGHT; i++) {
    uint16_t pixel = g_face_debug_image_rgb565[i];
    uint8_t r5 = (uint8_t)((pixel >> 11) & 0x1fU);
    uint8_t g6 = (uint8_t)((pixel >> 5) & 0x3fU);
    uint8_t b5 = (uint8_t)(pixel & 0x1fU);
    dst[i].r = (uint8_t)((r5 << 3) | (r5 >> 2));
    dst[i].g = (uint8_t)((g6 << 2) | (g6 >> 4));
    dst[i].b = (uint8_t)((b5 << 3) | (b5 >> 2));
  }
}

static void nn_thread_fct(void *arg)
{
  static od_pp_outBuffer_t nn_fd_boxes[APP_MAX_OBJECT_DETECT];
  static float faceid_features_all[APP_MAX_OBJECT_DETECT][NN_FACEID_OUTPUT_SIZE] ALIGN_32;
  stat_info_t *stats = app_stats_state();
  uint32_t nn_period[2];
  uint32_t total_ts, ts;
  uint32_t nn_period_ms;
  (void)nn_period_ms;

  nn_period[1] = HAL_GetTick();

  uint8_t *nn_pipe_dst = bqueue_get_free(&nn_input_queue, 0);
  assert(nn_pipe_dst);
  CAM_NNPipe_Start(nn_pipe_dst, CMW_MODE_CONTINUOUS);

  /* Get faceid input buffer — statically allocated by model, address is fixed */
  stai_ptr faceid_input_ptrs[1];
  stai_size n_faceid_in;
  stai_faceid_get_inputs(network_faceid_ctx, faceid_input_ptrs, &n_faceid_in);
  uint8_t *faceid_input_buf = (uint8_t *)faceid_input_ptrs[0];

  while (1)
  {
    uint8_t *capture_buffer_local;

    nn_period[0] = nn_period[1];
    nn_period[1] = HAL_GetTick();
    nn_period_ms = nn_period[1] - nn_period[0];

    capture_buffer_local = bqueue_get_ready(&nn_input_queue);
    assert(capture_buffer_local);

    total_ts = HAL_GetTick();
    ts = HAL_GetTick();

    /* 1. Face detection inference */
    /* Camera DMA wrote directly to memory.  Discard any cache lines left by
     * processing this buffer's previous frame; cleaning here would write that
     * stale frame back over the DMA result. */
    SYSOBJ_CacheInvalidate(capture_buffer_local, NN_INPUT_BUFFER_SIZE);

    uint32_t face_input_mode = s_face_input_mode;
    if (face_input_mode == FACE_INPUT_STATIC)
      load_static_face_fixture(capture_buffer_local);

    camera_bgr_to_rgb_chw(capture_buffer_local, fd_input_buffer);
    SCB_CleanDCache_by_Addr((uint32_t *)fd_input_buffer,
                            ALIGN_VALUE(NN_FD_INPUT_SIZE, 32));

    stai_ptr fd_inputs[1] = { fd_input_buffer };
    stai_fd_set_inputs(network_fd_ctx, fd_inputs, 1);
    xSemaphoreTake(s_inference_mutex, portMAX_DELAY);
    Run_Inference(network_fd_ctx, stai_fd_run, stai_ext_fd_new_inference);
    xSemaphoreGive(s_inference_mutex);

    /* Copy FD outputs — 4 heads (heatmap, scale, offset, landmarks) */
    stai_ptr fd_outputs[STAI_FD_OUT_NUM];
    stai_size n_fd_out;
    stai_fd_get_outputs(network_fd_ctx, fd_outputs, &n_fd_out);
    assert(n_fd_out == FD_OUT_NUM);

    /* Invalidate exactly the generated output ranges. */
    static const size_t fd_output_sizes[STAI_FD_OUT_NUM] = STAI_FD_OUT_SIZES_BYTES;
    for (int i = 0; i < (int)n_fd_out; i++)
      SYSOBJ_CacheInvalidate(fd_outputs[i], ALIGN_VALUE(fd_output_sizes[i], 32));

    /* 2. FD postprocess — outputs bounding boxes + landmarks */
    /* Build a temporary output struct with our landmark buffer */
    struct {
      od_pp_out_t       boxes;
      float             landmarks[APP_MAX_OBJECT_DETECT][FACE_NB_LANDMARKS * 2];
    } pp_out_buf;
    pp_out_buf.boxes.pOutBuff = nn_fd_boxes;
    pp_out_buf.boxes.nb_detect = 0;

    (void)app_postprocess_run((void **)fd_outputs, FD_OUT_NUM,
                              &pp_out_buf, &s_fd_pp_params);

    /* --- DIAGNOSTIC: dump NPU input/output stats every ~1s --- */
    {
      static uint32_t dbg_cnt = 0;
      if ((dbg_cnt++ % 30) == 0) {
        /* Input buffer non-zero / sum to confirm camera is writing real data. */
        const uint8_t *in = fd_input_buffer;
        uint32_t sum = 0;
        uint32_t nonzero = 0;
        uint8_t in_min = 255, in_max = 0;
        for (int i = 0; i < (int)NN_INPUT_BUFFER_SIZE; i++) {
          uint8_t v = in[i];
          sum += v;
          if (v != 0) nonzero++;
          if (v < in_min) in_min = v;
          if (v > in_max) in_max = v;
        }

        /* Heatmap scan */
        const float *hm  = (const float *)fd_outputs[FD_OUT_HEATMAP];
        const float *sc  = (const float *)fd_outputs[FD_OUT_SCALE];
        const float *off = (const float *)fd_outputs[FD_OUT_OFFSET];
        float hm_max = -1e30f;
        float hm_min =  1e30f;
        int hm_max_idx = -1;
        int cnt_03 = 0, cnt_05 = 0, cnt_07 = 0;
        for (int i = 0; i < AI_FD_PP_GRID_WIDTH * AI_FD_PP_GRID_HEIGHT; i++) {
          float v = hm[i];
          if (v > hm_max) { hm_max = v; hm_max_idx = i; }
          if (v < hm_min) hm_min = v;
          if (v > 0.3f) cnt_03++;
          if (v > 0.5f) cnt_05++;
          if (v > 0.7f) cnt_07++;
        }
        int mx_r = (hm_max_idx >= 0) ? hm_max_idx / AI_FD_PP_GRID_WIDTH : -1;
        int mx_c = (hm_max_idx >= 0) ? hm_max_idx % AI_FD_PP_GRID_WIDTH : -1;

        printf("[FD] src=%s in sum=%lu nz=%lu/%d min=%u max=%u  "
               "hm min=%.3f max=%.3f@(%d,%d) cnt>.3=%d >.5=%d >.7=%d  "
               "sc[0..1]=%.3f,%.3f off[0..1]=%.3f,%.3f nb=%d\r\n",
               face_input_mode == FACE_INPUT_STATIC ? "static" : "camera",
               (unsigned long)sum, (unsigned long)nonzero, (int)NN_INPUT_BUFFER_SIZE,
               in_min, in_max,
               hm_min, hm_max, mx_r, mx_c, cnt_03, cnt_05, cnt_07,
               sc[0], sc[1], off[0], off[1],
               (int)pp_out_buf.boxes.nb_detect);
      }
    }

    /* 3. FaceID embedding per detected face */
    int nb_faces = MIN(pp_out_buf.boxes.nb_detect, APP_MAX_OBJECT_DETECT);

    for (int i = 0; i < nb_faces; i++) {
      if (face_align_to_chw(capture_buffer_local, pp_out_buf.landmarks[i],
                            faceid_input_buf) != 0) {
        memset(faceid_features_all[i], 0,
               NN_FACEID_OUTPUT_SIZE * sizeof(float));
        continue;
      }
      SCB_CleanDCache_by_Addr((uint32_t *)faceid_input_buf,
                              ALIGN_VALUE(NN_FACEID_INPUT_SIZE, 32));

      stai_ptr faceid_out[1] = { (uint8_t *)faceid_features_all[i] };
      stai_faceid_set_outputs(network_faceid_ctx, faceid_out, 1);
      xSemaphoreTake(s_inference_mutex, portMAX_DELAY);
      Run_Inference(network_faceid_ctx, stai_faceid_run, stai_ext_faceid_new_inference);
      xSemaphoreGive(s_inference_mutex);
      SYSOBJ_CacheInvalidate(faceid_features_all[i],
                             ALIGN_VALUE(NN_FACEID_OUTPUT_SIZE * sizeof(float), 32));
    }

    /* 4. Update tracking */
    od_pp_out_t tracking_pp = pp_out_buf.boxes;
    tracking_pp.nb_detect = nb_faces;
    TrackObject_UpdateAll(&tracking_pp, faceid_features_all);

    time_stat_update(&stats->nn_inference_time, HAL_GetTick() - ts);

    /* Publish one atomic metadata snapshot. Video transmission never waits
     * for this section; it reuses the previous snapshot if inference is busy. */
    xSemaphoreTake(s_latest_frame_mutex, portMAX_DELAY);
    s_latest_frame.pp_out.nb_detect = nb_faces;
    memcpy(s_latest_frame.boxes, nn_fd_boxes,
           nb_faces * sizeof(od_pp_outBuffer_t));
    s_latest_frame.pp_out.pOutBuff = s_latest_frame.boxes;
    memcpy(s_latest_frame.landmarks, pp_out_buf.landmarks,
           nb_faces * FACE_NB_LANDMARKS * 2 * sizeof(float));
    xSemaphoreGive(s_latest_frame_mutex);

    bqueue_put_free(&nn_input_queue);

    time_stat_update(&stats->nn_total_time, HAL_GetTick() - total_ts);
  }
}

static void dp_thread_fct(void *arg)
{
  stat_info_t *stats = app_stats_state();
  uint32_t total_ts;

  while (1)
  {
    int is_dp_done;

    xSemaphoreTake(s_display_sem, portMAX_DELAY);
    total_ts = HAL_GetTick();

    xSemaphoreTake(s_latest_frame_mutex, portMAX_DELAY);
    memcpy(&s_display_frame, &s_latest_frame, sizeof(s_display_frame));
    xSemaphoreGive(s_latest_frame_mutex);
    s_display_frame.pp_out.pOutBuff = s_display_frame.boxes;

    app_stats_cpuload_update();

    is_dp_done = app_display_render(capture_buffer[capture_buffer_disp_idx],
                                    &s_display_frame.pp_out,
                                    s_display_frame.landmarks,
                                    s_display_frame.pp_out.nb_detect);

    if (is_dp_done)
      time_stat_update(&stats->disp_total_time, HAL_GetTick() - total_ts);

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
  { PARAM_FACE_INPUT_MODE, PARAM_TYPE_U32,  0ULL,   0ULL,    1ULL },
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

  uint64_t face_input_mode = FACE_INPUT_CAMERA;
  (void)sysobj_params_read(PARAM_FACE_INPUT_MODE, &face_input_mode, NULL);
  app_pipeline_set_face_input_mode((uint32_t)face_input_mode);

  /* Initialize LL_ATON runtime — creates the DAO mutex required by
   * APP_HAS_PARALLEL_NETWORKS=1 before any stai_*_init() or inference call. */
  LL_ATON_RT_RuntimeInit();

  /* Initialize STAI networks */
  stai_return_code ret_stai;
  ret_stai = stai_fd_init(network_fd_ctx);
  assert(ret_stai == STAI_SUCCESS);
  (void)ret_stai;
  ret_stai = stai_faceid_init(network_faceid_ctx);
  assert(ret_stai == STAI_SUCCESS);
  (void)ret_stai;

  /* Post-processing initialization */
  app_postprocess_init(&s_fd_pp_params, network_fd_ctx);

  /* Set target FPS to drive the camera capture */
  uint64_t target_fps = 30;
  sysobj_params_read(PARAM_TARGET_FPS, &target_fps, NULL);
  s_interframe_ms = 1000 / (uint32_t)target_fps;

  /* Initialize queues */
  int ret;
  ret = bqueue_init(&nn_input_queue, 2, (uint8_t *[2]){nn_input_buffers[0], nn_input_buffers[1]});
  assert(ret == 0);
  (void)ret;

  s_latest_frame_mutex = xSemaphoreCreateMutexStatic(&s_latest_frame_mutex_buf);
  assert(s_latest_frame_mutex);
  s_display_sem = xSemaphoreCreateBinaryStatic(&s_display_sem_buf);
  assert(s_display_sem);

  isp_sem = xSemaphoreCreateCountingStatic(1, 0, &isp_sem_buffer);
  assert(isp_sem);

  STM32Ipl_InitLib(ipl_mem_pool, sizeof(ipl_mem_pool));
}

void app_pipeline_start(void)
{
  UBaseType_t isp_priority = FREERTOS_PRIORITY(2);
  UBaseType_t dp_priority = FREERTOS_PRIORITY(0);
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
