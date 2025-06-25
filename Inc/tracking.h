#ifndef TRACKING_H
#define TRACKING_H

#include "pd_pp_output_if.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  TRACK_STATE_IDLE = 0,
  TRACK_STATE_TRACKING
} track_state_t;

typedef struct {
  pd_pp_box_t box;
  track_state_t state;
  uint32_t lost_count;
} tracker_t;

void tracker_init(tracker_t *t);
void tracker_process(tracker_t *t, pd_postprocess_out_t *det, float sim_threshold);
float tracker_iou(const pd_pp_box_t *b0, const pd_pp_box_t *b1);

#ifdef __cplusplus
}
#endif

#endif /* TRACKING_H */
