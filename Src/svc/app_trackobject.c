 /**
 ******************************************************************************
 * @file    app_trackobject.c
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

#include "app_trackobject.h"
#include "stm32n6xx_hal.h"

#include <assert.h>

/* Be sure APP_SCORE_THRESHOLD_TRUST_FLOAT is a float. In case user forgot to specify float value */
#define APP_SCORE_THRESHOLD_TRUST_FLOAT ((float)APP_SCORE_THRESHOLD)

static float score_matrix[APP_MAX_OBJECT_TRACKING][APP_MAX_OBJECT_DETECT];
static TrackObject_s track_objects[APP_MAX_OBJECT_TRACKING];

static int to_is_tracking(TrackObject_s *o)
{
  return o->id;
}

static int to_is_valid(TrackObject_s *o)
{
  return o->id && o->is_dbox_valid;
}

static int to_too_old(TrackObject_s *o)
{
  return (HAL_GetTick() - o->last_update) >= APP_LOST_TIME_IN_MS;
}

static TrackObject_s *to_find_free()
{
  int i;

  for (i = 0; i < APP_MAX_OBJECT_TRACKING; i++)
  {
    if (to_is_tracking(&track_objects[i]))
    {
      continue;
    }
    return &track_objects[i];
  }

  return NULL;
}

static void to_init(od_pp_outBuffer_t *dbox, uint8_t *features)
{
  static int tracking_id = 0;
  TrackObject_s *o = to_find_free();

  if (dbox->conf < APP_START_TRACKING_CONF_THRESHOLD)
  {
    return ;
  }

  if (!o)
  {
    return ;
  }

  o->id = ++tracking_id;
  o->is_dbox_valid = 1;
  o->dbox = *dbox;
  memcpy(o->features, features, NN_REID_NB_FEATURES);
  o->last_update = HAL_GetTick();
}

static void to_update(TrackObject_s *o, od_pp_outBuffer_t *dbox, uint8_t *features)
{
  /* If not tracking, nothing to do */
  if (!to_is_tracking(o))
  {
    return;
  }

  if (dbox)
  {
    o->is_dbox_valid = 1;
    o->dbox = *dbox;
    memcpy(o->features, features, NN_REID_NB_FEATURES);
    o->last_update = HAL_GetTick();
  }
  else
  {
    o->is_dbox_valid = 0;
  }

  if (to_too_old(o))
  {
    o->id = 0;
  }
}

static float compute_score(TrackObject_s *o, uint8_t features[NN_REID_NB_FEATURES])
{
  float sum_ab = 0;
  float sum_aa = 0;
  float sum_bb = 0;
  int i;

  for (i = 0; i < NN_REID_NB_FEATURES; i++)
  {
    sum_ab += o->features[i] * features[i];
    sum_aa += o->features[i] * o->features[i];
    sum_bb += features[i] * features[i];
  }

  return 1 - sum_ab / sqrtf(sum_aa * sum_bb);
}

static void compute_score_matrix(od_pp_out_t *pp_out, uint8_t reid_features[APP_MAX_OBJECT_DETECT][NN_REID_NB_FEATURES])
{
  TrackObject_s *o;
  int r, c;

  /* Init app_scores to default value */
  for (r = 0; r < APP_MAX_OBJECT_TRACKING; r++)
  {
    for (c = 0; c < APP_MAX_OBJECT_DETECT; c++)
    {
      score_matrix[r][c] = APP_SCORE_THRESHOLD_TRUST_FLOAT;
    }
  }

  /* compute score matrix */
  for (r = 0; r < APP_MAX_OBJECT_TRACKING; r++)
  {
    o = &track_objects[r];
    if (!to_is_tracking(o))
    {
      continue;
    }
    for (c = 0; c < pp_out->nb_detect; c++)
    {
      score_matrix[r][c] = compute_score(o, reid_features[c]);
    }
  }
}

static float assign_dbox_to_obj(int *r, int *c)
{
  float min_score = APP_SCORE_THRESHOLD_TRUST_FLOAT;
  int i, j;

  /* Find max score and pos */
  for (j = 0; j < APP_MAX_OBJECT_TRACKING; j++)
  {
    for (i = 0; i < APP_MAX_OBJECT_DETECT; i++)
    {
      if (score_matrix[j][i] >= min_score)
      {
        continue;
      }
      min_score = score_matrix[j][i];
      *r = j;
      *c = i;
    }
  }

  /* Update app_scores matrix to no more select track object and dbox */
  if (min_score < APP_SCORE_THRESHOLD_TRUST_FLOAT)
  {
    for (j = 0; j < APP_MAX_OBJECT_TRACKING; j++)
    {
      score_matrix[j][*c] = APP_SCORE_THRESHOLD_TRUST_FLOAT;
    }
    for (i = 0; i < APP_MAX_OBJECT_DETECT; i++)
    {
      score_matrix[*r][i] = APP_SCORE_THRESHOLD_TRUST_FLOAT;
    }
  }

  return min_score;
}

static void assign_or_create(od_pp_out_t *pp_out, uint8_t reid_features[APP_MAX_OBJECT_DETECT][NN_REID_NB_FEATURES])
{
  /* Global since it depends of user input that may explode stack */
  static int is_obj_match[APP_MAX_OBJECT_TRACKING];
  static int is_dbox_match[APP_MAX_OBJECT_DETECT];
  float score;
  int r, c;
  int i;

  r = c = -1;
  memset(is_obj_match, 0, sizeof(is_obj_match));
  memset(is_dbox_match, 0, sizeof(is_dbox_match));
  /* assign dbox to currently tracks objects */
  while (1)
  {
    /* Get current minimal score in app_scores array and return position */
    score = assign_dbox_to_obj(&r, &c);
    if (score >= APP_SCORE_THRESHOLD_TRUST_FLOAT)
    {
      break;
    }
    assert(r >= 0);
    assert(c >= 0);
    to_update(&track_objects[r], &pp_out->pOutBuff[c], reid_features[c]);
    is_obj_match[r] = 1;
    is_dbox_match[c] = 1;
  }

  /* Update obj which match no dbox */
  for (i = 0; i < APP_MAX_OBJECT_TRACKING; i++)
  {
    if (is_obj_match[i])
    {
      continue;
    }
    to_update(&track_objects[i], NULL, NULL);
  }

  /* Init new tracking object with remaining detected box */
  for (i = 0; i < pp_out->nb_detect; i++)
  {
    if (is_dbox_match[i])
    {
      continue;
    }
    to_init(&pp_out->pOutBuff[i], reid_features[i]);
  }
}

/* Public API */
void TrackObject_UpdateAll(od_pp_out_t *pp_out, uint8_t reid_features[APP_MAX_OBJECT_DETECT][NN_REID_NB_FEATURES])
{
  compute_score_matrix(pp_out, reid_features);
  assign_or_create(pp_out, reid_features);
}

void TrackObject_DisplayIterator_Init(TO_DisplayIterator_s *it)
{
  it->idx = 0;
}

TrackObject_s * TrackObject_DisplayIterator_GetNext(TO_DisplayIterator_s *it)
{
  TrackObject_s *to;

  while (it->idx < APP_MAX_OBJECT_TRACKING)
  {
    to = &track_objects[it->idx++];

    if (to_is_valid(to))
      return to;
  }

  return NULL;
}
