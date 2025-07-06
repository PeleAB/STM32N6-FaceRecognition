/**
 ******************************************************************************
 * @file    robust_pc_stream.c
 * @brief   Robust binary protocol for PC streaming with checksums and framing
 ******************************************************************************
 */

#include "robust_pc_stream.h"
#include "stm32n6570_discovery.h"
#include "stm32n6570_discovery_conf.h"
#include "stm32n6xx_hal_uart.h"
#include "app_config.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "stb_image_write.h"

/* ========================================================================= */
/* CONFIGURATION CONSTANTS                                                   */
/* ========================================================================= */

#define JPEG_QUALITY                85
#define STREAM_SCALE                2
#define ROBUST_UART_TIMEOUT         100
#define ROBUST_MAX_RETRIES          3

/* ========================================================================= */
/* GLOBAL VARIABLES                                                          */
/* ========================================================================= */

#if (USE_BSP_COM_FEATURE > 0)
extern UART_HandleTypeDef hcom_uart[COMn];

static MX_UART_InitTypeDef RobustPcUartInit = {
    .BaudRate = 921600 * 8,
    .WordLength = UART_WORDLENGTH_8B,
    .StopBits = UART_STOPBITS_1,
    .Parity = UART_PARITY_NONE,
    .HwFlowCtl = UART_HWCONTROL_NONE
};

/* Protocol context */
static robust_protocol_stats_t g_robust_stats = {0};
static bool g_robust_initialized = false;

/* Buffers */
__attribute__ ((section (".psram_bss")))
__attribute__((aligned (32)))
static uint8_t robust_jpeg_buffer[64 * 1024];

__attribute__ ((section (".psram_bss")))
__attribute__((aligned (32)))
static uint8_t robust_temp_buffer[8 * 1024];

__attribute__ ((section (".psram_bss")))
__attribute__((aligned (32)))
static uint8_t robust_stream_buffer[320 * 240];  // Downscaled frame buffer

/* ========================================================================= */
/* UTILITY FUNCTIONS                                                         */
/* ========================================================================= */

/**
 * @brief Calculate XOR checksum
 */
uint8_t robust_calculate_checksum(const uint8_t *data, uint32_t length)
{
    uint8_t checksum = 0;
    for (uint32_t i = 0; i < length; i++) {
        checksum ^= data[i];
    }
    return checksum;
}

/**
 * @brief Get next sequence ID for message type
 */
static uint16_t get_next_sequence_id(robust_message_type_t msg_type)
{
    if (msg_type < 16) {
        return ++g_robust_stats.sequence_counters[msg_type];
    }
    return 0;
}

/**
 * @brief Memory writer for JPEG compression
 */
typedef struct {
    uint8_t *buffer;
    size_t size;
    size_t capacity;
} robust_mem_writer_t;

static void robust_mem_write_func(void *context, void *data, int size)
{
    robust_mem_writer_t *writer = (robust_mem_writer_t *)context;
    if (writer->size + size <= writer->capacity) {
        memcpy(writer->buffer + writer->size, data, size);
        writer->size += size;
    }
}

/**
 * @brief Convert RGB565 to grayscale
 */
static uint8_t robust_rgb565_to_gray(uint16_t pixel)
{
    uint8_t r = ((pixel >> 11) & 0x1F) << 3;
    uint8_t g = ((pixel >> 5) & 0x3F) << 2;
    uint8_t b = (pixel & 0x1F) << 3;
    return (uint8_t)((r * 30 + g * 59 + b * 11) / 100);
}

/**
 * @brief Convert RGB888 to grayscale
 */
static uint8_t robust_rgb888_to_gray(uint8_t r, uint8_t g, uint8_t b)
{
    return (uint8_t)((r * 30 + g * 59 + b * 11) / 100);
}

/* ========================================================================= */
/* CORE PROTOCOL FUNCTIONS                                                   */
/* ========================================================================= */

/**
 * @brief Send raw message with robust framing
 */
