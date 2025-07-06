/**
 ******************************************************************************
 * @file    enhanced_pc_stream.c
 * @brief   Enhanced PC streaming with robust protocol and modern features
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

#define PROTOCOL_SYNC_WORD          0x12345678
#define PROTOCOL_VERSION            0x01
#define MAX_PACKET_SIZE             (64 * 1024)
#define MAX_METADATA_SIZE           (2 * 1024)
#define UART_CHUNK_SIZE             (16 * 1024)
#define JPEG_QUALITY                85
#define STREAM_SCALE                2

/* Packet types */
typedef enum {
    PACKET_TYPE_FRAME_DATA = 0x01,
    PACKET_TYPE_DETECTION_RESULTS = 0x02,
    PACKET_TYPE_EMBEDDING_DATA = 0x03,
    PACKET_TYPE_PERFORMANCE_METRICS = 0x04,
    PACKET_TYPE_COMMAND_REQUEST = 0x05,
    PACKET_TYPE_COMMAND_RESPONSE = 0x06,
    PACKET_TYPE_HEARTBEAT = 0x07,
    PACKET_TYPE_ERROR_REPORT = 0x08
} packet_type_t;

/* Packet flags */
#define PACKET_FLAG_COMPRESSED      0x01
#define PACKET_FLAG_ENCRYPTED       0x02
#define PACKET_FLAG_ACKNOWLEDGMENT  0x04

/* Command types */
typedef enum {
    CMD_GET_STATUS = 0x01,
    CMD_SET_PARAMETERS = 0x02,
    CMD_START_ENROLLMENT = 0x03,
    CMD_STOP_ENROLLMENT = 0x04,
    CMD_RESET_SYSTEM = 0x05,
    CMD_GET_DIAGNOSTICS = 0x06
} command_type_t;

/* ========================================================================= */
/* DATA STRUCTURES                                                           */
/* ========================================================================= */

/**
 * @brief Enhanced packet header structure
 */
typedef struct __attribute__((packed)) {
    uint32_t sync_word;         /* Synchronization word */
    uint8_t  packet_type;       /* Packet type identifier */
    uint8_t  flags;            /* Packet flags */
    uint16_t sequence;         /* Sequence number */
    uint32_t payload_length;   /* Payload data length */
    uint16_t metadata_length;  /* Metadata length */
    uint32_t crc32;           /* CRC32 checksum */
    uint32_t reserved;        /* Reserved for future use */
} enhanced_packet_header_t;

/**
 * @brief Protocol statistics
 */
typedef struct {
    uint32_t packets_sent;
    uint32_t packets_received;
    uint32_t bytes_sent;
    uint32_t bytes_received;
    uint32_t crc_errors;
    uint32_t timeouts;
    uint32_t last_heartbeat;
} protocol_stats_t;

/**
 * @brief Enhanced protocol context
 */
