/**
 ******************************************************************************
 * @file    app_config_manager.c
 * @author  PeleAB
 * @brief   Configuration management system implementation
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

#include "app_config_manager.h"
#include <string.h>
#include <stdio.h>

/* ========================================================================= */
/* PRIVATE CONSTANTS                                                         */
/* ========================================================================= */
#define CONFIG_VERSION          0x00010000  /**< Configuration version */
#define CONFIG_MAGIC_NUMBER     0xDEADBEEF  /**< Configuration magic number */

/* ========================================================================= */
/* PRIVATE FUNCTION PROTOTYPES                                               */
/* ========================================================================= */
static void config_set_defaults(app_config_t *config);
static uint32_t crc32_calculate(const uint8_t *data, size_t length);

/* ========================================================================= */
/* PUBLIC FUNCTION IMPLEMENTATIONS                                           */
/* ========================================================================= */

/**
 * @brief Initialize configuration manager with default values
 * @param config Pointer to configuration structure
 * @return 0 on success, negative on error
 */
int config_manager_init(app_config_t *config)
{
    if (config == NULL) {
        return -1;
    }
    
    config_set_defaults(config);
    config->config_version = CONFIG_VERSION;
    config->config_crc = config_manager_calculate_crc(config);
    
    return 0;
}

/**
 * @brief Load configuration from persistent storage
 * @param config Pointer to configuration structure
 * @return 0 on success, negative on error
 */
int config_manager_load(app_config_t *config)
{
    if (config == NULL) {
        return -1;
    }
    
    /* TODO: Implement loading from flash/EEPROM */
    /* For now, just initialize with defaults */
    return config_manager_init(config);
}

/**
 * @brief Save configuration to persistent storage
 * @param config Pointer to configuration structure
 * @return 0 on success, negative on error
 */
int config_manager_save(const app_config_t *config)
{
    if (config == NULL) {
        return -1;
    }
    
    if (!config_manager_validate(config)) {
        return -2;
    }
    
    /* TODO: Implement saving to flash/EEPROM */
    /* For now, just return success */
    return 0;
}

/**
 * @brief Validate configuration parameters
 * @param config Pointer to configuration structure
 * @return true if valid, false otherwise
 */
bool config_manager_validate(const app_config_t *config)
{
    if (config == NULL) {
        return false;
    }
    
    /* Validate face detection parameters */
    if (config->face_detection.confidence_threshold < 0.0f || 
        config->face_detection.confidence_threshold > 1.0f) {
        return false;
    }
    
    if (config->face_detection.nms_threshold < 0.0f || 
        config->face_detection.nms_threshold > 1.0f) {
        return false;
    }
    
    if (config->face_detection.max_detections == 0 || 
        config->face_detection.max_detections > 100) {
        return false;
    }
    
    /* Validate face recognition parameters */
    if (config->face_recognition.similarity_threshold < 0.0f || 
        config->face_recognition.similarity_threshold > 1.0f) {
        return false;
    }
    
    if (config->face_recognition.embedding_scale <= 0.0f) {
        return false;
    }
    
    if (config->face_recognition.bbox_padding_factor < 1.0f || 
        config->face_recognition.bbox_padding_factor > 2.0f) {
        return false;
    }
    
    /* Validate tracking parameters */
    if (config->tracking.smooth_factor < 0.0f || 
        config->tracking.smooth_factor > 1.0f) {
        return false;
    }
    
    if (config->tracking.iou_threshold < 0.0f || 
        config->tracking.iou_threshold > 1.0f) {
        return false;
    }
    
    if (config->tracking.max_lost_frames == 0 || 
        config->tracking.max_lost_frames > 100) {
        return false;
    }
    
    /* Validate performance parameters */
    if (config->performance.target_fps == 0 || 
        config->performance.target_fps > 120) {
        return false;
    }
    
    if (config->performance.reverify_interval_ms == 0 || 
        config->performance.reverify_interval_ms > 10000) {
        return false;
    }
    
    /* Validate protocol parameters */
    if (config->protocol.max_payload_size == 0 || 
        config->protocol.max_payload_size > (1024 * 1024)) {
        return false;
    }
    
    if (config->protocol.uart_timeout_ms == 0 || 
        config->protocol.uart_timeout_ms > 10000) {
        return false;
    }
    
    /* Validate UI parameters */
    if (config->ui.button_long_press_ms == 0 || 
        config->ui.button_long_press_ms > 5000) {
        return false;
    }
    
    return true;
}

/**
 * @brief Reset configuration to default values
 * @param config Pointer to configuration structure
 * @return 0 on success, negative on error
 */
int config_manager_reset(app_config_t *config)
{
    if (config == NULL) {
        return -1;
    }
    
    return config_manager_init(config);
}

/**
 * @brief Calculate configuration checksum
 * @param config Pointer to configuration structure
 * @return CRC32 checksum value
 */
uint32_t config_manager_calculate_crc(const app_config_t *config)
{
    if (config == NULL) {
        return 0;
    }
    
    /* Calculate CRC32 of configuration excluding the CRC field itself */
    size_t crc_offset = offsetof(app_config_t, config_crc);
    return crc32_calculate((const uint8_t*)config, crc_offset);
}

/**
 * @brief Print configuration to debug output
 * @param config Pointer to configuration structure
 */
