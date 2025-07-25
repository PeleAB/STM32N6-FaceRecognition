/**
 ******************************************************************************
 * @file    app_frame_processing.h
 * @author  PeleAB
 * @brief   Frame processing module for breaking down main loop components
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

#ifndef APP_FRAME_PROCESSING_H
#define APP_FRAME_PROCESSING_H

#include <stdint.h>
#include <stdbool.h>
#include "app_constants.h"
#include "app_config_manager.h"
#include "app_neural_network.h"
#include "memory_pool.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= */
/* FRAME PROCESSING STRUCTURES                                               */
/* ========================================================================= */

/**
 * @brief Frame processing context
 */
typedef struct {
    /* Neural network contexts */
    face_detection_nn_t face_detection;      /**< Face detection network */
    face_recognition_nn_t face_recognition;  /**< Face recognition network */
    
    
    /* Memory management */
    memory_pool_t memory_pool;               /**< Memory pool manager */
    
    /* Configuration */
    app_config_t config;                     /**< Application configuration */
    
    /* Frame buffers */
    uint8_t *input_frame_buffer;             /**< Input frame buffer */
    uint8_t *processing_buffer;              /**< Processing buffer */
    
    /* Performance metrics */
    uint32_t frame_count;                    /**< Total processed frames */
    uint32_t detection_count;                /**< Total detections */
    uint32_t recognition_count;              /**< Total recognitions */
    float average_fps;                       /**< Average frames per second */
    uint32_t last_process_time;              /**< Last processing time */
    
    /* State management */
    bool is_initialized;                     /**< Initialization status */
} frame_processing_context_t;

/**
 * @brief Frame processing pipeline stage
 */
typedef enum {
    PIPELINE_STAGE_CAPTURE = 0,              /**< Frame capture stage */
    PIPELINE_STAGE_PREPROCESSING,            /**< Preprocessing stage */
    PIPELINE_STAGE_DETECTION,                /**< Face detection stage */
    PIPELINE_STAGE_TRACKING,                 /**< Object tracking stage */
    PIPELINE_STAGE_RECOGNITION,              /**< Face recognition stage */
    PIPELINE_STAGE_POSTPROCESSING,           /**< Post-processing stage */
    PIPELINE_STAGE_OUTPUT,                   /**< Output stage */
    PIPELINE_STAGE_COUNT
} pipeline_stage_t;

/**
 * @brief Pipeline timing information
 */
typedef struct {
    uint32_t stage_times[PIPELINE_STAGE_COUNT]; /**< Individual stage times */
    uint32_t total_time;                     /**< Total pipeline time */
    uint32_t timestamp;                      /**< Frame timestamp */
} pipeline_timing_t;

/* ========================================================================= */
/* FUNCTION PROTOTYPES                                                       */
/* ========================================================================= */

/**
 * @brief Initialize frame processing pipeline
 * @param ctx Pointer to frame processing context
 * @param config Pointer to application configuration
 * @return 0 on success, negative on error
 */
int frame_processing_init(frame_processing_context_t *ctx, const app_config_t *config);

/**
 * @brief Process single frame through complete pipeline
 * @param ctx Pointer to frame processing context
 * @param input_frame Pointer to input frame data
 * @param frame_width Frame width in pixels
 * @param frame_height Frame height in pixels
 * @param timing Pointer to store timing information
 * @return 0 on success, negative on error
 */
int frame_processing_process_frame(frame_processing_context_t *ctx,
                                  const uint8_t *input_frame,
                                  uint32_t frame_width,
                                  uint32_t frame_height,
                                  pipeline_timing_t *timing);

/**
 * @brief Initialize neural network buffers
 * @param ctx Pointer to frame processing context
 * @return 0 on success, negative on error
 */
int frame_processing_init_nn_buffers(frame_processing_context_t *ctx);

/**
 * @brief Process frame capture stage
 * @param ctx Pointer to frame processing context
 * @param input_frame Pointer to input frame data
 * @param frame_width Frame width in pixels
 * @param frame_height Frame height in pixels
 * @return 0 on success, negative on error
 */
int frame_processing_capture_stage(frame_processing_context_t *ctx,
                                  const uint8_t *input_frame,
                                  uint32_t frame_width,
                                  uint32_t frame_height);

/**
 * @brief Process preprocessing stage
 * @param ctx Pointer to frame processing context
 * @return 0 on success, negative on error
 */
int frame_processing_preprocessing_stage(frame_processing_context_t *ctx);

/**
 * @brief Process face detection stage
 * @param ctx Pointer to frame processing context
 * @param detected_faces Pointer to store detected faces
 * @param max_faces Maximum number of faces to return
 * @param face_count Pointer to store actual number of faces
 * @return 0 on success, negative on error
 */
int frame_processing_detection_stage(frame_processing_context_t *ctx,
                                    pd_pp_box_t *detected_faces,
                                    uint32_t max_faces,
                                    uint32_t *face_count);

/**
 * @brief Process tracking stage
 * @param ctx Pointer to frame processing context
 * @param detected_faces Pointer to detected faces
 * @param face_count Number of detected faces
 * @return 0 on success, negative on error
 */
int frame_processing_tracking_stage(frame_processing_context_t *ctx,
                                   const pd_pp_box_t *detected_faces,
                                   uint32_t face_count);

/**
 * @brief Process face recognition stage
 * @param ctx Pointer to frame processing context
 * @param track_id Track identifier for recognition
 * @param similarity Pointer to store similarity score
 * @return 0 on success, negative on error
 */
int frame_processing_recognition_stage(frame_processing_context_t *ctx,
                                      uint32_t track_id,
                                      float *similarity);

/**
 * @brief Process post-processing stage
 * @param ctx Pointer to frame processing context
 * @param timing Pointer to timing information
 * @return 0 on success, negative on error
 */
int frame_processing_postprocessing_stage(frame_processing_context_t *ctx,
                                         const pipeline_timing_t *timing);

/**
 * @brief Process output stage
 * @param ctx Pointer to frame processing context
 * @param timing Pointer to timing information
 * @return 0 on success, negative on error
 */
int frame_processing_output_stage(frame_processing_context_t *ctx,
                                 const pipeline_timing_t *timing);

/**
 * @brief Update performance metrics
 * @param ctx Pointer to frame processing context
 * @param timing Pointer to timing information
 * @return 0 on success, negative on error
 */
int frame_processing_update_metrics(frame_processing_context_t *ctx,
                                   const pipeline_timing_t *timing);

/**
 * @brief Clean up frame processing resources
 * @param ctx Pointer to frame processing context
 * @return 0 on success, negative on error
 */
int frame_processing_cleanup(frame_processing_context_t *ctx);

/**
 * @brief Get frame processing statistics
 * @param ctx Pointer to frame processing context
 * @param stats Pointer to store statistics
 * @return 0 on success, negative on error
 */
int frame_processing_get_statistics(const frame_processing_context_t *ctx, void *stats);

/**
 * @brief Reset frame processing context
 * @param ctx Pointer to frame processing context
 * @return 0 on success, negative on error
 */
int frame_processing_reset(frame_processing_context_t *ctx);

/**
 * @brief Validate frame processing context
 * @param ctx Pointer to frame processing context
 * @return true if valid, false otherwise
 */
bool frame_processing_validate(const frame_processing_context_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* APP_FRAME_PROCESSING_H */
