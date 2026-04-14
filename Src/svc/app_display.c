/**
 ******************************************************************************
 * @file    app_display.c
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

#include "svc/app_display.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "FreeRTOS.h"
#include "app/app.h"
#include "app_postprocess.h"
#include "semphr.h"
#include "stm32n6570_discovery.h"
#include "stm32n6xx_hal.h"
#include "svc/app_stats.h"
#include "svc/app_trackobject.h"
#include "svc/draw.h"
#include "svc/figs.h"
#include "sysobj_encoder.h"
#include "task.h"
#include "utils.h"

#define DBG_INFO_FONT font_12
#define CONF_LEVEL_FONT font_16
#define INF_INFO_FONT font_16
#define OBJ_RECT_COLOR 0xffffffff
#define VENC_MAX_WIDTH 1280
#define VENC_MAX_HEIGHT 720
#define VENC_OUT_BUFFER_SIZE (255 * 1024)

typedef struct {
  float conf;
  int16_t x;
  int16_t y;
  int16_t w;
  int16_t h;
} box_t;

static DRAW_Font_t font_12;
static DRAW_Font_t font_16;
static SemaphoreHandle_t dma2d_lock;
static StaticSemaphore_t dma2d_lock_buffer;
static SemaphoreHandle_t dma2d_sem;
static StaticSemaphore_t dma2d_sem_buffer;

static struct uvcl_callbacks uvcl_cbs;
static int uvc_is_active;
static volatile int buffer_flying;
static int force_intra;

static uint8_t venc_out_buffer[VENC_OUT_BUFFER_SIZE] ALIGN_32 UNCACHED;
static uint8_t uvc_in_buffers[VENC_OUT_BUFFER_SIZE] ALIGN_32;

static int clamp_point(int *x, int *y) {
  int xi = *x;
  int yi = *y;

  if (*x < 0)
    *x = 0;
  if (*y < 0)
    *y = 0;
  if (*x >= VENC_WIDTH)
    *x = VENC_WIDTH - 1;
  if (*y >= VENC_HEIGHT)
    *y = VENC_HEIGHT - 1;

  return (xi != *x) || (yi != *y);
}

static void convert_length(float32_t wi, float32_t hi, int *wo, int *ho) {
  *wo = (int)(VENC_WIDTH * wi);
  *ho = (int)(VENC_HEIGHT * hi);
}

static void convert_point(float32_t xi, float32_t yi, int *xo, int *yo) {
  *xo = (int)(VENC_WIDTH * xi);
  *yo = (int)(VENC_HEIGHT * yi);
}

static void cvt_nn_box_to_dp_box(od_pp_outBuffer_t *detect, box_t *box_dp) {
  int xc, yc;
  int x0, y0;
  int x1, y1;
  int w, h;

  convert_point(detect->x_center, detect->y_center, &xc, &yc);
  convert_length(detect->width, detect->height, &w, &h);
  x0 = xc - (w + 1) / 2;
  y0 = yc - (h + 1) / 2;
  x1 = xc + (w + 1) / 2;
  y1 = yc + (h + 1) / 2;
  clamp_point(&x0, &y0);
  clamp_point(&x1, &y1);

  box_dp->x = x0;
  box_dp->y = y0;
  box_dp->w = x1 - x0;
  box_dp->h = y1 - y0;
  box_dp->conf = detect->conf;
}

static void draw_track_box(uint8_t *p_buffer, TrackObject_s *track) {
  box_t box_disp;

  cvt_nn_box_to_dp_box(&track->dbox, &box_disp);
  DRAW_RectArgbHw(p_buffer, VENC_WIDTH, VENC_HEIGHT, box_disp.x, box_disp.y,
                  box_disp.w, box_disp.h, OBJ_RECT_COLOR);
  DRAW_PrintfArgbHw(&CONF_LEVEL_FONT, p_buffer, VENC_WIDTH, VENC_HEIGHT,
                    box_disp.x, box_disp.y, "ID:%d", track->id);
}

static void time_stat_display(time_stat_t *p_stat, uint8_t *p_buffer,
                              char *label, int line_nb, int indent) {
  int offset = VENC_WIDTH - 41 * DBG_INFO_FONT.width;

  DRAW_PrintfArgbHw(&DBG_INFO_FONT, p_buffer, VENC_WIDTH, VENC_HEIGHT, offset,
                    line_nb * DBG_INFO_FONT.height,
                    "%*s%s : %3d ms / %5.1f ms ", indent + 1, "", label,
                    p_stat->last, p_stat->mean);
}

static int build_display_nn_dbg(uint8_t *p_buffer, stat_info_t *si,
                                int line_nb) {
  time_stat_display(&si->nn_total_time, p_buffer, "NN thread stats  ",
                    line_nb++, 0);
  time_stat_display(&si->nn_inference_time, p_buffer, "inference    ",
                    line_nb++, 4);

  return line_nb;
}

static int build_display_disp_dbg(uint8_t *p_buffer, stat_info_t *si,
                                  int line_nb) {
  time_stat_display(&si->disp_total_time, p_buffer, "DISP thread stats",
                    line_nb++, 0);
  time_stat_display(&si->nn_pp_time, p_buffer, "pp           ", line_nb++, 4);
  time_stat_display(&si->disp_display_time, p_buffer, "display      ",
                    line_nb++, 4);
  time_stat_display(&si->disp_enc_time, p_buffer, "encode       ", line_nb++,
                    4);

  return line_nb;
}

static int update_and_capture_debug_enabled(void) {
  static int prev_button_state = GPIO_PIN_RESET;
  static int display_debug_enabled = 0;
  int cur_button_state;

  cur_button_state = BSP_PB_GetState(BUTTON_USER1);
  if (cur_button_state == GPIO_PIN_SET && prev_button_state == GPIO_PIN_RESET)
    display_debug_enabled = !display_debug_enabled;
  prev_button_state = cur_button_state;

  return display_debug_enabled;
}

static int build_display_inference_info(uint8_t *p_buffer, uint32_t inf_time,
                                        int line_nb) {
  const int offset_x = 16;

  DRAW_PrintfArgbHw(&INF_INFO_FONT, p_buffer, VENC_WIDTH, VENC_HEIGHT, offset_x,
                    line_nb * INF_INFO_FONT.height, " Inference : %4.1f ms ",
                    (double)inf_time);

  return line_nb + 1;
}

static int build_display_cpu_load(uint8_t *p_buffer, int line_nb) {
  const int offset_x = 16;
  float cpu_load_one_second;

  app_stats_cpuload_get(NULL, &cpu_load_one_second, NULL);
  DRAW_PrintfArgbHw(&INF_INFO_FONT, p_buffer, VENC_WIDTH, VENC_HEIGHT, offset_x,
                    line_nb * INF_INFO_FONT.height, " Cpu load  : %4.1f  %% ",
                    cpu_load_one_second);
  line_nb++;

  return line_nb;
}

static void build_display_stat_info(uint8_t *p_buffer, stat_info_t *si) {
  int line_nb = 1;

  if (!update_and_capture_debug_enabled())
    return;

  line_nb = build_display_nn_dbg(p_buffer, si, line_nb);
  build_display_disp_dbg(p_buffer, si, line_nb);
}

static void build_display(uint8_t *p_buffer, od_pp_out_t *pp_out) {
  const uint8_t *fig_array[] = {fig0, fig1, fig2, fig3, fig4,
                                fig5, fig6, fig7, fig8, fig9};
  int line_nb = VENC_HEIGHT / INF_INFO_FONT.height - 4;
  stat_info_t si_copy;
  int nb;

  stat_info_copy(&si_copy);

  {
    TO_DisplayIterator_s it;
    TrackObject_s *track;
    TrackObject_DisplayIterator_Init(&it);
    while ((track = TrackObject_DisplayIterator_GetNext(&it)) != NULL) {
      if (track->is_dbox_valid)
        draw_track_box(p_buffer, track);
    }
  }

  line_nb = build_display_inference_info(
      p_buffer, si_copy.nn_inference_time.last, line_nb);
  line_nb = build_display_cpu_load(p_buffer, line_nb);

  nb = MIN(pp_out->nb_detect, ARRAY_NB(fig_array) - 1);
  DRAW_CopyArgbHW(p_buffer, VENC_WIDTH, VENC_HEIGHT, (uint8_t *)fig_array[nb],
                  64, 64, 16, 16);

  build_display_stat_info(p_buffer, &si_copy);
}

static size_t encode_display(int is_intra_force, uint8_t *p_buffer) {
  size_t res;

  res = ENC_EncodeFrame(p_buffer, venc_out_buffer, VENC_OUT_BUFFER_SIZE,
                        is_intra_force);
  if ((int)res > 0 && buffer_flying) {
    force_intra = 1;
    return (size_t)-1;
  }

  if ((int)res <= 0)
    return res;

  memcpy(&uvc_in_buffers, venc_out_buffer, res);

  return res;
}

static int send_display(int len) {
  int ret;

  buffer_flying = 1;
  ret = UVCL_ShowFrame(uvc_in_buffers, len);
  if (ret != 0)
    buffer_flying = 0;

  return ret;
}

static void app_uvc_streaming_active(struct uvcl_callbacks *cbs,
                                     UVCL_StreamConf_t stream) {
  (void)cbs;
  (void)stream;
  uvc_is_active = 1;
  BSP_LED_On(LED_RED);
}

static void app_uvc_streaming_inactive(struct uvcl_callbacks *cbs) {
  (void)cbs;
  uvc_is_active = 0;
  BSP_LED_Off(LED_RED);
}

static void app_uvc_frame_release(struct uvcl_callbacks *cbs, void *frame) {
  (void)cbs;
  (void)frame;
  assert(buffer_flying);

  buffer_flying = 0;
}

void app_display_init(void) {
  int ret;

  dma2d_sem = xSemaphoreCreateCountingStatic(1, 0, &dma2d_sem_buffer);
  assert(dma2d_sem);
  dma2d_lock = xSemaphoreCreateMutexStatic(&dma2d_lock_buffer);
  assert(dma2d_lock);

  ret = DRAW_FontSetup(&Font12, &font_12);
  assert(ret == 0);
  ret = DRAW_FontSetup(&Font16, &font_16);
  assert(ret == 0);

  buffer_flying = 0;
  force_intra = 0;
  uvc_is_active = 0;
}

int app_display_setup(const ENC_Conf_t *enc_conf,
                      const UVCL_Conf_t *uvcl_conf) {
  UVCL_Conf_t uvcl_local = *uvcl_conf;
  ENC_Conf_t enc_local = *enc_conf;
  int ret;

  ENC_Init(&enc_local);

  uvcl_cbs.streaming_active = app_uvc_streaming_active;
  uvcl_cbs.streaming_inactive = app_uvc_streaming_inactive;
  uvcl_cbs.frame_release = app_uvc_frame_release;
  ret = UVCL_Init(USB1_OTG_HS, &uvcl_local, &uvcl_cbs);

  return ret;
}

int app_display_render(uint8_t *frame_buffer, od_pp_out_t *pp_out) {
  static int uvc_is_active_prev = 0;
  stat_info_t *stats = app_stats_state();
  uint32_t ts;
  int len;

  if (!uvc_is_active) {
    uvc_is_active_prev = uvc_is_active;
    return 0;
  }

  ts = HAL_GetTick();
  build_display(frame_buffer, pp_out);

  stats_detect_update(pp_out->nb_detect);

  time_stat_update(&stats->disp_display_time, HAL_GetTick() - ts);

  ts = HAL_GetTick();
  len = (int)encode_display(!uvc_is_active_prev || force_intra, frame_buffer);
  time_stat_update(&stats->disp_enc_time, HAL_GetTick() - ts);

  if (len > 0)
    send_display(len);

  force_intra = 0;
  uvc_is_active_prev = uvc_is_active;

  return uvc_is_active;
}

void DRAW_HwLock(void *dma2d_handle) {
  int ret;

  (void)dma2d_handle;
  ret = xSemaphoreTake(dma2d_lock, portMAX_DELAY);
  assert(ret == pdTRUE);
}

void DRAW_HwUnlock(void) {
  int ret;

  ret = xSemaphoreGive(dma2d_lock);
  assert(ret == pdTRUE);
}

void DRAW_Wfe(void) {
  int ret;

  ret = xSemaphoreTake(dma2d_sem, portMAX_DELAY);
  assert(ret == pdTRUE);
}

void DRAW_Signal(void) {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  int ret;

  ret = xSemaphoreGiveFromISR(dma2d_sem, &xHigherPriorityTaskWoken);
  assert(ret == pdTRUE);

  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
