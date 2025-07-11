/**
 ******************************************************************************
 * @file    tracking.c
 * @author  Application Team
 * @brief   Object tracking implementation using smoothing and IoU-based matching
 * @details This module provides single-object tracking capabilities using
 *          exponential smoothing and Intersection over Union (IoU) based
 *          association. The tracker maintains object state and handles
 *          detection-to-track association with configurable thresholds.
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

#include "tracking.h"
#include "app_config.h"
#include "app_constants.h"
#include <string.h>

/* ========================================================================= */
/* CONFIGURATION CONSTANTS                                                   */
/* ========================================================================= */

/* Use constants from app_constants.h */
#define TRACKER_SMOOTH_FACTOR       TRACKER_SMOOTH_FACTOR
#define TRACKER_IOU_THRESHOLD       TRACKER_IOU_THRESHOLD
#define TRACKER_MAX_LOST_FRAMES     TRACKER_MAX_LOST_FRAMES

/* ========================================================================= */
/* UTILITY FUNCTIONS                                                         */
/* ========================================================================= */

/**
 * @brief Return minimum of two float values
 * @param a First value
 * @param b Second value
 * @return Minimum of a and b
 */
static inline float minf(float a, float b) 
{ 
    return a < b ? a : b; 
}

/**
 * @brief Return maximum of two float values
 * @param a First value
 * @param b Second value
 * @return Maximum of a and b
 */
static inline float maxf(float a, float b) 
{ 
    return a > b ? a : b; 
}

/**
 * @brief Apply exponential smoothing to bounding box coordinates
 * @details Uses exponential smoothing to reduce noise in tracking by combining
 *          previous track state with new detection measurements. The smoothing
 *          factor determines how much weight is given to new measurements.
 * @param dst Destination bounding box (modified in place)
 * @param src Source bounding box with new measurements
 * @note The smoothing factor is configurable via TRACKER_SMOOTH_FACTOR
 * @see TRACKER_SMOOTH_FACTOR
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
 * @details Computes the IoU metric which measures the overlap between two
 *          bounding boxes. IoU is defined as the area of intersection divided
 *          by the area of union. Values range from 0.0 (no overlap) to 1.0
 *          (perfect overlap).
 * @param b0 First bounding box in center-width format
 * @param b1 Second bounding box in center-width format
 * @return IoU value between 0.0 and 1.0
 * @note Input boxes are expected in normalized coordinates (0.0 to 1.0)
 * @warning Returns 0.0 if either box has zero or negative area
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
 * @brief Initialize tracker state to default values
 * @details Resets the tracker to idle state with zero similarity score
 *          and lost count. This should be called before using the tracker
 *          for the first time or when resetting tracking.
 * @param t Pointer to tracker structure to initialize
 * @pre t must be a valid pointer to a tracker_t structure
 * @post Tracker state is set to TRACK_STATE_IDLE
 * @post All tracking metrics are reset to zero
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
 * @details Main tracking function that processes detection results and updates
 *          the tracker state. Implements a simple single-object tracking algorithm
 *          using confidence-based association and IoU-based matching for lower
 *          confidence detections.
 * 
 * Algorithm:
 * 1. For each detection with confidence >= sim_threshold:
 *    - If tracking: smooth the track with new detection
 *    - If not tracking: initialize new track
 * 2. For lower confidence detections:
 *    - If tracking: check IoU overlap with current track
 *    - If IoU > threshold: smooth the track
 * 3. If no matching detection found:
 *    - Increment lost count
 *    - If lost count > max: reset tracker to idle
 * 4. If actively tracking: add tracked box to output
 * 
 * @param t Pointer to tracker structure
 * @param det Detection results from post-processing pipeline
 * @param sim_threshold Confidence threshold for high-confidence track updates
 * @pre t must be a valid pointer to initialized tracker_t structure
 * @pre det must contain valid detection results
 * @pre sim_threshold should be in range [0.0, 1.0]
 * @post Tracker state is updated based on detection results
 * @post If tracking, adds tracked box to detection output
 * @note Modifies det->box_nb if adding tracked box to output
 * @see tracker_iou() for IoU calculation
 * @see smooth_box() for smoothing implementation
 */
void tracker_process(tracker_t *t, pd_postprocess_out_t *det, float sim_threshold)
{
    pd_pp_box_t *boxes = (pd_pp_box_t *)det->pOutData;
    uint32_t box_count = det->box_nb;
    bool track_updated = false;

    /* Process each detected box for potential track association */
    for (uint32_t i = 0; i < box_count; i++) {
        pd_pp_box_t *current_box = &boxes[i];
        
        /* High confidence detection - direct track update */
        if (current_box->prob >= sim_threshold) {
            if (t->state == TRACK_STATE_TRACKING) {
                /* Smooth existing track with new high-confidence detection */
                smooth_box(&t->box, current_box);
            } else {
                /* Initialize new track with high-confidence detection */
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
                /* Smooth track with spatially consistent detection */
                smooth_box(&t->box, current_box);
                t->lost_count = 0;
                track_updated = true;
                break;
            }
        }
    }

    /* Handle lost track - no matching detection found */
    if (t->state == TRACK_STATE_TRACKING && !track_updated) {
        t->lost_count++;
        /* Reset tracker if track lost for too many consecutive frames */
        if (t->lost_count > TRACKER_MAX_LOST_FRAMES) {
            t->state = TRACK_STATE_IDLE;
            t->lost_count = 0;
            t->similarity = 0.0f;
        }
    }

    /* Add tracked box to output if actively tracking and space available */
    if (t->state == TRACK_STATE_TRACKING && box_count < AI_PD_MODEL_PP_MAX_BOXES_LIMIT) {
        boxes[box_count] = t->box;
        boxes[box_count].prob = t->similarity; /* Use face recognition similarity */
        det->box_nb = box_count + 1;
    }
}

