/**
 ******************************************************************************
 * @file    enhanced_pc_stream.c
 * @brief   Enhanced PC streaming with robust 4-byte header protocol
 ******************************************************************************
 */

#include "enhanced_pc_stream.h"
#include "stm32n6570_discovery.h"
#include "stm32n6570_discovery_conf.h"
#include "stm32n6xx_hal_uart.h"
#include "app_config.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

/* ========================================================================= */
/* CONFIGURATION CONSTANTS                                                   */
/* ========================================================================= */

#define ROBUST_SOF_BYTE             0xAA
#define ROBUST_HEADER_SIZE          4
#define ROBUST_MAX_PAYLOAD_SIZE     (64 * 1024)
#define ROBUST_MSG_HEADER_SIZE      3
#define UART_TIMEOUT                1000
#define JPEG_QUALITY                80  // High quality for better visual experience
#define STREAM_SCALE                2
#define CHUNK_SIZE                  255  // adjust as needed
/* ========================================================================= */
/* MESSAGE TYPES                                                             */
/* ========================================================================= */

typedef enum {
    ROBUST_MSG_FRAME_DATA = 0x01,
    ROBUST_MSG_DETECTION_RESULTS = 0x02,
    ROBUST_MSG_EMBEDDING_DATA = 0x03,
    ROBUST_MSG_PERFORMANCE_METRICS = 0x04,
    ROBUST_MSG_HEARTBEAT = 0x05,
    ROBUST_MSG_ERROR_REPORT = 0x06,
    ROBUST_MSG_COMMAND_REQUEST = 0x07,
    ROBUST_MSG_COMMAND_RESPONSE = 0x08,
    ROBUST_MSG_DEBUG_INFO = 0x09
} robust_message_type_t;

/* ========================================================================= */
/* DATA STRUCTURES                                                           */
/* ========================================================================= */

/**
 * @brief Robust 4-byte frame header structure
 */
typedef struct __attribute__((packed)) {
    uint8_t sof;                /* Start of Frame (0xAA) */
    uint16_t payload_size;      /* Payload size in bytes */
    uint8_t header_checksum;    /* XOR checksum of SOF + payload_size */
} robust_frame_header_t;

/**
 * @brief Message header within payload (3 bytes)
 */
typedef struct __attribute__((packed)) {
    uint8_t message_type;       /* Message type */
    uint16_t sequence_id;       /* Sequence ID for message ordering */
} robust_message_header_t;

/**
 * @brief Frame data payload format
 */
typedef struct __attribute__((packed)) {
    char frame_type[4];         /* "JPG", "ALN", etc. */
    uint32_t width;             /* Frame width */
    uint32_t height;            /* Frame height */
    /* Image data follows */
} robust_frame_data_t;

/**
 * @brief Embedding data payload format
 */
typedef struct __attribute__((packed)) {
    uint32_t embedding_size;    /* Number of float values */
    /* Embedding data follows (float array) */
} robust_embedding_data_t;

/**
 * @brief Enhanced protocol context
 */
typedef struct {
    protocol_stats_t stats;
    bool initialized;
    uint32_t last_heartbeat_time;
    uint16_t sequence_counters[16]; /* Sequence counters per message type */
} enhanced_protocol_ctx_t;

/* ========================================================================= */
/* GLOBAL VARIABLES                                                          */
/* ========================================================================= */

#if (USE_BSP_COM_FEATURE > 0)
extern UART_HandleTypeDef hcom_uart[COMn];

static MX_UART_InitTypeDef PcUartInit = {
    .BaudRate = 921600 * 8,
    .WordLength = UART_WORDLENGTH_8B,
    .StopBits = UART_STOPBITS_1,
    .Parity = UART_PARITY_NONE,
    .HwFlowCtl = UART_HWCONTROL_NONE
};

/* Protocol context */
static enhanced_protocol_ctx_t g_protocol_ctx = {0};

