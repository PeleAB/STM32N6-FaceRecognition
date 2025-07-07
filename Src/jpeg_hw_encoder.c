/**
 ******************************************************************************
 * @file    jpeg_hw_encoder.c
 * @brief   STM32N6 Hardware JPEG encoder implementation
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

/* Includes ------------------------------------------------------------------*/
#include "jpeg_hw_encoder.h"

#ifdef HAL_JPEG_MODULE_ENABLED

#include <string.h>
#include <stdio.h>

/* Private typedef -----------------------------------------------------------*/
typedef struct {
    uint8_t state;
    uint8_t *data_buffer;
    uint32_t data_buffer_size;
} JPEG_DataBuffer_t;

/* Private defines -----------------------------------------------------------*/
#define JPEG_BUFFER_EMPTY       0
#define JPEG_BUFFER_FULL        1

#define CHUNK_SIZE_IN           (800 * 4 * 16)  /* Max width * 4 bytes * 16 lines */
#define CHUNK_SIZE_OUT          (8 * 1024)     /* 8KB output chunks */

#define JPEG_TIMEOUT_MS         5000           /* 5 second timeout */

/* Private variables ---------------------------------------------------------*/
static JPEG_HandleTypeDef hjpeg;
static bool jpeg_hw_initialized = false;
static bool jpeg_hw_busy = false;

/* DMA handles (if needed) */
static DMA_HandleTypeDef hdma_jpeg_in;
static DMA_HandleTypeDef hdma_jpeg_out;

/* Working buffers */
__attribute__((aligned(32)))
static uint8_t jpeg_input_buffer[CHUNK_SIZE_IN];

__attribute__((aligned(32)))
static uint8_t jpeg_output_buffer0[CHUNK_SIZE_OUT];

__attribute__((aligned(32)))
static uint8_t jpeg_output_buffer1[CHUNK_SIZE_OUT];

/* Buffer management */
static JPEG_DataBuffer_t jpeg_in_buffer = {JPEG_BUFFER_EMPTY, jpeg_input_buffer, 0};
static JPEG_DataBuffer_t jpeg_out_buffer = {JPEG_BUFFER_EMPTY, jpeg_output_buffer0, 0};

/* Encoding state */
static volatile uint32_t jpeg_encoding_complete = 0;
static volatile uint32_t jpeg_output_paused = 0;
static volatile uint32_t jpeg_input_paused = 0;

/* Performance tracking */
static uint32_t last_encoding_time_ms = 0;
static float last_throughput_mbps = 0.0f;

/* Color conversion function pointer */
static JPEG_RGBToYCbCr_Convert_Function pRGBToYCbCr_Convert_Function = NULL;

/* Private function prototypes -----------------------------------------------*/
static bool JPEG_HW_InitPeripheral(void);
static void JPEG_HW_DeInitPeripheral(void);
static bool JPEG_HW_ConfigureEncoding(const JPEG_EncodeConfig_t *config, JPEG_ConfTypeDef *jpeg_conf);
static uint32_t JPEG_HW_ProcessInputData(const uint8_t *input_data, const JPEG_EncodeConfig_t *config);
static uint32_t JPEG_HW_ProcessOutputData(uint8_t *output_buffer, uint32_t output_buffer_size);
static void JPEG_HW_RGB565ToRGB888(const uint8_t *rgb565_data, uint8_t *rgb888_data, uint32_t pixel_count);
static void JPEG_HW_GrayscaleToRGB888(const uint8_t *gray_data, uint8_t *rgb888_data, uint32_t pixel_count);

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Initialize JPEG hardware encoder
 */
bool JPEG_HW_Init(void)
{
    if (jpeg_hw_initialized) {
        return true;
    }

    /* Initialize JPEG peripheral */
    if (!JPEG_HW_InitPeripheral()) {
        return false;
    }

    /* Initialize color conversion tables */
    JPEG_InitColorTables();

    jpeg_hw_initialized = true;
    jpeg_hw_busy = false;

    return true;
}

/**
 * @brief Deinitialize JPEG hardware encoder
 */