void config_manager_print(const app_config_t *config)
{
    if (config == NULL) {
        return;
    }
    
    printf("=== Application Configuration ===\n");
    printf("Version: 0x%08lX\n", (unsigned long)config->config_version);
    printf("CRC: 0x%08lX\n", (unsigned long)config->config_crc);
    
    printf("\n--- Face Detection ---\n");
    printf("Confidence Threshold: %.3f\n", config->face_detection.confidence_threshold);
    printf("NMS Threshold: %.3f\n", config->face_detection.nms_threshold);
    printf("Max Detections: %lu\n", (unsigned long)config->face_detection.max_detections);
    printf("Enable Preprocessing: %s\n", config->face_detection.enable_preprocessing ? "Yes" : "No");
    
    printf("\n--- Face Recognition ---\n");
    printf("Similarity Threshold: %.3f\n", config->face_recognition.similarity_threshold);
    printf("Embedding Scale: %.3f\n", config->face_recognition.embedding_scale);
    printf("Max Embeddings: %lu\n", (unsigned long)config->face_recognition.max_embeddings);
    printf("Enable Alignment: %s\n", config->face_recognition.enable_alignment ? "Yes" : "No");
    printf("BBox Padding Factor: %.3f\n", config->face_recognition.bbox_padding_factor);
    
    printf("\n--- Tracking ---\n");
    printf("Smooth Factor: %.3f\n", config->tracking.smooth_factor);
    printf("IoU Threshold: %.3f\n", config->tracking.iou_threshold);
    printf("Max Lost Frames: %lu\n", (unsigned long)config->tracking.max_lost_frames);
    printf("Min Init Confidence: %.3f\n", config->tracking.min_init_confidence);
    printf("Association Threshold: %.3f\n", config->tracking.association_threshold);
    printf("Enable Prediction: %s\n", config->tracking.enable_prediction ? "Yes" : "No");
    
    printf("\n--- Performance ---\n");
    printf("Target FPS: %lu\n", (unsigned long)config->performance.target_fps);
    printf("Reverify Interval: %lu ms\n", (unsigned long)config->performance.reverify_interval_ms);
    printf("Update Interval: %lu\n", (unsigned long)config->performance.update_interval);
    printf("Enable Profiling: %s\n", config->performance.enable_profiling ? "Yes" : "No");
    
    printf("\n--- Protocol ---\n");
    printf("Max Payload Size: %lu bytes\n", (unsigned long)config->protocol.max_payload_size);
    printf("UART Timeout: %lu ms\n", (unsigned long)config->protocol.uart_timeout_ms);
    printf("Stream Scale Factor: %lu\n", (unsigned long)config->protocol.stream_scale_factor);
    printf("Enable CRC Validation: %s\n", config->protocol.enable_crc_validation ? "Yes" : "No");
    
    printf("\n--- User Interface ---\n");
    printf("Button Long Press: %lu ms\n", (unsigned long)config->ui.button_long_press_ms);
    printf("LED Timeout: %lu ms\n", (unsigned long)config->ui.led_timeout_ms);
    printf("Enable Button Feedback: %s\n", config->ui.enable_button_feedback ? "Yes" : "No");
    
    printf("================================\n");
}

/* ========================================================================= */
/* PRIVATE FUNCTION IMPLEMENTATIONS                                          */
/* ========================================================================= */

/**
 * @brief Set default configuration values
 * @param config Pointer to configuration structure
 */
static void config_set_defaults(app_config_t *config)
{
    /* Face detection defaults */
    config->face_detection.confidence_threshold = FACE_DETECTION_CONFIDENCE_THRESHOLD;
    config->face_detection.nms_threshold = 0.5f;
    config->face_detection.max_detections = 10;
    config->face_detection.enable_preprocessing = true;
    
    /* Face recognition defaults */
    config->face_recognition.similarity_threshold = FACE_SIMILARITY_THRESHOLD;
    config->face_recognition.embedding_scale = FACE_EMBEDDING_QUANTIZATION_SCALE;
    config->face_recognition.max_embeddings = 100;
    config->face_recognition.enable_alignment = true;
    config->face_recognition.bbox_padding_factor = FACE_BBOX_PADDING_FACTOR;
    
    /* Tracking defaults */
    config->tracking.smooth_factor = TRACKER_SMOOTH_FACTOR;
    config->tracking.iou_threshold = TRACKER_IOU_THRESHOLD;
    config->tracking.max_lost_frames = TRACKER_MAX_LOST_FRAMES;
    config->tracking.min_init_confidence = TRACKER_MIN_INIT_CONFIDENCE;
    config->tracking.association_threshold = TRACKER_ASSOCIATION_THRESHOLD;
    config->tracking.enable_prediction = true;
    
    /* Performance defaults */
    config->performance.target_fps = TARGET_CAMERA_FPS;
    config->performance.reverify_interval_ms = FACE_REVERIFY_INTERVAL_MS;
    config->performance.update_interval = PERFORMANCE_UPDATE_INTERVAL;
    config->performance.enable_profiling = false;
    
    /* Protocol defaults */
    config->protocol.max_payload_size = PROTOCOL_MAX_PAYLOAD_SIZE;
    config->protocol.uart_timeout_ms = UART_COMMUNICATION_TIMEOUT_MS;
    config->protocol.stream_scale_factor = DISPLAY_STREAM_SCALE_FACTOR;
    config->protocol.enable_crc_validation = true;
    
    /* UI defaults */
    config->ui.button_long_press_ms = BUTTON_LONG_PRESS_DURATION_MS;
    config->ui.led_timeout_ms = FACE_UNVERIFIED_LED_TIMEOUT_MS;
    config->ui.enable_button_feedback = true;
}

/**
 * @brief Calculate CRC32 checksum
 * @param data Pointer to data buffer
 * @param length Length of data buffer
 * @return CRC32 checksum value
 */
static uint32_t crc32_calculate(const uint8_t *data, size_t length)
{
    uint32_t crc = 0xFFFFFFFF;
    
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ PROTOCOL_CRC32_POLYNOMIAL;
            } else {
                crc >>= 1;
            }
        }
    }
    
    return crc ^ 0xFFFFFFFF;
}