/* Buffers */
__attribute__ ((section (".psram_bss")))
__attribute__((aligned (32)))
static uint8_t jpeg_buffer[64 * 512];

__attribute__ ((section (".psram_bss")))
__attribute__((aligned (32)))
static uint8_t temp_buffer[64 * 1024];

__attribute__ ((section (".psram_bss")))
__attribute__((aligned (32)))
static uint8_t stream_buffer[320 * 240];  // Downscaled frame buffer

/* ========================================================================= */
/* UTILITY FUNCTIONS                                                         */
/* ========================================================================= */

/**
 * @brief Calculate XOR checksum for robust protocol
 */
static uint8_t robust_calculate_checksum(const uint8_t *data, uint32_t length)
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
        return ++g_protocol_ctx.sequence_counters[msg_type];
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
} mem_writer_t;

static void mem_write_func(void *context, void *data, int size)
{
    mem_writer_t *writer = (mem_writer_t *)context;
    if (writer->size + size <= writer->capacity) {
        memcpy(writer->buffer + writer->size, data, size);
        writer->size += size;
    }
}

/**
 * @brief Convert RGB565 to grayscale
 */
static uint8_t rgb565_to_gray(uint16_t pixel)
{
    uint8_t r = ((pixel >> 11) & 0x1F) << 3;
    uint8_t g = ((pixel >> 5) & 0x3F) << 2;
    uint8_t b = (pixel & 0x1F) << 3;
    return (uint8_t)((r * 30 + g * 59 + b * 11) / 100);
}

/**
 * @brief Convert RGB888 to grayscale
 */
static uint8_t rgb888_to_gray(uint8_t r, uint8_t g, uint8_t b)
{
    return (uint8_t)((r * 30 + g * 59 + b * 11) / 100);
}

/* ========================================================================= */
/* CORE PROTOCOL FUNCTIONS                                                   */
/* ========================================================================= */

/**
 * @brief Send raw message with robust 4-byte header framing
 */
static bool robust_send_message(robust_message_type_t message_type, 
                               const uint8_t *payload, uint32_t payload_size)
{
    if (!g_protocol_ctx.initialized) {
        return false;
    }
    
    if (payload_size > ROBUST_MAX_PAYLOAD_SIZE - ROBUST_MSG_HEADER_SIZE) {
        g_protocol_ctx.stats.crc_errors++; // Reuse for send errors
        return false;
    }
    
    // Prepare message header
    robust_message_header_t msg_header = {
        .message_type = (uint8_t)message_type,
        .sequence_id = get_next_sequence_id(message_type)
    };
    
    // Calculate total payload size (message header + payload)
    uint32_t total_payload_size = ROBUST_MSG_HEADER_SIZE + payload_size;
    
    // Validate payload size
    if (total_payload_size > ROBUST_MAX_PAYLOAD_SIZE) {
        g_protocol_ctx.stats.crc_errors++; // Reuse for send errors
        return false;
    }
    
    // Prepare frame header with proper byte ordering
    robust_frame_header_t frame_header;
    frame_header.sof = ROBUST_SOF_BYTE;
    frame_header.payload_size = (uint16_t)total_payload_size;
    frame_header.header_checksum = 0;
    
    // Calculate header checksum using exact same method as Python
    // SOF(1 byte) + payload_size(2 bytes, little endian)
    uint8_t header_data[3];
    header_data[0] = frame_header.sof;                              // SOF byte
    header_data[1] = (uint8_t)(frame_header.payload_size & 0xFF);   // Low byte of payload_size
    header_data[2] = (uint8_t)((frame_header.payload_size >> 8) & 0xFF); // High byte of payload_size
    frame_header.header_checksum = robust_calculate_checksum(header_data, 3);
    
    HAL_StatusTypeDef status;
    
    // Send frame header as atomic operation
    status = HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)&frame_header, 
                              ROBUST_HEADER_SIZE, UART_TIMEOUT);
    if (status != HAL_OK) {
        g_protocol_ctx.stats.crc_errors++; // Reuse for send errors
        return false;
    }
    
    // Send message header immediately after frame header
    status = HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)&msg_header, 
                              ROBUST_MSG_HEADER_SIZE, UART_TIMEOUT);
    if (status != HAL_OK) {
        g_protocol_ctx.stats.crc_errors++; // Reuse for send errors
        return false;
    }
    
    // Send payload data in chunks to avoid UART buffer overflow
    if (payload_size > 0) {
        uint32_t bytes_sent = 0;
        const uint32_t chunk_size = CHUNK_SIZE; // Send 8KB chunks for better efficiency
        
        while (bytes_sent < payload_size) {
            uint32_t chunk_len = (payload_size - bytes_sent > chunk_size) ? 
                                chunk_size : (payload_size - bytes_sent);
            
            status = HAL_UART_Transmit(&hcom_uart[COM1], 
                                      (uint8_t*)(payload + bytes_sent), 
                                      chunk_len, UART_TIMEOUT);
            if (status != HAL_OK) {
                g_protocol_ctx.stats.crc_errors++; // Reuse for send errors
                return false;
            }
            
            bytes_sent += chunk_len;
            
            // Minimal delay only for very large payloads
            if (bytes_sent < payload_size && chunk_len >= 4096) {
                HAL_Delay(1);
            }
        }
    }
    
    // Update statistics
    g_protocol_ctx.stats.packets_sent++;
    g_protocol_ctx.stats.bytes_sent += ROBUST_HEADER_SIZE + total_payload_size;
    
    return true;
}