void JPEG_HW_DeInit(void)
{
    if (!jpeg_hw_initialized) {
        return;
    }

    JPEG_HW_DeInitPeripheral();
    jpeg_hw_initialized = false;
    jpeg_hw_busy = false;
}

/**
 * @brief Main JPEG encoding function
 */
bool JPEG_HW_Encode(const uint8_t *input_data, 
                    const JPEG_EncodeConfig_t *config,
                    uint8_t *output_buffer,
                    uint32_t output_buffer_size,
                    JPEG_EncodeResult_t *result)
{
    if (!jpeg_hw_initialized || jpeg_hw_busy || !input_data || !config || !output_buffer || !result) {
        return false;
    }

    uint32_t start_time = HAL_GetTick();
    jpeg_hw_busy = true;
    
    /* Initialize result */
    result->encoded_size = 0;
    result->success = false;
    result->encoding_time_ms = 0;

    /* Configure JPEG encoding */
    JPEG_ConfTypeDef jpeg_conf;
    if (!JPEG_HW_ConfigureEncoding(config, &jpeg_conf)) {
        jpeg_hw_busy = false;
        return false;
    }

    /* Reset encoding state */
    jpeg_encoding_complete = 0;
    jpeg_output_paused = 0;
    jpeg_input_paused = 0;
    
    /* Process input data */
    uint32_t input_processed = JPEG_HW_ProcessInputData(input_data, config);
    if (input_processed == 0) {
        jpeg_hw_busy = false;
        return false;
    }

    /* Configure JPEG encoding parameters */
    if (HAL_JPEG_ConfigEncoding(&hjpeg, &jpeg_conf) != HAL_OK) {
        jpeg_hw_busy = false;
        return false;
    }

    /* Start JPEG encoding with DMA */
    if (HAL_JPEG_Encode_DMA(&hjpeg, 
                            jpeg_in_buffer.data_buffer, 
                            jpeg_in_buffer.data_buffer_size,
                            jpeg_out_buffer.data_buffer, 
                            CHUNK_SIZE_OUT) != HAL_OK) {
        jpeg_hw_busy = false;
        return false;
    }

    /* Wait for encoding completion with timeout */
    uint32_t timeout_start = HAL_GetTick();
    uint32_t total_output_size = 0;
    
    while (!jpeg_encoding_complete && ((HAL_GetTick() - timeout_start) < JPEG_TIMEOUT_MS)) {
        /* Process input data if needed */
        JPEG_EncodeInputHandler(&hjpeg);
        
        /* Process output data */
        uint32_t output_chunk_size = JPEG_HW_ProcessOutputData(
            output_buffer + total_output_size, 
            output_buffer_size - total_output_size
        );
        
        if (output_chunk_size > 0) {
            total_output_size += output_chunk_size;
            
            /* Check if output buffer is full */
            if (total_output_size >= output_buffer_size - CHUNK_SIZE_OUT) {
                break;
            }
        }
        
        /* Small delay to prevent busy waiting */
        HAL_Delay(1);
    }

    /* Calculate performance metrics */
    uint32_t encoding_time = HAL_GetTick() - start_time;
    last_encoding_time_ms = encoding_time;
    
    if (encoding_time > 0) {
        uint32_t input_size = config->width * config->height * config->channels;
        last_throughput_mbps = ((float)input_size * 8.0f) / ((float)encoding_time * 1000.0f);
    }

    /* Update result */
    result->encoded_size = total_output_size;
    result->success = (jpeg_encoding_complete != 0) && (total_output_size > 0);
    result->encoding_time_ms = encoding_time;

    jpeg_hw_busy = false;
    return result->success;
}

/**
 * @brief Optimized RGB565 encoding
 */
bool JPEG_HW_EncodeRGB565(const uint8_t *rgb565_data,
                          uint32_t width, uint32_t height, uint32_t quality,
                          uint8_t *output_buffer, uint32_t output_buffer_size,
                          JPEG_EncodeResult_t *result)
{
    JPEG_EncodeConfig_t config = {
        .width = width,
        .height = height,
        .quality = quality,
        .color_format = JPEG_HW_RGB565,
        .channels = 2
    };
    
    return JPEG_HW_Encode(rgb565_data, &config, output_buffer, output_buffer_size, result);
}