bool robust_send_message(robust_message_type_t message_type, 
                        const uint8_t *payload, uint32_t payload_size)
{
    if (!g_robust_initialized) {
        return false;
    }
    
    if (payload_size > ROBUST_MAX_PAYLOAD_SIZE - ROBUST_MSG_HEADER_SIZE) {
        g_robust_stats.send_errors++;
        return false;
    }
    
    // Prepare message header
    robust_message_header_t msg_header = {
        .message_type = (uint8_t)message_type,
        .sequence_id = get_next_sequence_id(message_type)
    };
    
    // Calculate total payload size (message header + payload)
    uint32_t total_payload_size = ROBUST_MSG_HEADER_SIZE + payload_size;
    
    // Prepare frame header
    robust_frame_header_t frame_header = {
        .sof = ROBUST_SOF_BYTE,
        .payload_size = total_payload_size,
        .header_checksum = 0
    };
    
    // Calculate header checksum (SOF + payload_size)
    uint8_t header_data[3];
    header_data[0] = frame_header.sof;
    header_data[1] = (uint8_t)(frame_header.payload_size & 0xFF);
    header_data[2] = (uint8_t)((frame_header.payload_size >> 8) & 0xFF);
    frame_header.header_checksum = robust_calculate_checksum(header_data, 3);
    
    HAL_StatusTypeDef status;
    
    // Send frame header
    status = HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)&frame_header, 
                              ROBUST_HEADER_SIZE, ROBUST_UART_TIMEOUT);
    if (status != HAL_OK) {
        g_robust_stats.send_errors++;
        return false;
    }
    
    // Send message header
    status = HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)&msg_header, 
                              ROBUST_MSG_HEADER_SIZE, ROBUST_UART_TIMEOUT);
    if (status != HAL_OK) {
        g_robust_stats.send_errors++;
        return false;
    }
    
    // Send payload data
    if (payload_size > 0) {
        status = HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)payload, 
                                  payload_size, ROBUST_UART_TIMEOUT * 10);
        if (status != HAL_OK) {
            g_robust_stats.send_errors++;
            return false;
        }
    }
    
    // Update statistics
    g_robust_stats.messages_sent++;
    g_robust_stats.bytes_sent += ROBUST_HEADER_SIZE + total_payload_size;
    
    return true;
}

/* ========================================================================= */
/* PUBLIC API FUNCTIONS                                                      */
/* ========================================================================= */

/**
 * @brief Initialize robust PC streaming protocol
 */
void Robust_PC_STREAM_Init(void)
{
    if (g_robust_initialized) {
        return;
    }
    
    BSP_COM_Init(COM1, &RobustPcUartInit);
    
#if (USE_COM_LOG > 0)
    BSP_COM_SelectLogPort(COM1);
#endif
    
    // Clear statistics
    memset(&g_robust_stats, 0, sizeof(g_robust_stats));
    
    g_robust_initialized = true;
    
    // Send initialization heartbeat
    Robust_PC_STREAM_SendHeartbeat();
}

/**
 * @brief Send frame data with robust protocol
 */
bool Robust_PC_STREAM_SendFrame(const uint8_t *frame, uint32_t frame_size,
                                uint32_t width, uint32_t height, 
                                const char *frame_type)
{
    if (!frame || !frame_type || frame_size == 0) {
        return false;
    }
    
    // Prepare frame data header
    robust_frame_data_t frame_data = {
        .width = width,
        .height = height
    };
    
    // Copy frame type (ensure null termination)
    strncpy(frame_data.frame_type, frame_type, 3);
    frame_data.frame_type[3] = '\0';
    
    // Calculate total payload size
    uint32_t total_size = sizeof(robust_frame_data_t) + frame_size;
    
    if (total_size > ROBUST_MAX_PAYLOAD_SIZE - ROBUST_MSG_HEADER_SIZE) {
        g_robust_stats.send_errors++;
        return false;
    }
    
    // Copy data to temporary buffer
    memcpy(robust_temp_buffer, &frame_data, sizeof(robust_frame_data_t));
    memcpy(robust_temp_buffer + sizeof(robust_frame_data_t), frame, frame_size);
    
    return robust_send_message(ROBUST_MSG_FRAME_DATA, robust_temp_buffer, total_size);
}

