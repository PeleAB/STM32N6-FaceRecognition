/**
 ******************************************************************************
 * @file    robust_pc_stream.h
 * @brief   Robust binary protocol for PC streaming with checksums and framing
 ******************************************************************************
 */

#ifndef ROBUST_PC_STREAM_H
#define ROBUST_PC_STREAM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "app_postprocess.h"

/* ========================================================================= */
/* PROTOCOL CONSTANTS                                                        */
/* ========================================================================= */

#define ROBUST_SOF_BYTE                 0xAA
#define ROBUST_HEADER_SIZE              4
#define ROBUST_MAX_PAYLOAD_SIZE         (64 * 1024)
#define ROBUST_MSG_HEADER_SIZE          3

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
 * @brief Protocol frame header (4 bytes)
 */
typedef struct __attribute__((packed)) {
    uint8_t sof;                    /* Start of Frame (0xAA) */
    uint16_t payload_size;          /* Payload size in bytes */
    uint8_t header_checksum;        /* XOR checksum of SOF + payload_size */
} robust_frame_header_t;

/**
 * @brief Message header within payload (3 bytes)
 */
typedef struct __attribute__((packed)) {
    uint8_t message_type;           /* Message type */
    uint16_t sequence_id;           /* Sequence ID for message ordering */
} robust_message_header_t;

/**
 * @brief Frame data payload format
 */
typedef struct __attribute__((packed)) {
    char frame_type[4];             /* "JPG", "ALN", etc. */
    uint32_t width;                 /* Frame width */
    uint32_t height;                /* Frame height */
    /* Image data follows */
} robust_frame_data_t;

/**
 * @brief Detection results payload format
 */
typedef struct __attribute__((packed)) {
    uint32_t frame_id;              /* Frame ID */
    uint32_t detection_count;       /* Number of detections */
    /* Detection data follows */
} robust_detection_data_t;

/**
 * @brief Single detection format
 */
typedef struct __attribute__((packed)) {
    uint32_t class_id;              /* Object class */
    float x, y, w, h;               /* Bounding box (normalized) */
    float confidence;               /* Detection confidence */
    uint32_t keypoint_count;        /* Number of keypoints */
    /* Keypoint data follows (2 floats per keypoint) */
} robust_detection_t;

/**
 * @brief Embedding data payload format
 */
typedef struct __attribute__((packed)) {
    uint32_t embedding_size;        /* Number of float values */
    /* Embedding data follows (float array) */
} robust_embedding_data_t;

/**
 * @brief Performance metrics payload format
 */
typedef struct __attribute__((packed)) {
    float fps;                      /* Current FPS */
    uint32_t inference_time_ms;     /* Inference time in ms */
    float cpu_usage_percent;        /* CPU usage percentage */
    uint32_t memory_usage_bytes;    /* Memory usage in bytes */
    uint32_t frame_count;           /* Total frame count */
    uint32_t detection_count;       /* Total detection count */
    uint32_t recognition_count;     /* Total recognition count */
} robust_performance_metrics_t;

/**
 * @brief Protocol statistics
 */
typedef struct {
    uint32_t messages_sent;         /* Total messages sent */
    uint32_t bytes_sent;           /* Total bytes sent */
    uint32_t send_errors;          /* Send error count */
    uint16_t sequence_counters[16]; /* Sequence counters per message type */
} robust_protocol_stats_t;

/* ========================================================================= */
/* FUNCTION PROTOTYPES                                                       */
/* ========================================================================= */

/**
 * @brief Initialize robust PC streaming protocol
 */
void Robust_PC_STREAM_Init(void);

/**
 * @brief Send frame data with robust protocol
 * @param frame Pointer to frame data
 * @param width Frame width in pixels
 * @param height Frame height in pixels
 * @param frame_type Frame type string ("JPG", "ALN", etc.)
 * @return true if successful, false otherwise
 */
bool Robust_PC_STREAM_SendFrame(const uint8_t *frame, uint32_t frame_size,
                                uint32_t width, uint32_t height, 
                                const char *frame_type);

/**
 * @brief Send detection results with robust protocol
 * @param frame_id Frame ID for correlation
 * @param detections Detection results
 * @return true if successful, false otherwise
 */
bool Robust_PC_STREAM_SendDetections(uint32_t frame_id, 
                                    const pd_postprocess_out_t *detections);

/**
 * @brief Send embedding data with robust protocol
 * @param embedding Pointer to embedding array
 * @param size Number of elements in embedding
 * @return true if successful, false otherwise
 */
bool Robust_PC_STREAM_SendEmbedding(const float *embedding, uint32_t size);

/**
 * @brief Send performance metrics with robust protocol
 * @param metrics Pointer to performance metrics structure
 * @return true if successful, false otherwise
 */
bool Robust_PC_STREAM_SendPerformanceMetrics(const robust_performance_metrics_t *metrics);

/**
 * @brief Send heartbeat message
 */
void Robust_PC_STREAM_SendHeartbeat(void);

/**
 * @brief Send debug information
 * @param debug_msg Debug message string
 */
void Robust_PC_STREAM_SendDebugInfo(const char *debug_msg);

/**
 * @brief Get protocol statistics
 * @param stats Pointer to statistics structure to fill
 */
void Robust_PC_STREAM_GetStats(robust_protocol_stats_t *stats);

/**
 * @brief Clear protocol statistics
 */
void Robust_PC_STREAM_ClearStats(void);

/* ========================================================================= */
/* UTILITY FUNCTIONS                                                         */
/* ========================================================================= */

/**
 * @brief Calculate XOR checksum
 * @param data Pointer to data
 * @param length Data length
 * @return XOR checksum
 */
uint8_t robust_calculate_checksum(const uint8_t *data, uint32_t length);

/**
 * @brief Send raw message with robust framing
 * @param message_type Message type
 * @param payload Payload data
 * @param payload_size Payload size
 * @return true if successful, false otherwise
 */
bool robust_send_message(robust_message_type_t message_type, 
                        const uint8_t *payload, uint32_t payload_size);

#ifdef __cplusplus
}
#endif

#endif /* ROBUST_PC_STREAM_H */