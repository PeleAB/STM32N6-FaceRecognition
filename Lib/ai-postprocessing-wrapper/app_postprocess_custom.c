/**
 ******************************************************************************
 * @file    app_postprocess_custom.c
 * @brief   CenterFace post-processing: heatmap decode, NMS, landmarks.
 *
 * CenterFace has 4 float32 output heads at grid_h x grid_w resolution:
 *   [0] heatmap   (grid_h, grid_w,  1) — sigmoid face confidence
 *   [1] scale     (grid_h, grid_w,  2) — w, h in pixels
 *   [2] offset    (grid_h, grid_w,  2) — sub-pixel center offset
 *   [3] landmarks (grid_h, grid_w, 10) — 5 keypoints x (x, y) pixel offsets
 *
 * Output head indices are defined in face_detect.h (FD_OUT_*).
 * Adjust if stedgeai reorders the outputs.
 ******************************************************************************
 */

#include "app_postprocess.h"
#include "app_config.h"
#include "svc/face_detect.h"
#include <assert.h>
#include <math.h>
#include <string.h>

#if POSTPROCESS_TYPE == POSTPROCESS_CUSTOM

/* CenterFace model input size — must match NN_WIDTH / NN_HEIGHT */
#define FD_INPUT_W  128
#define FD_INPUT_H  128
#define FD_STRIDE   (FD_INPUT_W / AI_FD_PP_GRID_WIDTH)  /* 4 */

static float clamp01(float value)
{
  return fminf(1.0f, fmaxf(0.0f, value));
}

/* Scratch: one candidate per grid cell (worst case) */
typedef struct {
  float x0, y0, x1, y1;   /* top-left / bottom-right, normalised [0,1] */
  float conf;
  float lm[FACE_NB_LANDMARKS * 2]; /* normalised [0,1] */
} candidate_t;

static candidate_t scratch[AI_FD_PP_GRID_WIDTH * AI_FD_PP_GRID_HEIGHT];

/* ------------------------------------------------------------------ */
static float iou(const candidate_t *a, const candidate_t *b)
{
  float ix0 = fmaxf(a->x0, b->x0);
  float iy0 = fmaxf(a->y0, b->y0);
  float ix1 = fminf(a->x1, b->x1);
  float iy1 = fminf(a->y1, b->y1);
  float iw  = fmaxf(0.0f, ix1 - ix0);
  float ih  = fmaxf(0.0f, iy1 - iy0);
  float inter = iw * ih;
  float area_a = (a->x1 - a->x0) * (a->y1 - a->y0);
  float area_b = (b->x1 - b->x0) * (b->y1 - b->y0);
  float uni = area_a + area_b - inter;
  return (uni > 0.0f) ? inter / uni : 0.0f;
}

/* Simple greedy NMS — sort by confidence, suppress overlapping boxes */
static int nms(candidate_t *cands, int n, float iou_thresh, int max_keep)
{
  /* Insertion sort descending by confidence (n is small, <=1024) */
  for (int i = 1; i < n; i++) {
    candidate_t key = cands[i];
    int j = i - 1;
    while (j >= 0 && cands[j].conf < key.conf) {
      cands[j + 1] = cands[j];
      j--;
    }
    cands[j + 1] = key;
  }

  int keep = 0;
  for (int i = 0; i < n && keep < max_keep; i++) {
    if (cands[i].conf < 0.0f)
      continue; /* suppressed */

    /* Move kept candidate to front */
    if (keep != i)
      cands[keep] = cands[i];
    keep++;

    /* Suppress lower-confidence overlapping boxes */
    for (int j = i + 1; j < n; j++) {
      if (cands[j].conf < 0.0f)
        continue;
      if (iou(&cands[i], &cands[j]) > iou_thresh)
        cands[j].conf = -1.0f;
    }
  }

  return keep;
}

/* ------------------------------------------------------------------ */
int32_t app_postprocess_init(void *params_postprocess, void *network_instance)
{
  (void)network_instance;
  centerface_pp_static_param_t *p = (centerface_pp_static_param_t *)params_postprocess;

  p->grid_width      = AI_FD_PP_GRID_WIDTH;
  p->grid_height     = AI_FD_PP_GRID_HEIGHT;
  p->conf_threshold  = AI_FD_PP_CONF_THRESHOLD;
  p->iou_threshold   = AI_FD_PP_IOU_THRESHOLD;
  p->max_boxes_limit = AI_FD_PP_MAX_BOXES_LIMIT;

  return 0;
}

