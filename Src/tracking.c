/**
 ******************************************************************************
 * @file    tracking.c
 * @brief   Object tracking implementation using smoothing and IoU-based matching
 ******************************************************************************
 */

#include "tracking.h"
#include "app_config.h"
#include <string.h>

/* ========================================================================= */
/* CONFIGURATION CONSTANTS                                                   */
/* ========================================================================= */

#define TRACKER_SMOOTH_FACTOR       (0.5f)
#define TRACKER_IOU_THRESHOLD       (0.3f)
#define TRACKER_MAX_LOST_FRAMES     (5)

/* ========================================================================= */
/* UTILITY FUNCTIONS                                                         */
/* ========================================================================= */

/**
 * @brief Return minimum of two float values
 */
static inline float minf(float a, float b) 
{ 
    return a < b ? a : b; 
}

/**
 * @brief Return maximum of two float values
 */
static inline float maxf(float a, float b) 
{ 
    return a > b ? a : b; 
}

/**
 * @brief Apply exponential smoothing to bounding box coordinates
 * @param dst Destination bounding box (modified in place)
 * @param src Source bounding box with new measurements
 */
static void smooth_box(pd_pp_box_t *dst, const pd_pp_box_t *src)
{
    const float alpha = TRACKER_SMOOTH_FACTOR;
    const float beta = 1.0f - alpha;
    
    /* Smooth box center and dimensions */
    dst->x_center = dst->x_center * beta + src->x_center * alpha;
    dst->y_center = dst->y_center * beta + src->y_center * alpha;
    dst->width    = dst->width * beta + src->width * alpha;
    dst->height   = dst->height * beta + src->height * alpha;
    
    /* Smooth keypoints if available */
    if (dst->pKps && src->pKps) {
        for (uint32_t k = 0; k < AI_PD_MODEL_PP_NB_KEYPOINTS; k++) {
            dst->pKps[k].x = dst->pKps[k].x * beta + src->pKps[k].x * alpha;
            dst->pKps[k].y = dst->pKps[k].y * beta + src->pKps[k].y * alpha;
        }
    }
}

/**
 * @brief Calculate Intersection over Union (IoU) between two bounding boxes
 * @param b0 First bounding box
 * @param b1 Second bounding box
 * @return IoU value between 0.0 and 1.0
 */
float tracker_iou(const pd_pp_box_t *b0, const pd_pp_box_t *b1)
{
    /* Convert center-width format to min-max format */
    float xmin0 = b0->x_center - b0->width * 0.5f;
    float ymin0 = b0->y_center - b0->height * 0.5f;
    float xmax0 = b0->x_center + b0->width * 0.5f;
    float ymax0 = b0->y_center + b0->height * 0.5f;

    float xmin1 = b1->x_center - b1->width * 0.5f;
    float ymin1 = b1->y_center - b1->height * 0.5f;
    float xmax1 = b1->x_center + b1->width * 0.5f;
    float ymax1 = b1->y_center + b1->height * 0.5f;

    /* Calculate areas */
    float area0 = (xmax0 - xmin0) * (ymax0 - ymin0);
    float area1 = (xmax1 - xmin1) * (ymax1 - ymin1);
    
    if (area0 <= 0.0f || area1 <= 0.0f) {
        return 0.0f;
    }

    /* Calculate intersection bounds */
    float ixmin = maxf(xmin0, xmin1);
    float iymin = maxf(ymin0, ymin1);
    float ixmax = minf(xmax0, xmax1);
    float iymax = minf(ymax0, ymax1);

    /* Calculate intersection area */
    float iw = ixmax - ixmin;
    float ih = iymax - iymin;
    
    if (iw <= 0.0f || ih <= 0.0f) {
        return 0.0f;
    }

    float intersection = iw * ih;
    float union_area = area0 + area1 - intersection;
    
    return intersection / union_area;
}

/**
 * @brief Initialize tracker state
 * @param t Pointer to tracker structure
 */
void tracker_init(tracker_t *t)
{
    memset(t, 0, sizeof(*t));
    t->state = TRACK_STATE_IDLE;
    t->similarity = 0.0f;
    t->lost_count = 0;
}

/**
 * @brief Process detection results and update tracker state
 * @param t Pointer to tracker structure
 * @param det Detection results from post-processing
 * @param sim_threshold Similarity threshold for track updates
 */
void tracker_process(tracker_t *t, pd_postprocess_out_t *det, float sim_threshold)
{
    pd_pp_box_t *boxes = (pd_pp_box_t *)det->pOutData;
    uint32_t box_count = det->box_nb;
    bool track_updated = false;

    /* Process each detected box */
    for (uint32_t i = 0; i < box_count; i++) {
        pd_pp_box_t *current_box = &boxes[i];
        
        /* High confidence detection - update track */
        if (current_box->prob >= sim_threshold) {
            if (t->state == TRACK_STATE_TRACKING) {
                smooth_box(&t->box, current_box);
            } else {
                t->box = *current_box;
                t->state = TRACK_STATE_TRACKING;
            }
            t->lost_count = 0;
            track_updated = true;
            break; /* Use first high-confidence detection */
        }
        /* Lower confidence but good IoU overlap - continue tracking */
        else if (t->state == TRACK_STATE_TRACKING) {
            float iou = tracker_iou(&t->box, current_box);
            if (iou > TRACKER_IOU_THRESHOLD) {
                smooth_box(&t->box, current_box);
                t->lost_count = 0;
                track_updated = true;
                break;
            }
        }
    }

    /* Handle lost track */
    if (t->state == TRACK_STATE_TRACKING && !track_updated) {
        t->lost_count++;
        if (t->lost_count > TRACKER_MAX_LOST_FRAMES) {
            t->state = TRACK_STATE_IDLE;
            t->lost_count = 0;
            t->similarity = 0.0f;
        }
    }

    /* Add tracked box to output if actively tracking */
    if (t->state == TRACK_STATE_TRACKING && box_count < AI_PD_MODEL_PP_MAX_BOXES_LIMIT) {
        boxes[box_count] = t->box;
        boxes[box_count].prob = t->similarity;
        det->box_nb = box_count + 1;
    }
}

