/**
 ******************************************************************************
 * @file    enhanced_tracking.h
 * @author  Application Team
 * @brief   Enhanced multi-object tracking with Kalman filtering and improved algorithms
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

#ifndef ENHANCED_TRACKING_H
#define ENHANCED_TRACKING_H

#include <stdint.h>
#include <stdbool.h>
#include "pd_pp_output_if.h"
#include "app_constants.h"
#include "app_config_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= */
/* TRACKING CONSTANTS                                                        */
/* ========================================================================= */
#define MAX_TRACKED_OBJECTS         16  /**< Maximum number of tracked objects */
#define KALMAN_STATE_SIZE           8   /**< Kalman filter state size (x,y,w,h,vx,vy,vw,vh) */
#define KALMAN_MEASUREMENT_SIZE     4   /**< Kalman filter measurement size (x,y,w,h) */
#define TRACK_HISTORY_SIZE          10  /**< Track history buffer size */

/* ========================================================================= */
/* TRACKING ENUMERATIONS                                                     */
/* ========================================================================= */

/**
 * @brief Track state enumeration
 */
typedef enum {
    TRACK_STATE_IDLE = 0,           /**< Track is idle/uninitialized */
    TRACK_STATE_TENTATIVE,          /**< Track is tentative (low confidence) */
    TRACK_STATE_CONFIRMED,          /**< Track is confirmed (high confidence) */
    TRACK_STATE_LOST,               /**< Track is temporarily lost */
    TRACK_STATE_DELETED             /**< Track is deleted */
} enhanced_track_state_t;

/**
 * @brief Track association method
 */
typedef enum {
    ASSOCIATION_METHOD_IOU = 0,     /**< Intersection over Union */
    ASSOCIATION_METHOD_CENTROID,    /**< Centroid distance */
    ASSOCIATION_METHOD_KALMAN,      /**< Kalman filter prediction */
    ASSOCIATION_METHOD_HUNGARIAN    /**< Hungarian algorithm */
} association_method_t;

/* ========================================================================= */
/* TRACKING STRUCTURES                                                       */
/* ========================================================================= */

/**
 * @brief Kalman filter state for tracking
 */
typedef struct {
    float state[KALMAN_STATE_SIZE];           /**< State vector [x,y,w,h,vx,vy,vw,vh] */
    float covariance[KALMAN_STATE_SIZE][KALMAN_STATE_SIZE]; /**< Covariance matrix */
    float process_noise;                      /**< Process noise parameter */
    float measurement_noise;                  /**< Measurement noise parameter */
    bool is_initialized;                      /**< Initialization flag */
} kalman_filter_t;

/**
 * @brief Track history point
 */
typedef struct {
    pd_pp_box_t box;                          /**< Bounding box */
    float confidence;                         /**< Detection confidence */
    float similarity;                         /**< Face similarity score */
    uint32_t timestamp;                       /**< Timestamp */
} track_history_point_t;

/**
 * @brief Enhanced track structure
 */
typedef struct {
    uint32_t track_id;                        /**< Unique track identifier */
    pd_pp_box_t current_box;                  /**< Current bounding box */
    pd_pp_box_t predicted_box;                /**< Predicted bounding box */
    enhanced_track_state_t state;             /**< Current track state */
    
    /* Kalman filter for motion prediction */
    kalman_filter_t kalman_filter;            /**< Kalman filter state */
    
    /* Track statistics */
    uint32_t age;                             /**< Track age in frames */
    uint32_t hit_count;                       /**< Number of successful detections */
    uint32_t lost_count;                      /**< Number of consecutive missed detections */
    uint32_t tentative_count;                 /**< Number of tentative detections */
    
    /* Quality metrics */
    float average_confidence;                 /**< Average detection confidence */
    float best_similarity;                    /**< Best face similarity score */
    float current_similarity;                 /**< Current face similarity score */
    float velocity_magnitude;                 /**< Current velocity magnitude */
    
    /* Track history */
    track_history_point_t history[TRACK_HISTORY_SIZE]; /**< Track history buffer */
    uint32_t history_index;                   /**< Current history index */
    uint32_t history_count;                   /**< Number of history points */
    
    /* Timestamps */
    uint32_t creation_time;                   /**< Track creation timestamp */
    uint32_t last_update_time;                /**< Last update timestamp */
    uint32_t last_seen_time;                  /**< Last seen timestamp */
    
    /* Flags */
    bool is_face_verified;                    /**< Face verification status */
    bool needs_reverification;                /**< Needs face re-verification */
    bool is_occluded;                         /**< Occlusion status */
} enhanced_track_t;

/**
 * @brief Enhanced multi-object tracker
 */
typedef struct {
    enhanced_track_t tracks[MAX_TRACKED_OBJECTS];  /**< Track array */
    uint32_t track_count;                     /**< Number of active tracks */
    uint32_t next_track_id;                   /**< Next available track ID */
    
    /* Configuration */
    tracking_config_t config;                 /**< Tracking configuration */
    association_method_t association_method;  /**< Association method */
    
    /* Performance metrics */
    uint32_t total_tracks_created;            /**< Total tracks created */
    uint32_t total_tracks_deleted;            /**< Total tracks deleted */
    uint32_t total_associations;              /**< Total successful associations */
    uint32_t total_missed_associations;       /**< Total missed associations */
    
    /* Frame statistics */
    uint32_t frame_count;                     /**< Total processed frames */
    uint32_t last_process_time;               /**< Last processing time */
    
    /* Internal state */
    bool is_initialized;                      /**< Initialization status */
} enhanced_tracker_t;

/**
 * @brief Track association result
 */
