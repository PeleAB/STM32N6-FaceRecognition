#ifndef PD_ANCHORS_H
#define PD_ANCHORS_H

#include "pd_pp_output_if.h"
#include "blazeface_anchors.h"

typedef struct {
    float32_t x;
    float32_t y;
    float32_t w;
    float32_t h;
} pd_anchor_t;

extern pd_anchor_t g_Anchors[BLAZEFACE_NUM_ANCHORS];
void pd_anchor_init(void);

#endif /* PD_ANCHORS_H */
