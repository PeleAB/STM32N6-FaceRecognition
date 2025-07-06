/**
 ******************************************************************************
 * @file    enhanced_pc_stream.h
 * @brief   Enhanced PC streaming with robust protocol and modern features
 ******************************************************************************
 */

#ifndef ENHANCED_PC_STREAM_H
#define ENHANCED_PC_STREAM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "app_postprocess.h"

/* ========================================================================= */
/* TYPE DEFINITIONS                                                          */
/* ========================================================================= */

/**
 * @brief Performance metrics structure
 */
typedef struct {
    float fps;                      /* Current frames per second */
    uint32_t inference_time_ms;     /* Last inference time in milliseconds */
    float cpu_usage_percent;        /* CPU usage percentage */
    uint32_t memory_usage_bytes;    /* Current memory usage */
    uint32_t frame_count;           /* Total processed frames */
    uint32_t detection_count;       /* Total detections */
    uint32_t recognition_count;     /* Total recognitions */
} performance_metrics_t;

/**
 * @brief Protocol statistics structure
 */
typedef struct {
    uint32_t packets_sent;          /* Total packets transmitted */
    uint32_t packets_received;      /* Total packets received */
    uint32_t bytes_sent;           /* Total bytes transmitted */
    uint32_t bytes_received;       /* Total bytes received */
    uint32_t crc_errors;           /* CRC error count */
    uint32_t timeouts;             /* Timeout error count */
    uint32_t last_heartbeat;       /* Last heartbeat timestamp */
} protocol_stats_t;

/* ========================================================================= */
/* FUNCTION PROTOTYPES                                                       */
/* ========================================================================= */

/**
 * @brief Initialize enhanced PC streaming protocol
 */
void Enhanced_PC_STREAM_Init(void);

/**
 * @brief Send frame with enhanced protocol including metadata
 * @param frame Pointer to frame data
 * @param width Frame width in pixels
 * @param height Frame height in pixels
 * @param bpp Bytes per pixel (2 for RGB565, 3 for RGB888)
 * @param tag Frame type tag ("JPG" or "ALN")
 * @param detections Optional detection results
 * @param performance Optional performance metrics
 * @return true if successful, false otherwise
 */
bool Enhanced_PC_STREAM_SendFrame(const uint8_t *frame, uint32_t width, uint32_t height,
                                 uint32_t bpp, const char *tag,
                                 const pd_postprocess_out_t *detections,
                                 const performance_metrics_t *performance);

/**
 * @brief Send embedding data with metadata
 * @param embedding Pointer to embedding array
 * @param size Number of elements in embedding
 * @return true if successful, false otherwise
 */
bool Enhanced_PC_STREAM_SendEmbedding(const float *embedding, uint32_t size);

/**
 * @brief Send detection results with robust protocol
 * @param frame_id Frame ID for correlation
 * @param detections Detection results
 * @return true if successful, false otherwise
 */
bool Enhanced_PC_STREAM_SendDetections(uint32_t frame_id, const pd_postprocess_out_t *detections);

/**
 * @brief Send performance metrics
 * @param metrics Pointer to performance metrics structure
 * @return true if successful, false otherwise
 */
bool Enhanced_PC_STREAM_SendPerformanceMetrics(const performance_metrics_t *metrics);

/**
 * @brief Send periodic heartbeat packet
 */
void Enhanced_PC_STREAM_SendHeartbeat(void);

/**
 * @brief Get protocol statistics
 * @param stats Pointer to statistics structure to fill
 */
void Enhanced_PC_STREAM_GetStats(protocol_stats_t *stats);

/**
 * @brief Legacy compatibility function for existing code
 * @param frame Pointer to frame data
 * @param width Frame width in pixels
 * @param height Frame height in pixels
 * @param bpp Bytes per pixel
 * @param tag Frame type tag
 */
void Enhanced_PC_STREAM_SendFrameEx(const uint8_t *frame, uint32_t width, uint32_t height,
                                   uint32_t bpp, const char *tag);

/* ========================================================================= */
/* LEGACY COMPATIBILITY MACROS                                              */
/* ========================================================================= */

/* Map old function names to new enhanced versions for backward compatibility */
#define PC_STREAM_Init()                    Enhanced_PC_STREAM_Init()
#define PC_STREAM_SendFrameEx(f,w,h,b,t)   Enhanced_PC_STREAM_SendFrameEx(f,w,h,b,t)

#ifdef __cplusplus
}
#endif

#endif /* ENHANCED_PC_STREAM_H */