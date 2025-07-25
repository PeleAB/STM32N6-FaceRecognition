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

/* ========================================================================= */
/* CONFIGURATION CONSTANTS                                                   */
/* ========================================================================= */

#define ROBUST_SOF_BYTE             0xAA
#define ROBUST_HEADER_SIZE          4   // Back to 4 bytes for header only
#define ROBUST_CRC_SIZE             4   // CRC32 at end of packet
#define ROBUST_MAX_PAYLOAD_SIZE     (64 * 1024)
#define ROBUST_MSG_HEADER_SIZE      3
#define UART_TIMEOUT                1000
#define STREAM_SCALE                2
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
 * @brief Frame data payload format for raw grayscale frames
 */
typedef struct __attribute__((packed)) {
    char frame_type[4];         /* Frame type: "JPG", "ALN", etc. */
    uint32_t width;             /* Frame width */
    uint32_t height;            /* Frame height */
    /* Raw grayscale image data follows (1 byte per pixel) */
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
static uint8_t temp_buffer[64 * 1024];

__attribute__ ((section (".psram_bss")))
__attribute__((aligned (32)))
static uint8_t stream_buffer[320 * 240];  // Raw grayscale frame buffer

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

    hcrc.Init.CRCLength=CRC_POLYLENGTH_32B;

    hcrc.Init.InputDataInversionMode = CRC_INPUTDATA_INVERSION_NONE;

    hcrc.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;

    hcrc.InputDataFormat = CRC_INPUTDATA_FORMAT_WORDS;
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
    
    return HAL_CRC_Calculate(&hcrc, (uint32_t*)data, (length+3)/4);
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
    
    // Calculate total payload size (message header + payload data, not including CRC32)
    uint32_t total_payload_size = ROBUST_MSG_HEADER_SIZE + payload_size;
    
    // Validate payload size
    if (total_payload_size > ROBUST_MAX_PAYLOAD_SIZE) {
        g_protocol_ctx.stats.crc_errors++; // Reuse for send errors
        return false;
    }
    
    // Prepare message header
    uint8_t msg_header[ROBUST_MSG_HEADER_SIZE];
    msg_header[0] = (uint8_t)message_type;
    uint16_t sequence_id = get_next_sequence_id(message_type);
    msg_header[1] = (uint8_t)(sequence_id & 0xFF);
    msg_header[2] = (uint8_t)((sequence_id >> 8) & 0xFF);
    
    // Calculate CRC32 only on payload data (header has its own checksum)
    uint32_t payload_crc32 = 0;
//    uint8_t test_data[4] = {0x01, 0x02, 0x03, 0x04};
    if (payload_size > 0) {
        payload_crc32 = calculate_crc32(payload, payload_size);
    }
    
    // Prepare CRC32 bytes (little endian)
    uint8_t crc32_bytes[ROBUST_CRC_SIZE];
    crc32_bytes[0] = (uint8_t)(payload_crc32 & 0xFF);
    crc32_bytes[1] = (uint8_t)((payload_crc32 >> 8) & 0xFF);
    crc32_bytes[2] = (uint8_t)((payload_crc32 >> 16) & 0xFF);
    crc32_bytes[3] = (uint8_t)((payload_crc32 >> 24) & 0xFF);
    
    // Prepare frame header
    uint8_t frame_header[ROBUST_HEADER_SIZE];
    frame_header[0] = ROBUST_SOF_BYTE;
    frame_header[1] = (uint8_t)(total_payload_size & 0xFF);
    frame_header[2] = (uint8_t)((total_payload_size >> 8) & 0xFF);
    frame_header[3] = frame_header[0] ^ frame_header[1] ^ frame_header[2]; // XOR checksum
    
    HAL_StatusTypeDef status;
    
    // Send frame header
    status = HAL_UART_Transmit(&hcom_uart[COM1], frame_header, 
                              ROBUST_HEADER_SIZE, UART_TIMEOUT);
    if (status != HAL_OK) {
        g_protocol_ctx.stats.crc_errors++; // Reuse for send errors
        return false;
    }
    
    // Send message header
    status = HAL_UART_Transmit(&hcom_uart[COM1], msg_header, 
                              ROBUST_MSG_HEADER_SIZE, UART_TIMEOUT);
    if (status != HAL_OK) {
        g_protocol_ctx.stats.crc_errors++; // Reuse for send errors
        return false;
    }
    
    // Send payload data if any
    if (payload_size > 0) {
        status = HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)payload, 
                                  payload_size, UART_TIMEOUT);
        if (status != HAL_OK) {
            g_protocol_ctx.stats.crc_errors++; // Reuse for send errors
            return false;
        }
    }
    
    // Send CRC32
    status = HAL_UART_Transmit(&hcom_uart[COM1], crc32_bytes, 
                              ROBUST_CRC_SIZE, UART_TIMEOUT);
    if (status != HAL_OK) {
        g_protocol_ctx.stats.crc_errors++; // Reuse for send errors
        return false;
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
 * @brief Send frame with enhanced protocol as raw grayscale data
 */
bool Enhanced_PC_STREAM_SendFrame(const uint8_t *frame, uint32_t width, uint32_t height,
                                 uint32_t bpp, const char *tag,
                                 const pd_postprocess_out_t *detections,
                                 const performance_metrics_t *performance)
{
    if (!frame || !tag) {
        return false;
    }
    
    // Determine scaling based on frame type (ALN frames are full resolution)
    bool full_resolution = (strcmp(tag, "ALN") == 0);
    uint32_t scale_factor = full_resolution ? 1 : STREAM_SCALE;
    
    uint32_t output_width = width / scale_factor;
    uint32_t output_height = height / scale_factor;
    
    if (output_width > 320) output_width = 320;   // Max width limit
    if (output_height > 240) output_height = 240; // Max height limit
    
    // Convert to grayscale and send raw data
    for (uint32_t y = 0; y < output_height; y++) {
        const uint8_t *line = frame + (y * scale_factor) * width * bpp;
        for (uint32_t x = 0; x < output_width; x++) {
            if (bpp == 2) {
                const uint16_t *line16 = (const uint16_t *)line;
                uint16_t px = line16[x * scale_factor];
                stream_buffer[y * output_width + x] = rgb565_to_gray(px);
            } else if (bpp == 3) {
                const uint8_t *px = line + x * scale_factor * 3;
                stream_buffer[y * output_width + x] = rgb888_to_gray(px[0], px[1], px[2]);
            } else {
                stream_buffer[y * output_width + x] = line[x * scale_factor];
            }
        }
    }
    
    // Prepare frame data header
    robust_frame_data_t frame_data = {
        .width = output_width,
        .height = output_height
    };
    
    // Copy frame type (preserve original tag for different frame types)
    strncpy(frame_data.frame_type, tag, 3);
    frame_data.frame_type[3] = '\0';
    
    // Calculate total payload size using raw grayscale data
    uint32_t raw_data_size = output_width * output_height; // 1 byte per pixel
    uint32_t total_size = sizeof(robust_frame_data_t) + raw_data_size;
    
    if (total_size > ROBUST_MAX_PAYLOAD_SIZE - ROBUST_MSG_HEADER_SIZE) {
        g_protocol_ctx.stats.crc_errors++; // Reuse for send errors
        return false;
    }
    
    // Build payload directly in temp_buffer with raw grayscale data
    memcpy(temp_buffer, &frame_data, sizeof(robust_frame_data_t));
    memcpy(temp_buffer + sizeof(robust_frame_data_t), stream_buffer, raw_data_size);
    
    bool frame_sent = robust_send_message(ROBUST_MSG_FRAME_DATA, temp_buffer, total_size);
    
    // Send performance metrics if available
    if (performance) {
        Enhanced_PC_STREAM_SendPerformanceMetrics(performance);
    }
    
    // Send detections if available
    if (detections && detections->box_nb > 0) {
        Enhanced_PC_STREAM_SendDetections(0, detections);  // Frame ID = 0 for now
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
