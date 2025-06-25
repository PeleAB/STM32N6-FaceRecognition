#include "tracking.h"
#include "app_config.h"
#include <string.h>

static float minf(float a, float b) { return a < b ? a : b; }
static float maxf(float a, float b) { return a > b ? a : b; }

float tracker_iou(const pd_pp_box_t *b0, const pd_pp_box_t *b1)
{
  float xmin0 = b0->x_center - b0->width / 2.f;
  float ymin0 = b0->y_center - b0->height / 2.f;
  float xmax0 = b0->x_center + b0->width / 2.f;
  float ymax0 = b0->y_center + b0->height / 2.f;

  float xmin1 = b1->x_center - b1->width / 2.f;
  float ymin1 = b1->y_center - b1->height / 2.f;
  float xmax1 = b1->x_center + b1->width / 2.f;
  float ymax1 = b1->y_center + b1->height / 2.f;

  float area0 = (xmax0 - xmin0) * (ymax0 - ymin0);
  float area1 = (xmax1 - xmin1) * (ymax1 - ymin1);
  if (area0 <= 0.f || area1 <= 0.f)
    return 0.f;

  float ixmin = maxf(xmin0, xmin1);
  float iymin = maxf(ymin0, ymin1);
  float ixmax = minf(xmax0, xmax1);
  float iymax = minf(ymax0, ymax1);

  float iw = ixmax - ixmin;
  float ih = iymax - iymin;
  if (iw <= 0.f || ih <= 0.f)
    return 0.f;

  float inter = iw * ih;
  return inter / (area0 + area1 - inter);
}

void tracker_init(tracker_t *t)
{
  memset(t, 0, sizeof(*t));
  t->state = TRACK_STATE_IDLE;
}

void tracker_process(tracker_t *t, pd_postprocess_out_t *det, float sim_threshold)
{
  pd_pp_box_t *boxes = det->pOutData;
  uint32_t nb = det->box_nb;
  int updated = 0;

  for (uint32_t i = 0; i < nb; i++)
  {
    pd_pp_box_t *b = &boxes[i];
    if (b->prob >= sim_threshold)
    {
      t->box = *b;
      t->state = TRACK_STATE_TRACKING;
      t->lost_count = 0;
      updated = 1;
    }
    else if (t->state == TRACK_STATE_TRACKING)
    {
      if (tracker_iou(&t->box, b) > 0.3f)
      {
        t->box = *b;
        t->lost_count = 0;
        updated = 1;
      }
    }
  }

  if (t->state == TRACK_STATE_TRACKING && !updated)
  {
    if (++t->lost_count > 5)
    {
      t->state = TRACK_STATE_IDLE;
      t->lost_count = 0;
    }
  }

  if (t->state == TRACK_STATE_TRACKING)
  {
    if (nb < AI_PD_MODEL_PP_MAX_BOXES_LIMIT)
    {
      boxes[nb] = t->box;
      det->box_nb = nb + 1;
    }
  }
}