/**
 * @brief Optimized RGB888 encoding
 */
bool JPEG_HW_EncodeRGB888(const uint8_t *rgb888_data,
                          uint32_t width, uint32_t height, uint32_t quality,
                          uint8_t *output_buffer, uint32_t output_buffer_size,
                          JPEG_EncodeResult_t *result)
{
    JPEG_EncodeConfig_t config = {
        .width = width,
        .height = height,
        .quality = quality,
        .color_format = JPEG_HW_RGB888,
        .channels = 3
    };
    
    return JPEG_HW_Encode(rgb888_data, &config, output_buffer, output_buffer_size, result);
}

/**
 * @brief Optimized grayscale encoding
 */
bool JPEG_HW_EncodeGrayscale(const uint8_t *gray_data,
                             uint32_t width, uint32_t height, uint32_t quality,
                             uint8_t *output_buffer, uint32_t output_buffer_size,
                             JPEG_EncodeResult_t *result)
{
    JPEG_EncodeConfig_t config = {
        .width = width,
        .height = height,
        .quality = quality,
        .color_format = JPEG_HW_GRAYSCALE,
        .channels = 1
    };
    
    return JPEG_HW_Encode(gray_data, &config, output_buffer, output_buffer_size, result);
}

/**
 * @brief Check if JPEG hardware is ready
 */
bool JPEG_HW_IsReady(void)
{
    return jpeg_hw_initialized && !jpeg_hw_busy;
}

/**
 * @brief Get performance metrics
 */
void JPEG_HW_GetPerformanceMetrics(uint32_t *encoding_time_ms, float *throughput_mbps)
{
    if (encoding_time_ms) {
        *encoding_time_ms = last_encoding_time_ms;
    }
    if (throughput_mbps) {
        *throughput_mbps = last_throughput_mbps;
    }
}

/* Private functions ---------------------------------------------------------*/

/**
 * @brief Initialize JPEG peripheral and DMA
 */
static bool JPEG_HW_InitPeripheral(void)
{
    /* Configure JPEG peripheral */
    hjpeg.Instance = JPEG;
    
    if (HAL_JPEG_Init(&hjpeg) != HAL_OK) {
        return false;
    }

    /* Note: DMA configuration would typically be done in HAL_JPEG_MspInit */
    
    return true;
}

/**
 * @brief Deinitialize JPEG peripheral
 */
static void JPEG_HW_DeInitPeripheral(void)
{
    HAL_JPEG_DeInit(&hjpeg);
}

/**
 * @brief Configure JPEG encoding parameters
 */
static bool JPEG_HW_ConfigureEncoding(const JPEG_EncodeConfig_t *config, JPEG_ConfTypeDef *jpeg_conf)
{
    if (!config || !jpeg_conf) {
        return false;
    }

    /* Set basic parameters */
    jpeg_conf->ImageWidth = config->width;
    jpeg_conf->ImageHeight = config->height;
    jpeg_conf->ImageQuality = config->quality;

    /* Set color format */
    switch (config->color_format) {
        case JPEG_HW_RGB565:
            jpeg_conf->ColorSpace = JPEG_CONFCOLOR_RGB;
            jpeg_conf->ChromaSubsampling = JPEG_422_SUBSAMPLING;
            break;
            
        case JPEG_HW_RGB888:
            jpeg_conf->ColorSpace = JPEG_CONFCOLOR_RGB;
            jpeg_conf->ChromaSubsampling = JPEG_444_SUBSAMPLING;
            break;
            
        case JPEG_HW_GRAYSCALE:
            jpeg_conf->ColorSpace = JPEG_GRAYSCALE_COLORSPACE;
            jpeg_conf->ChromaSubsampling = JPEG_444_SUBSAMPLING;
            break;
            
        default:
            return false;
    }

    return true;
}

