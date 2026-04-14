 /**
 ******************************************************************************
 * @file    app_trackobject.h
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
#ifndef APP_TRACKOBJECT
#define APP_TRACKOBJECT

#include <stdint.h>

#include "app_config.h"
#include "od_pp_output_if.h"
#include "stai_reid.h"

#define NN_REID_NB_FEATURES                  (STAI_REID_OUT_1_SIZE)

typedef struct
{
  int id;
  int is_dbox_valid;
  od_pp_outBuffer_t dbox;
  uint8_t features[NN_REID_NB_FEATURES];
  uint32_t last_update;
} TrackObject_s;

typedef struct
{
  int idx;
} TO_DisplayIterator_s;

void TrackObject_UpdateAll(od_pp_out_t *pp_out, uint8_t reid_features[APP_MAX_OBJECT_DETECT][NN_REID_NB_FEATURES]);
void TrackObject_DisplayIterator_Init(TO_DisplayIterator_s *it);
TrackObject_s * TrackObject_DisplayIterator_GetNext(TO_DisplayIterator_s *it);

#endif
