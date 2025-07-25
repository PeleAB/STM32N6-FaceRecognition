/**
 ******************************************************************************
 * @file    app_config_manager.h
 * @author  PeleAB
 * @brief   Configuration management system for runtime parameters
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

#ifndef APP_CONFIG_MANAGER_H
#define APP_CONFIG_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "app_constants.h"

/* ========================================================================= */
/* CONFIGURATION STRUCTURE                                                   */
/* ========================================================================= */

/**
 * @brief Face detection configuration parameters
 */
typedef struct {
    float confidence_threshold;     /**< Detection confidence threshold */
    float nms_threshold;           /**< Non-maximum suppression threshold */
    uint32_t max_detections;       /**< Maximum number of detections per frame */
    bool enable_preprocessing;     /**< Enable input preprocessing */
} face_detection_config_t;

/**
 * @brief Face recognition configuration parameters
 */
typedef struct {
    float similarity_threshold;    /**< Face similarity threshold */
    float embedding_scale;         /**< Embedding quantization scale */
    uint32_t max_embeddings;       /**< Maximum stored embeddings */
    bool enable_alignment;         /**< Enable face alignment */
    float bbox_padding_factor;     /**< Bounding box padding factor */
} face_recognition_config_t;

/**
 * @brief Tracking configuration parameters
 */
typedef struct {
    float smooth_factor;           /**< Kalman filter smoothing factor */
    float iou_threshold;           /**< IoU threshold for association */
    uint32_t max_lost_frames;      /**< Maximum lost frames before track deletion */
    float min_init_confidence;     /**< Minimum confidence for track initialization */
    float association_threshold;   /**< Track association distance threshold */
    bool enable_prediction;        /**< Enable motion prediction */
} tracking_config_t;

/**
 * @brief Performance configuration parameters
 */
typedef struct {
    uint32_t target_fps;           /**< Target frames per second */
    uint32_t reverify_interval_ms; /**< Face re-verification interval */
    uint32_t update_interval;      /**< Performance update interval */
    bool enable_profiling;         /**< Enable performance profiling */
} performance_config_t;

/**
 * @brief Protocol configuration parameters
 */
typedef struct {
    uint32_t max_payload_size;     /**< Maximum payload size */
    uint32_t uart_timeout_ms;      /**< UART communication timeout */
    uint32_t stream_scale_factor;  /**< Display stream scale factor */
    bool enable_crc_validation;    /**< Enable CRC32 validation */
} protocol_config_t;

/**
 * @brief User interface configuration parameters
 */
typedef struct {
    uint32_t button_long_press_ms; /**< Long press duration */
    uint32_t led_timeout_ms;       /**< LED indication timeout */
    bool enable_button_feedback;   /**< Enable button feedback */
} ui_config_t;

/**
 * @brief Main application configuration structure
 */
typedef struct {
    face_detection_config_t face_detection;  /**< Face detection parameters */
    face_recognition_config_t face_recognition; /**< Face recognition parameters */
    tracking_config_t tracking;              /**< Tracking parameters */
    performance_config_t performance;        /**< Performance parameters */
    protocol_config_t protocol;              /**< Protocol parameters */
    ui_config_t ui;                          /**< User interface parameters */
    uint32_t config_version;                 /**< Configuration version */
    uint32_t config_crc;                     /**< Configuration checksum */
} app_config_t;

/* ========================================================================= */
/* FUNCTION PROTOTYPES                                                       */
/* ========================================================================= */

/**
 * @brief Initialize configuration manager with default values
 * @param config Pointer to configuration structure
 * @return 0 on success, negative on error
 */
int config_manager_init(app_config_t *config);

/**
 * @brief Load configuration from persistent storage
 * @param config Pointer to configuration structure
 * @return 0 on success, negative on error
 */
int config_manager_load(app_config_t *config);

/**
 * @brief Save configuration to persistent storage
 * @param config Pointer to configuration structure
 * @return 0 on success, negative on error
 */
int config_manager_save(const app_config_t *config);

/**
 * @brief Validate configuration parameters
 * @param config Pointer to configuration structure
 * @return true if valid, false otherwise
 */
bool config_manager_validate(const app_config_t *config);

/**
 * @brief Reset configuration to default values
 * @param config Pointer to configuration structure
 * @return 0 on success, negative on error
 */
int config_manager_reset(app_config_t *config);

/**
 * @brief Get configuration parameter by name
 * @param config Pointer to configuration structure
 * @param param_name Parameter name string
 * @param value Pointer to store parameter value
 * @return 0 on success, negative on error
 */
int config_manager_get_param(const app_config_t *config, const char *param_name, void *value);

/**
 * @brief Set configuration parameter by name
 * @param config Pointer to configuration structure
 * @param param_name Parameter name string
 * @param value Pointer to parameter value
 * @return 0 on success, negative on error
 */
int config_manager_set_param(app_config_t *config, const char *param_name, const void *value);

/**
 * @brief Calculate configuration checksum
 * @param config Pointer to configuration structure
 * @return CRC32 checksum value
 */
uint32_t config_manager_calculate_crc(const app_config_t *config);

/**
 * @brief Print configuration to debug output
 * @param config Pointer to configuration structure
 */
void config_manager_print(const app_config_t *config);

#endif /* APP_CONFIG_MANAGER_H */