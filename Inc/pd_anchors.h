#ifndef PD_ANCHORS_H
#define PD_ANCHORS_H

#include "pd_pp_output_if.h"
#include "blazeface_anchors.h"

extern pd_pp_point_t g_Anchors[BLAZEFACE_NUM_ANCHORS];
void pd_anchor_init(void);

#endif /* PD_ANCHORS_H */