int32_t app_postprocess_run(void *pInput[], int nb_input, void *pOutput, void *pInput_param)
{
  assert(nb_input == FD_OUT_NUM);

  centerface_pp_static_param_t *p = (centerface_pp_static_param_t *)pInput_param;
  centerface_pp_out_t *out = (centerface_pp_out_t *)pOutput;

  const int gw = p->grid_width;
  const int gh = p->grid_height;
  const float conf_th = p->conf_threshold;
  const float inv_w = 1.0f / (float)FD_INPUT_W;
  const float inv_h = 1.0f / (float)FD_INPUT_H;

  const float *heatmap   = (const float *)pInput[FD_OUT_HEATMAP];   /* [gh, gw, 1]  */
  const float *scale     = (const float *)pInput[FD_OUT_SCALE];     /* [gh, gw, 2]  */
  const float *offset    = (const float *)pInput[FD_OUT_OFFSET];    /* [gh, gw, 2]  */
  const float *landmarks = (const float *)pInput[FD_OUT_LANDMARKS]; /* [gh, gw, 10] */

  int n_cand = 0;

  for (int row = 0; row < gh; row++) {
    for (int col = 0; col < gw; col++) {
      int cell = row * gw + col;

      /* Heatmap is post-sigmoid in the CenterFace graph */
      float conf = heatmap[cell];
      if (conf < conf_th)
        continue;

      /* Decode bounding box → normalised [0,1]
       * Scale values are in feature-map stride units (already exp-baked by model).
       * Offset is a sub-pixel correction to the grid-cell center. */
      float bh_px = expf(fminf(10.0f, fmaxf(-10.0f, scale[cell * 2 + 0]))) * FD_STRIDE;
      float bw_px = expf(fminf(10.0f, fmaxf(-10.0f, scale[cell * 2 + 1]))) * FD_STRIDE;
      float y0_px = ((float)row + offset[cell * 2 + 0] + 0.5f) * FD_STRIDE - bh_px * 0.5f;
      float x0_px = ((float)col + offset[cell * 2 + 1] + 0.5f) * FD_STRIDE - bw_px * 0.5f;

      candidate_t *c = &scratch[n_cand++];
      c->x0   = clamp01(x0_px * inv_w);
      c->y0   = clamp01(y0_px * inv_h);
      c->x1   = clamp01((x0_px + bw_px) * inv_w);
      c->y1   = clamp01((y0_px + bh_px) * inv_h);
      c->conf = conf;

      /* Decode landmarks — pixel offsets from grid cell origin */
      const float *lm_ptr = &landmarks[cell * 10];
      for (int k = 0; k < FACE_NB_LANDMARKS; k++) {
        c->lm[2 * k + 0] = clamp01((x0_px + lm_ptr[2 * k + 1] * bw_px) * inv_w);
        c->lm[2 * k + 1] = clamp01((y0_px + lm_ptr[2 * k + 0] * bh_px) * inv_h);
      }
    }
  }

  /* NMS */
  int n_keep = nms(scratch, n_cand, p->iou_threshold, p->max_boxes_limit);

  /* Fill output */
  out->boxes.nb_detect = n_keep;
  for (int i = 0; i < n_keep; i++) {
    candidate_t *c = &scratch[i];
    od_pp_outBuffer_t *b = &out->boxes.pOutBuff[i];
    b->x_center    = (c->x0 + c->x1) * 0.5f;
    b->y_center    = (c->y0 + c->y1) * 0.5f;
    b->width       = c->x1 - c->x0;
    b->height      = c->y1 - c->y0;
    b->conf        = c->conf;
    b->class_index = 0;

    /* Copy landmarks into the output array.
     * out->landmarks is a flexible trailing array — the caller must have
     * allocated enough room for max_boxes_limit entries. */
    float *lm_out = ((float *)out->landmarks) + i * FACE_NB_LANDMARKS * 2;
    memcpy(lm_out, c->lm, FACE_NB_LANDMARKS * 2 * sizeof(float));
  }

  return 0;
}

#endif /* POSTPROCESS_TYPE == POSTPROCESS_CUSTOM */