/* ========================================================================= */
/* PUBLIC API FUNCTIONS                                                      */
/* ========================================================================= */

/**
 * @brief Initialize enhanced PC streaming protocol
 */
void Enhanced_PC_STREAM_Init(void)
{
    if (g_protocol_ctx.initialized) {
        return;
    }
    
    BSP_COM_Init(COM1, &PcUartInit);
    
#if (USE_COM_LOG > 0)
    BSP_COM_SelectLogPort(COM1);
#endif
    
    // Clear statistics
    memset(&g_protocol_ctx.stats, 0, sizeof(g_protocol_ctx.stats));
    memset(g_protocol_ctx.sequence_counters, 0, sizeof(g_protocol_ctx.sequence_counters));
    
    g_protocol_ctx.initialized = true;
    
    // Send initialization heartbeat
    Enhanced_PC_STREAM_SendHeartbeat();
}

/**
 * @brief Send frame with enhanced protocol including metadata
 */
bool Enhanced_PC_STREAM_SendFrame(const uint8_t *frame, uint32_t width, uint32_t height,
                                 uint32_t bpp, const char *tag,
                                 const pd_postprocess_out_t *detections,
                                 const performance_metrics_t *performance)
{
    if (!frame || !tag) {
        return false;
    }
    
    // Convert to grayscale or keep color based on frame type
    mem_writer_t writer = {0};
    writer.buffer = jpeg_buffer;
    writer.capacity = sizeof(jpeg_buffer);
    
    // Check if this is an alignment frame (ALN) - keep full color and resolution
    bool is_alignment_frame = (strcmp(tag, "ALN") == 0);
    uint32_t output_width = is_alignment_frame ? width : (width / STREAM_SCALE);
    uint32_t output_height = is_alignment_frame ? height : (height / STREAM_SCALE);
    
    if (output_width * output_height > sizeof(stream_buffer)) {
        output_width = 160;  // Fallback size
        output_height = 120;
    }
    
    // Handle color vs grayscale based on frame type
    if (is_alignment_frame) {
        // Keep full color and resolution for alignment frames
        if (bpp == 2) {
            // RGB565 to RGB888 for better quality
            for (uint32_t y = 0; y < output_height; y++) {
                const uint8_t *src_line = frame + y * width * bpp;
                uint8_t *dst_line = stream_buffer + y * output_width * 3;
                for (uint32_t x = 0; x < output_width; x++) {
                    const uint16_t *src_pixel = (const uint16_t *)(src_line + x * 2);
                    uint16_t pixel = *src_pixel;
                    dst_line[x * 3 + 0] = ((pixel >> 11) & 0x1F) << 3;  // R
                    dst_line[x * 3 + 1] = ((pixel >> 5) & 0x3F) << 2;   // G
                    dst_line[x * 3 + 2] = (pixel & 0x1F) << 3;          // B
                }
            }
        } else {
            // Copy RGB888 or grayscale directly
            uint32_t bytes_to_copy = output_width * output_height * bpp;
            if (bytes_to_copy <= sizeof(stream_buffer)) {
                memcpy(stream_buffer, frame, bytes_to_copy);
            }
        }
    } else {
        // Convert to grayscale and downsample for regular frames
        for (uint32_t y = 0; y < output_height; y++) {
            const uint8_t *src_line = frame + (y * STREAM_SCALE) * width * bpp;
            for (uint32_t x = 0; x < output_width; x++) {
                if (bpp == 2) {
                    // RGB565 to grayscale
                    const uint16_t *src_pixel = (const uint16_t *)(src_line + x * STREAM_SCALE * 2);
                    stream_buffer[y * output_width + x] = rgb565_to_gray(*src_pixel);
                } else if (bpp == 3) {
                    // RGB888 to grayscale
                    const uint8_t *src_pixel = src_line + x * STREAM_SCALE * 3;
                    stream_buffer[y * output_width + x] = rgb888_to_gray(src_pixel[0], src_pixel[1], src_pixel[2]);
                } else {
                    // Already grayscale
                    stream_buffer[y * output_width + x] = src_line[x * STREAM_SCALE];
                }
            }
        }
    }
    
    // Compress to JPEG (grayscale or color based on frame type)
    writer.size = 0;
    int channels = is_alignment_frame ? (bpp == 2 ? 3 : bpp) : 1;
    stbi_write_jpg_to_func(mem_write_func, &writer, output_width, output_height, channels, stream_buffer, JPEG_QUALITY);
    
    // Prepare frame data header
    robust_frame_data_t frame_data = {
        .width = output_width,
        .height = output_height
    };
    
    // Copy frame type (ensure null termination)
    strncpy(frame_data.frame_type, tag, 3);
    frame_data.frame_type[3] = '\0';
    
    // Calculate total payload size
    uint32_t total_size = sizeof(robust_frame_data_t) + writer.size;
    
    if (total_size > ROBUST_MAX_PAYLOAD_SIZE - ROBUST_MSG_HEADER_SIZE) {
        g_protocol_ctx.stats.crc_errors++; // Reuse for send errors
        return false;
    }
    
    // Copy data to temporary buffer
    memcpy(temp_buffer, &frame_data, sizeof(robust_frame_data_t));
    memcpy(temp_buffer + sizeof(robust_frame_data_t), jpeg_buffer, writer.size);
    
    bool frame_sent = robust_send_message(ROBUST_MSG_FRAME_DATA, temp_buffer, total_size);
    
    // Send detections if available
    if (detections && detections->box_nb > 0) {
        Enhanced_PC_STREAM_SendDetections(0, detections);  // Frame ID = 0 for now
    }
    
    // Send performance metrics if available
    if (performance) {
        Enhanced_PC_STREAM_SendPerformanceMetrics(performance);
    }
    
    return frame_sent;
}

