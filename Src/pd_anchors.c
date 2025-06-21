#include "pd_anchors.h"

pd_pp_point_t g_Anchors[BLAZEFACE_NUM_ANCHORS];

void pd_anchor_init(void)
{
    for (int i = 0; i < BLAZEFACE_NUM_ANCHORS; i++) {
        g_Anchors[i].x = BLAZEFACE_ANCHORS[BLAZEFACE_ANCHOR_DIM * i];
        g_Anchors[i].y = BLAZEFACE_ANCHORS[BLAZEFACE_ANCHOR_DIM * i + 1];
    }
}