/**
 * @brief Process input data for JPEG encoding
 */
static uint32_t JPEG_HW_ProcessInputData(const uint8_t *input_data, const JPEG_EncodeConfig_t *config)
{
    if (!input_data || !config) {
        return 0;
    }

    uint32_t input_size = config->width * config->height * config->channels;
    uint32_t processed_size = 0;

    /* For now, copy data directly to input buffer */
    /* In a full implementation, this would handle chunked processing */
    if (input_size <= CHUNK_SIZE_IN) {
        memcpy(jpeg_input_buffer, input_data, input_size);
        jpeg_in_buffer.data_buffer_size = input_size;
        jpeg_in_buffer.state = JPEG_BUFFER_FULL;
        processed_size = input_size;
    }

    return processed_size;
}

/**
 * @brief Process JPEG output data
 */
static uint32_t JPEG_HW_ProcessOutputData(uint8_t *output_buffer, uint32_t output_buffer_size)
{
    uint32_t copied_size = 0;

    if (jpeg_out_buffer.state == JPEG_BUFFER_FULL) {
        uint32_t copy_size = (jpeg_out_buffer.data_buffer_size < output_buffer_size) ? 
                            jpeg_out_buffer.data_buffer_size : output_buffer_size;
        
        memcpy(output_buffer, jpeg_out_buffer.data_buffer, copy_size);
        copied_size = copy_size;
        
        /* Mark buffer as empty */
        jpeg_out_buffer.state = JPEG_BUFFER_EMPTY;
        jpeg_out_buffer.data_buffer_size = 0;
    }

    return copied_size;
}

/**
 * @brief Convert RGB565 to RGB888
 */
static void JPEG_HW_RGB565ToRGB888(const uint8_t *rgb565_data, uint8_t *rgb888_data, uint32_t pixel_count)
{
    const uint16_t *src = (const uint16_t *)rgb565_data;
    uint8_t *dst = rgb888_data;
    
    for (uint32_t i = 0; i < pixel_count; i++) {
        uint16_t pixel = src[i];
        dst[i * 3 + 0] = ((pixel >> 11) & 0x1F) << 3;  /* Red */
        dst[i * 3 + 1] = ((pixel >> 5) & 0x3F) << 2;   /* Green */
        dst[i * 3 + 2] = (pixel & 0x1F) << 3;          /* Blue */
    }
}

/**
 * @brief Convert grayscale to RGB888
 */
static void JPEG_HW_GrayscaleToRGB888(const uint8_t *gray_data, uint8_t *rgb888_data, uint32_t pixel_count)
{
    for (uint32_t i = 0; i < pixel_count; i++) {
        uint8_t gray = gray_data[i];
        rgb888_data[i * 3 + 0] = gray;  /* Red */
        rgb888_data[i * 3 + 1] = gray;  /* Green */
        rgb888_data[i * 3 + 2] = gray;  /* Blue */
    }
}

/* HAL Callbacks -------------------------------------------------------------*/

/**
 * @brief JPEG encoding complete callback
 */
void HAL_JPEG_EncodeCpltCallback(JPEG_HandleTypeDef *hjpeg)
{
    jpeg_encoding_complete = 1;
}

/**
 * @brief JPEG data ready callback
 */
void HAL_JPEG_DataReadyCallback(JPEG_HandleTypeDef *hjpeg, uint8_t *pDataOut, uint32_t OutDataLength)
{
    jpeg_out_buffer.data_buffer_size = OutDataLength;
    jpeg_out_buffer.state = JPEG_BUFFER_FULL;
}

/**
 * @brief JPEG get data callback
 */
void HAL_JPEG_GetDataCallback(JPEG_HandleTypeDef *hjpeg, uint32_t NbDecodedData)
{
    /* Handle input data requests if needed */
}

/**
 * @brief JPEG error callback
 */
void HAL_JPEG_ErrorCallback(JPEG_HandleTypeDef *hjpeg)
{
    jpeg_encoding_complete = 1;  /* Stop processing on error */
}

#endif /* HAL_JPEG_MODULE_ENABLED */