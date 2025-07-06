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
        0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
        0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988, 0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
        0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
        0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
        0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172, 0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
        0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
        0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
        0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924, 0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
        0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
        0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
        0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E, 0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
        0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
        0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
        0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0, 0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
        0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
        0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
        0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A, 0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
        0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
        0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
        0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC, 0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
        0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
        0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
        0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236, 0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
        0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
        0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
        0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38, 0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
        0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
        0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
        0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2, 0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
        0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
        0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
        0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94, 0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
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