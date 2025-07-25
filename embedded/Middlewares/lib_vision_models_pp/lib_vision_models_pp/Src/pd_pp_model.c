/**
  ******************************************************************************
  * @file    pd_pp_model.c
  * @author  MDG Application Team
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

#include "pd_model_pp_if.h"
#include "vision_models_pp.h"
#include "pd_pp_loc.h"


static int32_t pd_pp_nms_comparator(const void *arg1, const void *arg2)
{
  const pd_pp_box_t *box1 = arg1;
  const pd_pp_box_t *box2 = arg2;

  if (box1->prob < box2->prob)
    return 1;

  if (box1->prob > box2->prob)
    return -1;

  return 0;
}

static void pd_pp_compute_opposite_corners_from_box(pd_pp_box_t *box, pd_pp_point_t corners[2])
{
  corners[0].x = box->x_center - box->width / 2;
  corners[0].y = box->y_center - box->height / 2;
  corners[1].x = box->x_center + box->width / 2;
  corners[1].y = box->y_center + box->height / 2;
}

static float pd_pp_compute_iou(pd_pp_box_t *box0, pd_pp_box_t *box1)
{
  pd_pp_point_t corners[2][2];
  float32_t intersect_xmin;
  float32_t intersect_ymin;
  float32_t intersect_xmax;
  float32_t intersect_ymax;
  float32_t intersect_area;
  float32_t area[2];
  float32_t xmin[2];
  float32_t xmax[2];
  float32_t ymin[2];
  float32_t ymax[2];
  int i;

  pd_pp_compute_opposite_corners_from_box(box0, corners[0]);
  pd_pp_compute_opposite_corners_from_box(box1, corners[1]);

  for (i = 0; i < 2; i++) {
    xmin[i] = MIN(corners[i][0].x, corners[i][1].x);
    xmax[i] = MAX(corners[i][0].x, corners[i][1].x);
    ymin[i] = MIN(corners[i][0].y, corners[i][1].y);
    ymax[i] = MAX(corners[i][0].y, corners[i][1].y);
  }

  for (i = 0; i < 2; i++) {
    area[i] = (ymax[i] - ymin[i]) * (xmax[i] - xmin[i]);
  }
  if (area[0] <= 0 || area[1] <= 0)
    return 0;

  intersect_xmin = MAX(xmin[0], xmin[1]);
  intersect_ymin = MAX(ymin[0], ymin[1]);
  intersect_xmax = MIN(xmax[0], xmax[1]);
  intersect_ymax = MIN(ymax[0], ymax[1]);

  intersect_area = MAX(intersect_ymax - intersect_ymin, 0.0f) * MAX(intersect_xmax - intersect_xmin, 0.0f);

  return intersect_area / (area[0] + area[1] - intersect_area);
}
static int32_t pd_pp_decode(pd_model_pp_in_t *pInput,
                                     pd_postprocess_out_t *pOutput,
                                     pd_model_pp_static_param_t *pInput_static_param) {

  pOutput->box_nb = 0;

  pd_pp_box_t *pBoxes = (pd_pp_box_t *)pOutput->pOutData;
  float32_t width  = pInput_static_param->width;
  float32_t height = pInput_static_param->height;
  const int grid = 32;
  size_t box_nb = 0;

  float32_t *pScale = pInput->pScale;
  float32_t *pLms = pInput->pLms;
  float32_t *pHeatmap = pInput->pHeatmap;
  float32_t *pOffset = pInput->pOffset;

  for (int y = 0; y < grid; y++) {
    for (int x = 0; x < grid; x++) {
      int index_h = (y * grid + x);
      float32_t score = pHeatmap[index_h];
      if (score > pInput_static_param->conf_threshold) {
        float32_t s0 = expf(pScale[(index_h * 2) + 0]) * 4.0f;
        float32_t s1 = expf(pScale[(index_h * 2) + 1]) * 4.0f;
        float32_t o0 = pOffset[(index_h * 2) + 0];
        float32_t o1 = pOffset[(index_h * 2) + 1];

        float32_t x1 = (x + o1 + 0.5f) * 4.0f - s1 / 2.0f;
        float32_t y1 = (y + o0 + 0.5f) * 4.0f - s0 / 2.0f;
        if (x1 < 0) x1 = 0;
        if (y1 < 0) y1 = 0;
        float32_t x2 = x1 + s1;
        float32_t y2 = y1 + s0;

        pd_pp_box_t *pBox = &pBoxes[box_nb];
        pBox->prob = score;
        pBox->x_center = ((x1 + x2) * 0.5f) / width;
        pBox->y_center = ((y1 + y2) * 0.5f) / height;
        pBox->width  = s1 / width;
        pBox->height = s0 / height;

        for (uint32_t j = 0; j < pInput_static_param->nb_keypoints; j++) {
          float32_t lm_y = pLms[index_h * pInput_static_param->nb_keypoints * 2 + j * 2 + 0];
          float32_t lm_x = pLms[index_h * pInput_static_param->nb_keypoints * 2 + j * 2 + 1];
          pBox->pKps[j].x = (lm_x * s1 + x1) / width;
          pBox->pKps[j].y = (lm_y * s0 + y1) / height;
        }

        box_nb++;
        if (box_nb >= pInput_static_param->max_boxes_limit) {
          pOutput->box_nb = box_nb;
          return AI_PD_POSTPROCESS_ERROR_NO;
        }
      }
    }
  }

  pOutput->box_nb = box_nb;
  return AI_PD_POSTPROCESS_ERROR_NO;
}

static int pd_pp_nms(pd_postprocess_out_t *pOutput,
                     pd_model_pp_static_param_t *pInput_static_param)
{
  int hand_nb = 0;
  int skip_box;
  float iou;
  pd_pp_box_t *pd_boxes = (pd_pp_box_t *)pOutput->pOutData;
  size_t box_nb =  pOutput->box_nb;

  /* first sort boxes by higher probability */
  qsort(pd_boxes, box_nb, sizeof(pd_pp_box_t), pd_pp_nms_comparator);

  /* then apply iou to filter them */
  for (size_t i = 0; i < box_nb; i++) {
    skip_box = 0;
    for (int j = 0; j < hand_nb; j++) {
      iou = pd_pp_compute_iou(&pd_boxes[i], &pd_boxes[j]);
      if (iou >= pInput_static_param->iou_threshold) {
        skip_box = 1;
        break;
      }
    }
    if (skip_box)
      continue;

    pd_boxes[hand_nb++] = pd_boxes[i];
  }

  /* update the output count to reflect the filtered detections */
  pOutput->box_nb = hand_nb;

  return hand_nb;
}
int32_t pd_model_pp_reset(pd_model_pp_static_param_t *pInput_static_param)
{
  return AI_PD_POSTPROCESS_ERROR_NO;
}


int32_t pd_model_pp_process(pd_model_pp_in_t *pInput,
                            pd_postprocess_out_t *pOutput,
                            pd_model_pp_static_param_t *pInput_static_param)
{
  int32_t ret = AI_PD_POSTPROCESS_ERROR;
  ret = pd_pp_decode(pInput,
                     pOutput,
                     pInput_static_param);

  if (AI_PD_POSTPROCESS_ERROR_NO != ret) {
    return ret;
  }
  pd_pp_nms(pOutput,
            pInput_static_param);

  return ret;
}
