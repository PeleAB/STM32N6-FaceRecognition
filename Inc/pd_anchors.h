#ifndef PD_ANCHORS_H
#define PD_ANCHORS_H

#include "pd_model_pp_if.h"
#include "blazeface_anchors.h"

extern pd_anchor_t g_Anchors[BLAZEFACE_NUM_ANCHORS];
void pd_anchor_init(void);

#endif /* PD_ANCHORS_H */
