/**
 ******************************************************************************
 * @file    app_constants.h
 * @author  PeleAB
 * @brief   Application constants and configuration parameters
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2023 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

#ifndef APP_CONSTANTS_H
#define APP_CONSTANTS_H

/* ========================================================================= */
/* TIMING CONSTANTS                                                          */
/* ========================================================================= */
/** @brief Interval between face re-verification attempts (milliseconds) */
#define FACE_REVERIFY_INTERVAL_MS           1000

/** @brief Duration for button long press detection (milliseconds) */
#define BUTTON_LONG_PRESS_DURATION_MS       1000

/** @brief Additional timeout for unverified face LED indication (milliseconds) */
#define FACE_UNVERIFIED_LED_TIMEOUT_MS      1000

/** @brief UART communication timeout (milliseconds) */
#define UART_COMMUNICATION_TIMEOUT_MS       1000

/* ========================================================================= */
/* NEURAL NETWORK CONSTANTS                                                  */
/* ========================================================================= */
/** @brief Maximum number of neural network output buffers */
#define NN_MAX_OUTPUT_BUFFERS               5

/** @brief Face recognition input image width */
#define FACE_RECOGNITION_WIDTH              112

/** @brief Face recognition input image height */
#define FACE_RECOGNITION_HEIGHT             112

/** @brief Face similarity threshold for verification */
#define FACE_SIMILARITY_THRESHOLD           0.55f

/** @brief Face detection confidence threshold */
#define FACE_DETECTION_CONFIDENCE_THRESHOLD 0.7f

/** @brief Face bounding box padding factor */
#define FACE_BBOX_PADDING_FACTOR            1.2f

/** @brief Face embedding quantization scale */
#define FACE_EMBEDDING_QUANTIZATION_SCALE   128.0f

/* ========================================================================= */
/* TRACKING CONSTANTS                                                        */
/* ========================================================================= */
/** @brief Kalman filter smoothing factor for tracking */
#define TRACKER_SMOOTH_FACTOR               0.5f

/** @brief IoU threshold for track association */
#define TRACKER_IOU_THRESHOLD               0.3f

/** @brief Maximum number of frames a track can be lost */
#define TRACKER_MAX_LOST_FRAMES             5

/** @brief Minimum confidence for track initialization */
#define TRACKER_MIN_INIT_CONFIDENCE         0.6f

/** @brief Track association distance threshold */
#define TRACKER_ASSOCIATION_THRESHOLD       0.5f

/* ========================================================================= */
/* PROTOCOL CONSTANTS                                                        */
/* ========================================================================= */
/** @brief Maximum payload size for robust protocol */
#define PROTOCOL_MAX_PAYLOAD_SIZE           (64 * 1024)

/** @brief Display stream scale factor */
#define DISPLAY_STREAM_SCALE_FACTOR         2

/** @brief CRC32 polynomial for protocol validation */
#define PROTOCOL_CRC32_POLYNOMIAL           0xEDB88320

/* ========================================================================= */
/* MEMORY ALIGNMENT CONSTANTS                                                */
/* ========================================================================= */
/** @brief Memory alignment requirement for buffers */
#define MEMORY_ALIGNMENT_BYTES              32

/** @brief Cache line alignment requirement */
#define CACHE_LINE_ALIGNMENT                32

/* ========================================================================= */
/* PERFORMANCE CONSTANTS                                                     */
/* ========================================================================= */
/** @brief Target frames per second for display */
#define TARGET_DISPLAY_FPS                  60

/** @brief Target frames per second for camera capture */
#define TARGET_CAMERA_FPS                   30

/** @brief Performance monitoring update interval (frames) */
#define PERFORMANCE_UPDATE_INTERVAL         10

/* ========================================================================= */
/* UTILITY MACROS                                                           */
/* ========================================================================= */
/** @brief Align value to 16-byte boundary */
#define ALIGN_TO_16(value)                  (((value) + 15) & ~15)

/** @brief Align value to 32-byte boundary */
#define ALIGN_TO_32(value)                  (((value) + 31) & ~31)

/** @brief Calculate buffer size with alignment */
#define DCMIPP_OUT_NN_LEN                   (ALIGN_TO_16(NN_WIDTH * NN_BPP) * NN_HEIGHT)
#define DCMIPP_OUT_NN_BUFF_LEN              (DCMIPP_OUT_NN_LEN + MEMORY_ALIGNMENT_BYTES - DCMIPP_OUT_NN_LEN % MEMORY_ALIGNMENT_BYTES)

/** @brief Convert milliseconds to microseconds */
#define MS_TO_US(ms)                        ((ms) * 1000)

/** @brief Convert seconds to milliseconds */
#define SEC_TO_MS(sec)                      ((sec) * 1000)

#endif /* APP_CONSTANTS_H */