/**
 * @brief Send embedding data with metadata
 */
bool Enhanced_PC_STREAM_SendEmbedding(const float *embedding, uint32_t size)
{
    if (!embedding || size == 0 || size > 1024) {
        return false;
    }
    
    uint8_t *buffer = temp_buffer;
    uint32_t offset = 0;
    
    // Prepare embedding data header
    robust_embedding_data_t emb_data = {
        .embedding_size = size
    };
    
    memcpy(buffer + offset, &emb_data, sizeof(robust_embedding_data_t));
    offset += sizeof(robust_embedding_data_t);
    
    // Add embedding data
    uint32_t embedding_bytes = size * sizeof(float);
    if (offset + embedding_bytes > sizeof(temp_buffer)) {
        g_protocol_ctx.stats.crc_errors++; // Reuse for send errors
        return false;
    }
    
    memcpy(buffer + offset, embedding, embedding_bytes);
    offset += embedding_bytes;
    return 0;
    //return robust_send_message(ROBUST_MSG_EMBEDDING_DATA, buffer, offset);
}

/**
 * @brief Send detection results with robust protocol
 */
bool Enhanced_PC_STREAM_SendDetections(uint32_t frame_id, const pd_postprocess_out_t *detections)
{
    if (!detections || detections->box_nb == 0) {
        return false;
    }
    
    uint8_t *buffer = temp_buffer;
    uint32_t offset = 0;
    
    // Prepare detection data header
    struct __attribute__((packed)) {
        uint32_t frame_id;
        uint32_t detection_count;
    } det_header = {
        .frame_id = frame_id,
        .detection_count = detections->box_nb
    };
    
    memcpy(buffer + offset, &det_header, sizeof(det_header));
    offset += sizeof(det_header);
    
    // Add detection data (limit to reasonable number)
    uint32_t max_detections = 10;  // Reasonable limit for streaming
    for (uint32_t i = 0; i < detections->box_nb && i < max_detections; i++) {
        const pd_pp_box_t *box = &detections->pOutData[i];
        
        struct __attribute__((packed)) {
            uint32_t class_id;
            float x, y, w, h;
            float confidence;
            uint32_t keypoint_count;
        } det = {
            .class_id = 0,  // Default class (person detection)
            .x = box->x_center,
            .y = box->y_center,
            .w = box->width,
            .h = box->height,
            .confidence = box->prob,
            .keypoint_count = 0  // No keypoints for now
        };
        
        if (offset + sizeof(det) > sizeof(temp_buffer)) {
            break;  // Buffer full
        }
        
        memcpy(buffer + offset, &det, sizeof(det));
        offset += sizeof(det);
    }
    return 0;
    //return robust_send_message(ROBUST_MSG_DETECTION_RESULTS, buffer, offset);
}