/**
 * @brief Send detection results with robust protocol
 */
bool Robust_PC_STREAM_SendDetections(uint32_t frame_id, 
                                    const pd_postprocess_out_t *detections)
{
    if (!detections) {
        return false;
    }
    
    uint8_t *buffer = robust_temp_buffer;
    uint32_t offset = 0;
    
    // Prepare detection data header
    robust_detection_data_t det_data = {
        .frame_id = frame_id,
        .detection_count = detections->nb_dets
    };
    
    memcpy(buffer + offset, &det_data, sizeof(robust_detection_data_t));
    offset += sizeof(robust_detection_data_t);
    
    // Add detection data
    for (uint32_t i = 0; i < detections->nb_dets && i < MAX_NUM_DET; i++) {
        const pd_pp_box_t *box = &detections->boxes[i];
        
        robust_detection_t det = {
            .class_id = box->obj_class,
            .x = box->xc,
            .y = box->yc,
            .w = box->w,
            .h = box->h,
            .confidence = box->conf,
            .keypoint_count = 0  // No keypoints for now
        };
        
        if (offset + sizeof(robust_detection_t) > sizeof(robust_temp_buffer)) {
            break;  // Buffer full
        }
        
        memcpy(buffer + offset, &det, sizeof(robust_detection_t));
        offset += sizeof(robust_detection_t);
    }
    
    return robust_send_message(ROBUST_MSG_DETECTION_RESULTS, buffer, offset);
}

/**
 * @brief Send embedding data with robust protocol
 */
bool Robust_PC_STREAM_SendEmbedding(const float *embedding, uint32_t size)
{
    if (!embedding || size == 0 || size > 1024) {
        return false;
    }
    
    uint8_t *buffer = robust_temp_buffer;
    uint32_t offset = 0;
    
    // Prepare embedding data header
    robust_embedding_data_t emb_data = {
        .embedding_size = size
    };
    
    memcpy(buffer + offset, &emb_data, sizeof(robust_embedding_data_t));
    offset += sizeof(robust_embedding_data_t);
    
    // Add embedding data
    uint32_t embedding_bytes = size * sizeof(float);
    if (offset + embedding_bytes > sizeof(robust_temp_buffer)) {
        g_robust_stats.send_errors++;
        return false;
    }
    
    memcpy(buffer + offset, embedding, embedding_bytes);
    offset += embedding_bytes;
    
    return robust_send_message(ROBUST_MSG_EMBEDDING_DATA, buffer, offset);
}

/**
 * @brief Send performance metrics with robust protocol
 */
bool Robust_PC_STREAM_SendPerformanceMetrics(const robust_performance_metrics_t *metrics)
{
    if (!metrics) {
        return false;
    }
    
    return robust_send_message(ROBUST_MSG_PERFORMANCE_METRICS, 
                              (const uint8_t*)metrics, 
                              sizeof(robust_performance_metrics_t));
}

/**
 * @brief Send heartbeat message
 */
void Robust_PC_STREAM_SendHeartbeat(void)
{
    uint32_t timestamp = HAL_GetTick();
    robust_send_message(ROBUST_MSG_HEARTBEAT, (const uint8_t*)&timestamp, sizeof(timestamp));
}

/**
 * @brief Send debug information
 */
void Robust_PC_STREAM_SendDebugInfo(const char *debug_msg)
{
    if (!debug_msg) {
        return;
    }
    
    uint32_t msg_len = strlen(debug_msg);
    if (msg_len > ROBUST_MAX_PAYLOAD_SIZE - ROBUST_MSG_HEADER_SIZE) {
        msg_len = ROBUST_MAX_PAYLOAD_SIZE - ROBUST_MSG_HEADER_SIZE;
    }
    
    robust_send_message(ROBUST_MSG_DEBUG_INFO, (const uint8_t*)debug_msg, msg_len);
}

