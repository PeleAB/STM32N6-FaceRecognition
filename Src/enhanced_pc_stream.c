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
#include "stm32n6xx_hal_crc.h"
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
#define ROBUST_HEADER_SIZE          4   // Back to 4 bytes for header only
#define ROBUST_CRC_SIZE             4   // CRC32 at end of packet
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
 * @brief Robust 4-byte frame header structure (CRC32 follows payload)
 */
typedef struct __attribute__((packed)) {
    uint8_t sof;                /* Start of Frame (0xAA) */
    uint16_t payload_size;      /* Payload size in bytes (not including CRC32) */
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

/* CRC handle for payload validation */
static CRC_HandleTypeDef hcrc;

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
 * @brief Initialize CRC32 peripheral
 */
static bool crc32_init(void)
{
    hcrc.Instance = CRC;
    hcrc.Init.DefaultPolynomialUse = DEFAULT_POLYNOMIAL_ENABLE;
    hcrc.Init.DefaultInitValueUse = DEFAULT_INIT_VALUE_ENABLE;
    hcrc.Init.InputDataInversionMode = CRC_INPUTDATA_INVERSION_NONE;
    hcrc.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;
    hcrc.InputDataFormat = CRC_INPUTDATA_FORMAT_BYTES;
    
    __HAL_RCC_CRC_CLK_ENABLE();
    
    if (HAL_CRC_Init(&hcrc) != HAL_OK) {
        return false;
    }
    
    return true;
}

/**
 * @brief Calculate CRC32 for payload data
 */
static uint32_t calculate_crc32(const uint8_t *data, uint32_t length)
{
    if (!data || length == 0) {
        return 0;
    }
    
    return HAL_CRC_Calculate(&hcrc, (uint32_t*)data, (length + 3) / 4);
}

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
 * @brief Send raw message with robust header and CRC32 at packet end
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
    
    // Calculate total payload size (message header + payload data, not including CRC32)
    uint32_t total_payload_size = ROBUST_MSG_HEADER_SIZE + payload_size;
    
    // Validate payload size
    if (total_payload_size > ROBUST_MAX_PAYLOAD_SIZE) {
        g_protocol_ctx.stats.crc_errors++; // Reuse for send errors
        return false;
    }
    
    // Check if we have enough buffer space for payload + CRC32
    if (total_payload_size + ROBUST_CRC_SIZE > sizeof(temp_buffer)) {
        g_protocol_ctx.stats.crc_errors++; // Buffer too small
        return false;
    }
    
    // Prepare complete payload (message header + payload data)
    uint8_t *complete_payload = temp_buffer;
    memcpy(complete_payload, &msg_header, ROBUST_MSG_HEADER_SIZE);
    if (payload_size > 0) {
        memcpy(complete_payload + ROBUST_MSG_HEADER_SIZE, payload, payload_size);
    }
    
    // Calculate CRC32 for the payload (before appending CRC)
    uint32_t payload_crc32 = calculate_crc32(complete_payload, total_payload_size);
    
    // Append CRC32 at the end of payload (little endian)
    uint32_t crc32_le = payload_crc32;  // STM32 is little endian
    memcpy(complete_payload + total_payload_size, &crc32_le, ROBUST_CRC_SIZE);
    
    // Prepare frame header (payload size does NOT include CRC32)
    robust_frame_header_t frame_header;
    frame_header.sof = ROBUST_SOF_BYTE;
    frame_header.payload_size = (uint16_t)total_payload_size;  // Payload only, not CRC
    frame_header.header_checksum = 0;
    
    // Calculate header checksum
    uint8_t header_data[3];
    header_data[0] = frame_header.sof;
    header_data[1] = (uint8_t)(frame_header.payload_size & 0xFF);
    header_data[2] = (uint8_t)((frame_header.payload_size >> 8) & 0xFF);
    frame_header.header_checksum = robust_calculate_checksum(header_data, 3);
    
    HAL_StatusTypeDef status;
    
    // Send frame header as atomic operation
    status = HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)&frame_header, 
                              ROBUST_HEADER_SIZE, UART_TIMEOUT);
    if (status != HAL_OK) {
        g_protocol_ctx.stats.crc_errors++; // Reuse for send errors
        return false;
    }
    
    // Send payload + CRC32 in chunks to avoid UART buffer overflow
    uint32_t total_data_size = total_payload_size + ROBUST_CRC_SIZE;
    if (total_data_size > 0) {
        uint32_t bytes_sent = 0;
        const uint32_t chunk_size = CHUNK_SIZE; // Send chunks for better efficiency
        
        while (bytes_sent < total_data_size) {
            uint32_t chunk_len = (total_data_size - bytes_sent > chunk_size) ? 
                                chunk_size : (total_data_size - bytes_sent);
            
            status = HAL_UART_Transmit(&hcom_uart[COM1], 
                                      complete_payload + bytes_sent, 
                                      chunk_len, UART_TIMEOUT);
            if (status != HAL_OK) {
                g_protocol_ctx.stats.crc_errors++; // Reuse for send errors
                return false;
            }
            
            bytes_sent += chunk_len;
            
            // Minimal delay only for very large payloads
            if (bytes_sent < total_data_size && chunk_len >= 4096) {
                HAL_Delay(1);
            }
        }
    }
    
    // Update statistics
    g_protocol_ctx.stats.packets_sent++;
    g_protocol_ctx.stats.bytes_sent += ROBUST_HEADER_SIZE + total_payload_size + ROBUST_CRC_SIZE;
    
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
    
    // Initialize CRC32 peripheral
    if (!crc32_init()) {
        printf("Failed to initialize CRC32 peripheral\n");
        return;
    }
    
    // Clear statistics
    memset(&g_protocol_ctx.stats, 0, sizeof(g_protocol_ctx.stats));
    memset(g_protocol_ctx.sequence_counters, 0, sizeof(g_protocol_ctx.sequence_counters));
    
    g_protocol_ctx.initialized = true;
    
    printf("Enhanced PC streaming initialized with CRC32 validation\n");
    
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
    
    // DEBUG: Skip all image processing and generate test pattern for any frame type
    // This ensures the test pattern is sent regardless of "JPG" or "ALN" frame type
    
    // Set dummy dimensions for test pattern
    uint32_t output_width = 160;
    uint32_t output_height = 120;
    
    // DEBUG: Create artificial test buffer instead of JPEG compression
    // This will help us detect if bytes are missing or scrambled during transmission
    
    writer.size = 0;
    uint32_t test_size = 1024; // 1KB test buffer
    
    // Ensure we don't exceed buffer capacity
    if (test_size > writer.capacity) {
        test_size = writer.capacity;
    }
    
    // Clear the buffer first to ensure clean state
    memset(writer.buffer, 0, test_size);
    
    // Pattern 1: Sequential bytes (0x00, 0x01, 0x02, ...)
    for (uint32_t i = 0; i < test_size / 4; i++) {
        writer.buffer[i] = i & 0xFF;
    }
    
    // Pattern 2: Alternating pattern (0xAA, 0x55, 0xAA, 0x55, ...)
    for (uint32_t i = test_size / 4; i < test_size / 2; i++) {
        writer.buffer[i] = (i % 2) ? 0x55 : 0xAA;
    }
    
    // Pattern 3: Known sequence with markers (0xDEADBEEF pattern)
    uint32_t *pattern_ptr = (uint32_t *)(writer.buffer + test_size / 2);
    for (uint32_t i = 0; i < (test_size / 2) / 4; i++) {
        pattern_ptr[i] = 0xDEADBEEF + i;
    }
    
    // Add sync markers at key positions to detect shifts
    writer.buffer[0] = 0xFF;           // Start marker
    writer.buffer[1] = 0xD8;           // JPEG SOI marker (for pattern recognition)
    writer.buffer[test_size - 2] = 0xFF;  // End marker
    writer.buffer[test_size - 1] = 0xD9;   // JPEG EOI marker
    
    // Add position markers every 100 bytes to detect missing chunks
    for (uint32_t i = 100; i < test_size - 100; i += 100) {
        writer.buffer[i] = 0xFE;       // Position marker
        writer.buffer[i + 1] = (i / 100) & 0xFF;  // Position ID
    }
    
    writer.size = test_size;  // Set the size directly instead of using JPEG compression
    
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
    memcpy(temp_buffer + sizeof(robust_frame_data_t), writer.buffer, writer.size);
    
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
    
    return robust_send_message(ROBUST_MSG_EMBEDDING_DATA, buffer, offset);
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
    
    return robust_send_message(ROBUST_MSG_DETECTION_RESULTS, buffer, offset);
}

/**
 * @brief Send performance metrics
 */
bool Enhanced_PC_STREAM_SendPerformanceMetrics(const performance_metrics_t *metrics)
{
    if (!metrics) {
        return false;
    }
    
    return robust_send_message(ROBUST_MSG_PERFORMANCE_METRICS,
                              (const uint8_t*)metrics, 
                              sizeof(performance_metrics_t));
}

/**
 * @brief Send periodic heartbeat packet
 */
void Enhanced_PC_STREAM_SendHeartbeat(void)
{
    uint32_t timestamp = HAL_GetTick();
    robust_send_message(ROBUST_MSG_HEARTBEAT, (const uint8_t*)&timestamp, sizeof(timestamp));
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