/**
 * @brief Send performance metrics
 */
bool Enhanced_PC_STREAM_SendPerformanceMetrics(const performance_metrics_t *metrics)
{
    if (!metrics) {
        return false;
    }
    return 0;
    /*return robust_send_message(ROBUST_MSG_PERFORMANCE_METRICS,
                              (const uint8_t*)metrics, 
                              sizeof(performance_metrics_t));*/
}

/**
 * @brief Send periodic heartbeat packet
 */
void Enhanced_PC_STREAM_SendHeartbeat(void)
{
    uint32_t timestamp = HAL_GetTick();
    //robust_send_message(ROBUST_MSG_HEARTBEAT, (const uint8_t*)&timestamp, sizeof(timestamp));
    g_protocol_ctx.last_heartbeat_time = timestamp;
}

/**
 * @brief Get protocol statistics
 */
void Enhanced_PC_STREAM_GetStats(protocol_stats_t *stats)
{
    if (stats) {
        memcpy(stats, &g_protocol_ctx.stats, sizeof(protocol_stats_t));
    }
}

/**
 * @brief Legacy compatibility function for existing code
 */
void Enhanced_PC_STREAM_SendFrameEx(const uint8_t *frame, uint32_t width, uint32_t height,
                                   uint32_t bpp, const char *tag)
{
    Enhanced_PC_STREAM_SendFrame(frame, width, height, bpp, tag, NULL, NULL);
}

#endif /* USE_BSP_COM_FEATURE */
