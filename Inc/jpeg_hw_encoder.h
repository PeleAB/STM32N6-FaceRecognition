/**
 ******************************************************************************
 * @file    jpeg_hw_encoder.h
 * @brief   STM32N6 Hardware JPEG encoder for enhanced PC streaming
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

#ifndef __JPEG_HW_ENCODER_H
#define __JPEG_HW_ENCODER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32n6xx_hal.h"

#ifdef HAL_JPEG_MODULE_ENABLED

#include "stm32n6xx_hal_jpeg.h"
#include <stdint.h>
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/

/**
 * @brief JPEG encoding result structure
 */
typedef struct {
    uint32_t encoded_size;      /* Size of encoded JPEG data */
    bool     success;           /* Encoding success flag */
    uint32_t encoding_time_ms;  /* Encoding time in milliseconds */
} JPEG_EncodeResult_t;

/**
 * @brief JPEG encoder configuration
 */
typedef struct {
    uint32_t width;             /* Image width */
    uint32_t height;            /* Image height */
    uint32_t quality;           /* JPEG quality (1-100) */
    uint32_t color_format;      /* RGB color format */
    uint8_t  channels;          /* Number of color channels (1=grayscale, 3=color) */
} JPEG_EncodeConfig_t;

/* Exported constants --------------------------------------------------------*/

/* JPEG Quality levels */
#define JPEG_HW_QUALITY_LOW     50
#define JPEG_HW_QUALITY_MEDIUM  75
#define JPEG_HW_QUALITY_HIGH    85
#define JPEG_HW_QUALITY_MAX     95

/* RGB Color formats */
#define JPEG_HW_RGB565          0
#define JPEG_HW_RGB888          1
#define JPEG_HW_ARGB8888        2
#define JPEG_HW_GRAYSCALE       3

/* Buffer sizes */
#define JPEG_HW_MAX_WIDTH       800
#define JPEG_HW_MAX_HEIGHT      600
#define JPEG_HW_MAX_INPUT_SIZE  (JPEG_HW_MAX_WIDTH * JPEG_HW_MAX_HEIGHT * 4)
#define JPEG_HW_MAX_OUTPUT_SIZE (64 * 1024)  /* 64KB output buffer */

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Initialize JPEG hardware encoder
 * @retval true if initialization successful, false otherwise
 */
bool JPEG_HW_Init(void);

/**
 * @brief Deinitialize JPEG hardware encoder
 */
void JPEG_HW_DeInit(void);

/**
 * @brief Encode image data using STM32N6 JPEG hardware
 * @param input_data: Pointer to input image data
 * @param config: Encoder configuration
 * @param output_buffer: Pointer to output buffer for JPEG data
 * @param output_buffer_size: Size of output buffer
 * @param result: Pointer to result structure
 * @retval true if encoding successful, false otherwise
 */
bool JPEG_HW_Encode(const uint8_t *input_data, 
                    const JPEG_EncodeConfig_t *config,
                    uint8_t *output_buffer,
                    uint32_t output_buffer_size,
                    JPEG_EncodeResult_t *result);

/**
 * @brief Encode RGB565 image to JPEG (optimized for camera frames)
 * @param rgb565_data: Pointer to RGB565 input data
 * @param width: Image width
 * @param height: Image height
 * @param quality: JPEG quality (1-100)
 * @param output_buffer: Pointer to output buffer
 * @param output_buffer_size: Size of output buffer
 * @param result: Pointer to result structure
 * @retval true if encoding successful, false otherwise
 */
bool JPEG_HW_EncodeRGB565(const uint8_t *rgb565_data,
                          uint32_t width, uint32_t height, uint32_t quality,
                          uint8_t *output_buffer, uint32_t output_buffer_size,
                          JPEG_EncodeResult_t *result);

/**
 * @brief Encode RGB888 image to JPEG (optimized for alignment frames)
 * @param rgb888_data: Pointer to RGB888 input data
 * @param width: Image width
 * @param height: Image height
 * @param quality: JPEG quality (1-100)
 * @param output_buffer: Pointer to output buffer
 * @param output_buffer_size: Size of output buffer
 * @param result: Pointer to result structure
 * @retval true if encoding successful, false otherwise
 */
bool JPEG_HW_EncodeRGB888(const uint8_t *rgb888_data,
                          uint32_t width, uint32_t height, uint32_t quality,
                          uint8_t *output_buffer, uint32_t output_buffer_size,
                          JPEG_EncodeResult_t *result);

/**
 * @brief Encode grayscale image to JPEG (optimized for detection frames)
 * @param gray_data: Pointer to grayscale input data
 * @param width: Image width
 * @param height: Image height
 * @param quality: JPEG quality (1-100)
 * @param output_buffer: Pointer to output buffer
 * @param output_buffer_size: Size of output buffer
 * @param result: Pointer to result structure
 * @retval true if encoding successful, false otherwise
 */
bool JPEG_HW_EncodeGrayscale(const uint8_t *gray_data,
                             uint32_t width, uint32_t height, uint32_t quality,
                             uint8_t *output_buffer, uint32_t output_buffer_size,
                             JPEG_EncodeResult_t *result);

/**
 * @brief Get JPEG hardware encoder status
 * @retval true if encoder is ready, false if busy
 */
bool JPEG_HW_IsReady(void);

/**
 * @brief Get last encoding performance metrics
 * @param encoding_time_ms: Pointer to store encoding time
 * @param throughput_mbps: Pointer to store throughput in Mbps
 */
void JPEG_HW_GetPerformanceMetrics(uint32_t *encoding_time_ms, float *throughput_mbps);

#endif /* HAL_JPEG_MODULE_ENABLED */

#ifdef __cplusplus
}
#endif

#endif /* __JPEG_HW_ENCODER_H */