/**
 * @brief Get protocol statistics
 */
void Robust_PC_STREAM_GetStats(robust_protocol_stats_t *stats)
{
    if (stats) {
        memcpy(stats, &g_robust_stats, sizeof(robust_protocol_stats_t));
    }
}

/**
 * @brief Clear protocol statistics
 */
void Robust_PC_STREAM_ClearStats(void)
{
    memset(&g_robust_stats, 0, sizeof(robust_protocol_stats_t));
}

/* ========================================================================= */
/* LEGACY COMPATIBILITY FUNCTIONS                                            */
/* ========================================================================= */

/**
 * @brief Legacy compatibility function for PC_STREAM_SendFrame
 */
void PC_STREAM_SendFrame(const uint8_t *frame, uint32_t width, uint32_t height, uint32_t bpp)
{
    if (!frame) {
        return;
    }
    
    // Determine if we need to convert/compress the frame
    bool is_full_color = (bpp == 3);  // Assume RGB888
    robust_mem_writer_t writer = {0};
    writer.buffer = robust_jpeg_buffer;
    writer.capacity = sizeof(robust_jpeg_buffer);
    
    uint32_t output_width, output_height;
    
    if (is_full_color) {
        // Full color frame - compress as JPEG
        output_width = width;
        output_height = height;
        stbi_write_jpg_to_func(robust_mem_write_func, &writer, width, height, bpp, frame, JPEG_QUALITY);
        
        // Send compressed frame
        Robust_PC_STREAM_SendFrame(robust_jpeg_buffer, writer.size, output_width, output_height, "JPG");
    } else {
        // Grayscale or RGB565 - downsample and compress
        output_width = width / STREAM_SCALE;
        output_height = height / STREAM_SCALE;
        
        if (output_width * output_height > sizeof(robust_stream_buffer)) {
            output_width = 160;  // Fallback size
            output_height = 120;
        }
        
        // Convert to grayscale and downsample
        for (uint32_t y = 0; y < output_height; y++) {
            const uint8_t *src_line = frame + (y * STREAM_SCALE) * width * bpp;
            for (uint32_t x = 0; x < output_width; x++) {
                if (bpp == 2) {
                    const uint16_t *src_pixel = (const uint16_t *)(src_line + x * STREAM_SCALE * 2);
                    robust_stream_buffer[y * output_width + x] = robust_rgb565_to_gray(*src_pixel);
                } else if (bpp == 3) {
                    const uint8_t *src_pixel = src_line + x * STREAM_SCALE * 3;
                    robust_stream_buffer[y * output_width + x] = robust_rgb888_to_gray(src_pixel[0], src_pixel[1], src_pixel[2]);
                } else {
                    // Grayscale
                    robust_stream_buffer[y * output_width + x] = src_line[x * STREAM_SCALE];
                }
            }
        }
        
        // Compress grayscale data
        writer.size = 0;  // Reset writer
        stbi_write_jpg_to_func(robust_mem_write_func, &writer, output_width, output_height, 1, robust_stream_buffer, JPEG_QUALITY);
        
        // Send compressed frame
        Robust_PC_STREAM_SendFrame(robust_jpeg_buffer, writer.size, output_width, output_height, "JPG");
    }
}

/**
 * @brief Legacy compatibility function for PC_STREAM_SendDetections
 */
void PC_STREAM_SendDetections(const pd_postprocess_out_t *detections, uint32_t frame_id)
{
    Robust_PC_STREAM_SendDetections(frame_id, detections);
}

/**
 * @brief Legacy compatibility function for PC_STREAM_SendEmbedding
 */
void PC_STREAM_SendEmbedding(const float *embedding, uint32_t length)
{
    Robust_PC_STREAM_SendEmbedding(embedding, length);
}

/**
 * @brief Legacy compatibility function for PC_STREAM_Init
 */
void PC_STREAM_Init(void)
{
    Robust_PC_STREAM_Init();
}

#endif /* USE_BSP_COM_FEATURE */