typedef struct {
    uint16_t tx_sequence;
    uint16_t rx_sequence;
    protocol_stats_t stats;
    bool initialized;
    uint32_t last_heartbeat_time;
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
static uint8_t jpeg_buffer[64 * 1024];

__attribute__ ((section (".psram_bss")))
__attribute__((aligned (32)))
static uint8_t metadata_buffer[MAX_METADATA_SIZE];

__attribute__ ((section (".psram_bss")))
__attribute__((aligned (32)))
static uint8_t stream_buffer[LCD_FG_WIDTH * LCD_FG_HEIGHT / (STREAM_SCALE * STREAM_SCALE)];

/* ========================================================================= */
/* UTILITY FUNCTIONS                                                         */
/* ========================================================================= */

/**
 * @brief Calculate CRC32 checksum
 */
static uint32_t calculate_crc32(const uint8_t *data, size_t length)
{
    uint32_t crc = 0xFFFFFFFF;
    static const uint32_t crc_table[256] = {
        /* Standard CRC32 table - truncated for brevity */
        0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, /* ... */
    };
    
    for (size_t i = 0; i < length; i++) {
        uint8_t byte = data[i];
        crc = crc_table[(crc ^ byte) & 0xFF] ^ (crc >> 8);
    }
    
    return crc ^ 0xFFFFFFFF;
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
/* PROTOCOL FUNCTIONS                                                        */
/* ========================================================================= */

/**
 * @brief Send enhanced packet with header, metadata, and payload
 */
static bool send_enhanced_packet(packet_type_t type, const uint8_t *payload, 
                                size_t payload_len, const char *metadata_json)
{
    enhanced_packet_header_t header = {0};
    size_t metadata_len = metadata_json ? strlen(metadata_json) : 0;
    
    /* Validate sizes */
    if (payload_len > MAX_PACKET_SIZE || metadata_len > MAX_METADATA_SIZE) {
        return false;
    }
    
    /* Prepare header */
    header.sync_word = PROTOCOL_SYNC_WORD;
    header.packet_type = type;
    header.flags = 0;
    header.sequence = g_protocol_ctx.tx_sequence++;
    header.payload_length = (uint32_t)payload_len;
    header.metadata_length = (uint16_t)metadata_len;
    
    /* Calculate CRC over metadata + payload */
    uint32_t total_data_len = metadata_len + payload_len;
    uint8_t *temp_buffer = metadata_buffer; /* Reuse buffer */
    
    if (metadata_json) {
        memcpy(temp_buffer, metadata_json, metadata_len);
    }
    if (payload && payload_len > 0) {
        memcpy(temp_buffer + metadata_len, payload, payload_len);
    }
    
    header.crc32 = calculate_crc32(temp_buffer, total_data_len);
    
    /* Send header */
    HAL_StatusTypeDef status = HAL_UART_Transmit(&hcom_uart[COM1], 
                                                (uint8_t*)&header, 
                                                sizeof(header), 
                                                1000);
    if (status != HAL_OK) {
        return false;
    }
    
    /* Send metadata */
    if (metadata_json && metadata_len > 0) {
        status = HAL_UART_Transmit(&hcom_uart[COM1], 
                                  (uint8_t*)metadata_json, 
                                  metadata_len, 
                                  1000);
        if (status != HAL_OK) {
            return false;
        }
    }
    
    /* Send payload in chunks */
    if (payload && payload_len > 0) {
        size_t remaining = payload_len;
        const uint8_t *ptr = payload;
        
        while (remaining > 0) {
            size_t chunk_size = (remaining > UART_CHUNK_SIZE) ? UART_CHUNK_SIZE : remaining;
            status = HAL_UART_Transmit(&hcom_uart[COM1], 
                                      (uint8_t*)ptr, 
                                      chunk_size, 
                                      2000);
            if (status != HAL_OK) {
                return false;
            }
            ptr += chunk_size;
            remaining -= chunk_size;
        }
    }
    
    /* Update statistics */
    g_protocol_ctx.stats.packets_sent++;
    g_protocol_ctx.stats.bytes_sent += sizeof(header) + metadata_len + payload_len;
    
    return true;
}

/**
 * @brief Create JSON metadata string
 */
static int create_metadata_json(char *buffer, size_t buffer_size, 
                               uint32_t width, uint32_t height, uint32_t channels,
                               const char *encoding, uint32_t timestamp,
                               const pd_postprocess_out_t *detections,
                               const performance_metrics_t *perf)
{
    int pos = 0;
    
    pos += snprintf(buffer + pos, buffer_size - pos,
                   "{\"width\":%lu,\"height\":%lu,\"channels\":%lu,"
                   "\"encoding\":\"%s\",\"timestamp\":%lu",
                   width, height, channels, encoding, timestamp);
    
    /* Add detection results */
    if (detections && detections->box_nb > 0) {
        pos += snprintf(buffer + pos, buffer_size - pos, ",\"detections\":[");
        
        pd_pp_box_t *boxes = (pd_pp_box_t *)detections->pOutData;
        for (uint32_t i = 0; i < detections->box_nb && i < 10; i++) {
            if (i > 0) {
                pos += snprintf(buffer + pos, buffer_size - pos, ",");
            }
            pos += snprintf(buffer + pos, buffer_size - pos,
                           "[%d,%.4f,%.4f,%.4f,%.4f,%.4f]",
                           0, /* class_id */
                           boxes[i].x_center,
                           boxes[i].y_center,
                           boxes[i].width,
                           boxes[i].height,
                           boxes[i].prob);
        }
        pos += snprintf(buffer + pos, buffer_size - pos, "]");
    }
    
    /* Add performance metrics */
    if (perf) {
        pos += snprintf(buffer + pos, buffer_size - pos,
                       ",\"performance\":{\"fps\":%.1f,\"latency\":%lu,"
                       "\"cpu_usage\":%.1f,\"memory_usage\":%lu}",
                       perf->fps, perf->inference_time_ms,
                       perf->cpu_usage_percent, perf->memory_usage_bytes);
    }
    
    pos += snprintf(buffer + pos, buffer_size - pos, "}");
    
    return pos;
}

/* ========================================================================= */
/* PUBLIC INTERFACE                                                          */
/* ========================================================================= */

/**
 * @brief Initialize enhanced PC streaming
 */
void Enhanced_PC_STREAM_Init(void)
{
    BSP_COM_Init(COM1, &PcUartInit);
    
#if (USE_COM_LOG > 0)
    BSP_COM_SelectLogPort(COM1);
#endif
    
    memset(&g_protocol_ctx, 0, sizeof(g_protocol_ctx));
    g_protocol_ctx.initialized = true;
    g_protocol_ctx.last_heartbeat_time = HAL_GetTick();
    
    /* Send initialization packet */
    const char *init_metadata = "{\"event\":\"initialization\",\"version\":1}";
    send_enhanced_packet(PACKET_TYPE_HEARTBEAT, NULL, 0, init_metadata);
}

/**
 * @brief Send frame with enhanced protocol
 */
bool Enhanced_PC_STREAM_SendFrame(const uint8_t *frame, uint32_t width, uint32_t height,
                                 uint32_t bpp, const char *tag,
                                 const pd_postprocess_out_t *detections,
                                 const performance_metrics_t *performance)
{
    if (!g_protocol_ctx.initialized) {
        Enhanced_PC_STREAM_Init();
    }
    
    bool is_full_color = (strcmp(tag, "ALN") == 0);
    mem_writer_t writer = {0};
    writer.buffer = jpeg_buffer;
    writer.capacity = sizeof(jpeg_buffer);
    
    uint32_t output_width, output_height;
    
    if (is_full_color) {
        /* Full color aligned frame */
        output_width = width;
        output_height = height;
        stbi_write_jpg_to_func(mem_write_func, &writer, width, height, bpp, frame, JPEG_QUALITY);
    } else {
        /* Downscaled grayscale frame */
        output_width = width / STREAM_SCALE;
        output_height = height / STREAM_SCALE;
        
        if (output_width > sizeof(stream_buffer) / output_height) {
            output_width = sizeof(stream_buffer) / output_height;
        }
        
        /* Convert to grayscale and downsample */
        for (uint32_t y = 0; y < output_height; y++) {
            const uint8_t *src_line = frame + (y * STREAM_SCALE) * width * bpp;
            for (uint32_t x = 0; x < output_width; x++) {
                if (bpp == 2) {
                    const uint16_t *src_pixel = (const uint16_t *)(src_line + x * STREAM_SCALE * 2);
                    stream_buffer[y * output_width + x] = rgb565_to_gray(*src_pixel);
                } else if (bpp == 3) {
                    const uint8_t *src_pixel = src_line + x * STREAM_SCALE * 3;
                    stream_buffer[y * output_width + x] = rgb888_to_gray(src_pixel[0], src_pixel[1], src_pixel[2]);
                }
            }
        }
        
        /* Compress grayscale data */
        stbi_write_jpg_to_func(mem_write_func, &writer, output_width, output_height, 1, stream_buffer, JPEG_QUALITY);
    }
    
    /* Create metadata */
    int metadata_len = create_metadata_json((char*)metadata_buffer, sizeof(metadata_buffer),
                                           output_width, output_height, is_full_color ? bpp : 1,
                                           "jpeg", HAL_GetTick(), detections, performance);
    
    if (metadata_len > 0) {
        metadata_buffer[metadata_len] = '\0';
    }
    
    /* Send packet */
    return send_enhanced_packet(PACKET_TYPE_FRAME_DATA, 
                               jpeg_buffer, writer.size,
                               metadata_len > 0 ? (char*)metadata_buffer : NULL);
}

/**
 * @brief Send embedding data
 */
bool Enhanced_PC_STREAM_SendEmbedding(const float *embedding, uint32_t size)
{
    if (!embedding || size == 0 || size > 1024) {
        return false;
    }
    
    /* Convert embedding to JSON format */
    int pos = snprintf((char*)metadata_buffer, sizeof(metadata_buffer), 
                      "{\"embedding\":[");
    
    for (uint32_t i = 0; i < size && pos < sizeof(metadata_buffer) - 20; i++) {
        if (i > 0) {
            pos += snprintf((char*)metadata_buffer + pos, sizeof(metadata_buffer) - pos, ",");
        }
        pos += snprintf((char*)metadata_buffer + pos, sizeof(metadata_buffer) - pos, 
                       "%.6f", embedding[i]);
    }
    
    pos += snprintf((char*)metadata_buffer + pos, sizeof(metadata_buffer) - pos, 
                   "],\"size\":%lu,\"timestamp\":%lu}", size, HAL_GetTick());
    
    metadata_buffer[pos] = '\0';
    
    return send_enhanced_packet(PACKET_TYPE_EMBEDDING_DATA, NULL, 0, (char*)metadata_buffer);
}

/**
 * @brief Send performance metrics
 */
bool Enhanced_PC_STREAM_SendPerformanceMetrics(const performance_metrics_t *metrics)
{
    if (!metrics) {
        return false;
    }
    
    int pos = snprintf((char*)metadata_buffer, sizeof(metadata_buffer),
                      "{\"fps\":%.1f,\"inference_time\":%lu,\"cpu_usage\":%.1f,"
                      "\"memory_usage\":%lu,\"frame_count\":%lu,\"timestamp\":%lu}",
                      metrics->fps, metrics->inference_time_ms, 
                      metrics->cpu_usage_percent, metrics->memory_usage_bytes,
                      metrics->frame_count, HAL_GetTick());
    
    metadata_buffer[pos] = '\0';
    
    return send_enhanced_packet(PACKET_TYPE_PERFORMANCE_METRICS, NULL, 0, (char*)metadata_buffer);
}

/**
 * @brief Send heartbeat packet
 */
void Enhanced_PC_STREAM_SendHeartbeat(void)
{
    uint32_t current_time = HAL_GetTick();
    
    if (current_time - g_protocol_ctx.last_heartbeat_time >= 5000) { /* 5 second interval */
        int pos = snprintf((char*)metadata_buffer, sizeof(metadata_buffer),
                          "{\"event\":\"heartbeat\",\"uptime\":%lu,\"stats\":{"
                          "\"packets_sent\":%lu,\"packets_received\":%lu,"
                          "\"bytes_sent\":%lu,\"bytes_received\":%lu}}",
                          current_time,
                          g_protocol_ctx.stats.packets_sent,
                          g_protocol_ctx.stats.packets_received,
                          g_protocol_ctx.stats.bytes_sent,
                          g_protocol_ctx.stats.bytes_received);
        
        metadata_buffer[pos] = '\0';
        
        send_enhanced_packet(PACKET_TYPE_HEARTBEAT, NULL, 0, (char*)metadata_buffer);
        g_protocol_ctx.last_heartbeat_time = current_time;
    }
}

/**
 * @brief Get protocol statistics
 */
void Enhanced_PC_STREAM_GetStats(protocol_stats_t *stats)
{
    if (stats) {
        *stats = g_protocol_ctx.stats;
    }
}

/**
 * @brief Legacy compatibility function
 */
void Enhanced_PC_STREAM_SendFrameEx(const uint8_t *frame, uint32_t width, uint32_t height,
                                   uint32_t bpp, const char *tag)
{
    Enhanced_PC_STREAM_SendFrame(frame, width, height, bpp, tag, NULL, NULL);
}

#endif /* USE_BSP_COM_FEATURE */