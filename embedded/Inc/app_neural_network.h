/**
 ******************************************************************************
 * @file    app_neural_network.h
 * @author  PeleAB
 * @brief   Neural network processing module for face detection and recognition
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

#ifndef APP_NEURAL_NETWORK_H
#define APP_NEURAL_NETWORK_H

#include <stdint.h>
#include <stdbool.h>
#include "app_constants.h"
#include "app_config_manager.h"
#include "app_postprocess.h"
#include "memory_pool.h"

/* ========================================================================= */
/* NEURAL NETWORK CONTEXT STRUCTURE                                          */
/* ========================================================================= */

/**
 * @brief Neural network buffer information
 */
typedef struct {
    void *input_buffer;                 /**< Input buffer pointer */
    void *output_buffers[NN_MAX_OUTPUT_BUFFERS]; /**< Output buffer pointers */
    uint32_t input_size;                /**< Input buffer size */
    uint32_t output_sizes[NN_MAX_OUTPUT_BUFFERS]; /**< Output buffer sizes */
    uint32_t output_count;              /**< Number of output buffers */
} nn_buffers_t;

/**
 * @brief Face detection neural network context
 */
typedef struct {
    nn_buffers_t buffers;               /**< Network buffers */
    pd_model_pp_static_param_t pp_params; /**< Post-processing parameters */
    pd_postprocess_out_t pp_output;     /**< Post-processing output */
    uint32_t inference_time_ms;         /**< Last inference time */
    uint32_t total_inferences;          /**< Total inference count */
    bool is_initialized;                /**< Initialization status */
} face_detection_nn_t;

/**
 * @brief Face recognition neural network context
 */
typedef struct {
    nn_buffers_t buffers;               /**< Network buffers */
    float current_embedding[EMBEDDING_SIZE]; /**< Current face embedding */
    uint32_t inference_time_ms;         /**< Last inference time */
    uint32_t total_inferences;          /**< Total inference count */
    bool embedding_valid;               /**< Embedding validity flag */
    bool is_initialized;                /**< Initialization status */
} face_recognition_nn_t;

/* ========================================================================= */
/* FUNCTION PROTOTYPES                                                       */
/* ========================================================================= */

/**
 * @brief Initialize face detection neural network
 * @param nn_ctx Pointer to face detection context
 * @param config Pointer to application configuration
 * @param memory_pool Pointer to memory pool
 * @return 0 on success, negative on error
 */
int nn_face_detection_init(face_detection_nn_t *nn_ctx, 
                          const app_config_t *config, 
                          memory_pool_t *memory_pool);

/**
 * @brief Initialize face recognition neural network
 * @param nn_ctx Pointer to face recognition context
 * @param config Pointer to application configuration
 * @param memory_pool Pointer to memory pool
 * @return 0 on success, negative on error
 */
int nn_face_recognition_init(face_recognition_nn_t *nn_ctx, 
                            const app_config_t *config, 
                            memory_pool_t *memory_pool);

/**
 * @brief Process frame with face detection network
 * @param nn_ctx Pointer to face detection context
 * @param input_frame Pointer to input frame data
 * @param frame_width Frame width in pixels
 * @param frame_height Frame height in pixels
 * @param config Pointer to application configuration
 * @return 0 on success, negative on error
 */
int nn_face_detection_process(face_detection_nn_t *nn_ctx,
                             const uint8_t *input_frame,
                             uint32_t frame_width,
                             uint32_t frame_height,
                             const app_config_t *config);

/**
 * @brief Process face region with recognition network
 * @param nn_ctx Pointer to face recognition context
 * @param face_region Pointer to face region data
 * @param region_width Region width in pixels
 * @param region_height Region height in pixels
 * @param config Pointer to application configuration
 * @return 0 on success, negative on error
 */
int nn_face_recognition_process(face_recognition_nn_t *nn_ctx,
                               const uint8_t *face_region,
                               uint32_t region_width,
                               uint32_t region_height,
                               const app_config_t *config);

/**
 * @brief Get face detection results
 * @param nn_ctx Pointer to face detection context
 * @param boxes Pointer to store detected bounding boxes
 * @param max_boxes Maximum number of boxes to return
 * @param box_count Pointer to store actual number of boxes
 * @return 0 on success, negative on error
 */
int nn_face_detection_get_results(const face_detection_nn_t *nn_ctx,
                                 pd_pp_box_t *boxes,
                                 uint32_t max_boxes,
                                 uint32_t *box_count);

/**
 * @brief Get face recognition embedding
 * @param nn_ctx Pointer to face recognition context
 * @param embedding Pointer to store embedding data
 * @param embedding_size Size of embedding array
 * @return 0 on success, negative on error
 */
int nn_face_recognition_get_embedding(const face_recognition_nn_t *nn_ctx,
                                     float *embedding,
                                     uint32_t embedding_size);

/**
 * @brief Calculate similarity between two embeddings
 * @param embedding1 First embedding
 * @param embedding2 Second embedding
 * @param embedding_size Size of embeddings
 * @return Cosine similarity score (0.0 to 1.0)
 */
float nn_calculate_embedding_similarity(const float *embedding1,
                                       const float *embedding2,
                                       uint32_t embedding_size);

/**
 * @brief Prepare neural network input buffer
 * @param nn_ctx Pointer to neural network context
 * @param input_data Pointer to input data
 * @param data_size Size of input data
 * @return 0 on success, negative on error
 */
int nn_prepare_input_buffer(const nn_buffers_t *nn_ctx,
                           const uint8_t *input_data,
                           uint32_t data_size);

/**
 * @brief Invalidate neural network output buffers
 * @param nn_ctx Pointer to neural network context
 * @param memory_pool Pointer to memory pool
 * @return 0 on success, negative on error
 */
int nn_invalidate_output_buffers(const nn_buffers_t *nn_ctx,
                                memory_pool_t *memory_pool);

/**
 * @brief Clean and invalidate neural network input buffer
 * @param nn_ctx Pointer to neural network context
 * @param memory_pool Pointer to memory pool
 * @return 0 on success, negative on error
 */
int nn_clean_invalidate_input_buffer(const nn_buffers_t *nn_ctx,
                                    memory_pool_t *memory_pool);

/**
 * @brief Get neural network performance metrics
 * @param nn_ctx Pointer to neural network context
 * @param avg_inference_time Pointer to store average inference time
 * @param total_inferences Pointer to store total inference count
 * @return 0 on success, negative on error
 */
int nn_get_performance_metrics(const face_detection_nn_t *nn_ctx,
                              float *avg_inference_time,
                              uint32_t *total_inferences);

/**
 * @brief Deinitialize face detection neural network
 * @param nn_ctx Pointer to face detection context
 * @param memory_pool Pointer to memory pool
 * @return 0 on success, negative on error
 */
int nn_face_detection_deinit(face_detection_nn_t *nn_ctx,
                            memory_pool_t *memory_pool);

/**
 * @brief Deinitialize face recognition neural network
 * @param nn_ctx Pointer to face recognition context
 * @param memory_pool Pointer to memory pool
 * @return 0 on success, negative on error
 */
int nn_face_recognition_deinit(face_recognition_nn_t *nn_ctx,
                              memory_pool_t *memory_pool);

#endif /* APP_NEURAL_NETWORK_H */