typedef struct {
    int track_index;                          /**< Track index (-1 if no match) */
    int detection_index;                      /**< Detection index (-1 if no match) */
    float association_cost;                   /**< Association cost/distance */
    bool is_valid;                            /**< Association validity */
} track_association_t;

/* ========================================================================= */
/* FUNCTION PROTOTYPES                                                       */
/* ========================================================================= */

/**
 * @brief Initialize enhanced tracker
 * @param tracker Pointer to tracker structure
 * @param config Pointer to tracking configuration
 * @return 0 on success, negative on error
 */
int enhanced_tracker_init(enhanced_tracker_t *tracker, const tracking_config_t *config);

/**
 * @brief Process detections with enhanced tracking
 * @param tracker Pointer to tracker structure
 * @param detections Pointer to detection results
 * @param detection_count Number of detections
 * @param frame_timestamp Current frame timestamp
 * @return 0 on success, negative on error
 */
int enhanced_tracker_process(enhanced_tracker_t *tracker,
                            const pd_pp_box_t *detections,
                            uint32_t detection_count,
                            uint32_t frame_timestamp);

/**
 * @brief Get confirmed tracks
 * @param tracker Pointer to tracker structure
 * @param tracks Pointer to store track array
 * @param max_tracks Maximum number of tracks to return
 * @param track_count Pointer to store actual number of tracks
 * @return 0 on success, negative on error
 */
int enhanced_tracker_get_tracks(const enhanced_tracker_t *tracker,
                               enhanced_track_t *tracks,
                               uint32_t max_tracks,
                               uint32_t *track_count);

/**
 * @brief Get track by ID
 * @param tracker Pointer to tracker structure
 * @param track_id Track identifier
 * @return Pointer to track or NULL if not found
 */
enhanced_track_t *enhanced_tracker_get_track_by_id(const enhanced_tracker_t *tracker,
                                                   uint32_t track_id);

/**
 * @brief Update track similarity score
 * @param tracker Pointer to tracker structure
 * @param track_id Track identifier
 * @param similarity Similarity score
 * @return 0 on success, negative on error
 */
int enhanced_tracker_update_similarity(enhanced_tracker_t *tracker,
                                      uint32_t track_id,
                                      float similarity);

/**
 * @brief Mark track as face verified
 * @param tracker Pointer to tracker structure
 * @param track_id Track identifier
 * @param is_verified Verification status
 * @return 0 on success, negative on error
 */
int enhanced_tracker_set_face_verified(enhanced_tracker_t *tracker,
                                      uint32_t track_id,
                                      bool is_verified);

/**
 * @brief Calculate IoU between two bounding boxes
 * @param box1 First bounding box
 * @param box2 Second bounding box
 * @return IoU value (0.0 to 1.0)
 */
float enhanced_tracker_calculate_iou(const pd_pp_box_t *box1, const pd_pp_box_t *box2);

/**
 * @brief Calculate centroid distance between two bounding boxes
 * @param box1 First bounding box
 * @param box2 Second bounding box
 * @return Normalized centroid distance
 */
float enhanced_tracker_calculate_centroid_distance(const pd_pp_box_t *box1, const pd_pp_box_t *box2);

/**
 * @brief Predict track position using Kalman filter
 * @param track Pointer to track structure
 * @param predicted_box Pointer to store predicted box
 * @return 0 on success, negative on error
 */
int enhanced_tracker_predict_position(enhanced_track_t *track, pd_pp_box_t *predicted_box);

/**
 * @brief Update track with new detection
 * @param track Pointer to track structure
 * @param detection Pointer to detection box
 * @param confidence Detection confidence
 * @param timestamp Current timestamp
 * @return 0 on success, negative on error
 */
int enhanced_tracker_update_track(enhanced_track_t *track,
                                 const pd_pp_box_t *detection,
                                 float confidence,
                                 uint32_t timestamp);

/**
 * @brief Get tracker performance statistics
 * @param tracker Pointer to tracker structure
 * @param stats Pointer to store statistics
 * @return 0 on success, negative on error
 */
int enhanced_tracker_get_statistics(const enhanced_tracker_t *tracker,
                                   void *stats);

/**
 * @brief Reset tracker state
 * @param tracker Pointer to tracker structure
 * @return 0 on success, negative on error
 */
int enhanced_tracker_reset(enhanced_tracker_t *tracker);

/**
 * @brief Clean up expired tracks
 * @param tracker Pointer to tracker structure
 * @param current_time Current timestamp
 * @return Number of tracks cleaned up
 */
int enhanced_tracker_cleanup_expired_tracks(enhanced_tracker_t *tracker, uint32_t current_time);

/**
 * @brief Initialize Kalman filter for track
 * @param kalman Pointer to Kalman filter structure
 * @param initial_box Initial bounding box
 * @param process_noise Process noise parameter
 * @param measurement_noise Measurement noise parameter
 * @return 0 on success, negative on error
 */
int kalman_filter_init(kalman_filter_t *kalman,
                      const pd_pp_box_t *initial_box,
                      float process_noise,
                      float measurement_noise);

/**
 * @brief Predict next state using Kalman filter
 * @param kalman Pointer to Kalman filter structure
 * @param predicted_box Pointer to store predicted box
 * @return 0 on success, negative on error
 */
int kalman_filter_predict(kalman_filter_t *kalman, pd_pp_box_t *predicted_box);

/**
 * @brief Update Kalman filter with measurement
 * @param kalman Pointer to Kalman filter structure
 * @param measurement Measurement box
 * @return 0 on success, negative on error
 */
int kalman_filter_update(kalman_filter_t *kalman, const pd_pp_box_t *measurement);

#ifdef __cplusplus
}
#endif

#endif /* ENHANCED_TRACKING_H */