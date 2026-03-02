/**
 ******************************************************************************
 * @file    uvcl_internal.h
 * @author  MDG Application Team
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

#ifndef UVCL_INTERNAL_H
#define UVCL_INTERNAL_H

#include <stdint.h>

#include "cmsis_compiler.h"
#include "uvcl.h"

#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

#ifndef USBL_PACKET_PER_MICRO_FRAME
#define USBL_PACKET_PER_MICRO_FRAME 1
#endif

#define USBX_BUFFER_NB                                  4
#define USBX_MEM_SIZE                                   (32 * 1024)
#define UVC_MAX_CONF_LEN                                512
#define UVC_MAX_STRING_LEN                              512
#define UVC_MAX_LANGID_LEN                              2

#define UVC_ISO_FS_MPS                                  1023
#define UVC_ISO_HS_MPS                                  (USBL_PACKET_PER_MICRO_FRAME * 1024)

#define USB_REQ_TYPE_STANDARD                          0x00U
#define USB_REQ_TYPE_CLASS                             0x20U
#define USB_REQ_TYPE_VENDOR                            0x40U
#define USB_REQ_TYPE_MASK                              0x60U

#define USB_REQ_RECIPIENT_DEVICE                       0x00U
#define USB_REQ_RECIPIENT_INTERFACE                    0x01U
#define USB_REQ_RECIPIENT_ENDPOINT                     0x02U
#define USB_REQ_RECIPIENT_MASK                         0x03U

/* Video Class-Specific Request values */
#define UVC_RQ_UNDEFINED                               0x00U
#define UVC_SET_CUR                                    0x01U
#define UVC_GET_CUR                                    0x81U
#define UVC_GET_MIN                                    0x82U
#define UVC_GET_MAX                                    0x83U
#define UVC_GET_RES                                    0x84U
#define UVC_GET_LEN                                    0x85U
#define UVC_GET_INFO                                   0x86U
#define UVC_GET_DEF                                    0x87U

/* VideoStreaming Interface Control Selectors */
#define VS_CONTROL_UNDEFINED                           0x00U
#define VS_PROBE_CONTROL                               0x100U
#define VS_COMMIT_CONTROL                              0x200U
#define VS_PROBE_CONTROL_CS                            0x01U
#define VS_COMMIT_CONTROL_CS                           0x02U
#define VS_STILL_PROBE_CONTROL                         0x03U
#define VS_STILL_COMMIT_CONTROL                        0x04U
#define VS_STILL_IMAGE_TRIGGER_CONTROL                 0x05U
#define VS_STREAM_ERROR_CODE_CONTROL                   0x06U
#define VS_GENERATE_KEY_FRAME_CONTROL                  0x07U
#define VS_UPDATE_FRAME_SEGMENT_CONTROL                0x08U
#define VS_SYNC_DELAY_CONTROL                          0x09U

#define UVC_INTERVAL(n)                               (10000000U/(n))

#define UVCL_ALIGN_32 __attribute__ ((aligned (32)))
#define UVCL_UNCACHED __attribute__ ((section (".uncached_bss")))

typedef struct
{
  uint16_t bmHint;
  uint8_t bFormatIndex;
  uint8_t bFrameIndex;
  uint32_t dwFrameInterval;
  uint16_t wKeyFrameRate;
  uint16_t wPFrameRate;
  uint16_t wCompQuality;
  uint16_t wCompWindowSize;
  uint16_t wDelay;
  uint32_t dwMaxVideoFrameSize;
  uint32_t dwMaxPayloadTransferSize;
  uint32_t dwClockFrequency;
  uint8_t bmFramingInfo;
  uint8_t bPreferedVersion;
  uint8_t bMinVersion;
  uint8_t bMaxVersion;
  uint8_t bUsage;
  uint8_t bBitDepthLuma;
  uint8_t bmSettings;
  uint8_t bMaxNumberOfRefFramesPlus1;
  uint16_t bmRateControlModes;
  uint64_t bmLayoutPerStream;
} __PACKED UVC_VideoControlTypeDef;

typedef enum {
  UVCL_STATUS_STOP,
  UVCL_STATUS_STREAMING,
} UVCL_State_t;

typedef struct {
  /* usb request packet */
  uint8_t bmRequestType;
  uint8_t bRequest;
  uint16_t wValue;
  uint16_t wIndex;
  uint16_t wLength;
  /* info use by common code */
  int dwMaxPayloadTransferSize;
  /* ctx argument for cb functions */
  void *ctx;
  /* impl must implement following functions */
  int (*stop_streaming)(void *);
  int (*start_streaming)(void *);
  int (*send_data)(void *, uint8_t *, int);
  int (*receive_data)(void *, uint8_t *, int);
} UVCL_SetupReq_t;

typedef struct {
  int frame_index;
  uint8_t *cursor;
  int packet_nb;
  int packet_index;
  int last_packet_size;
  int prev_len;
  uint8_t *p_frame;
} UVCL_OnFlyCtx_t;

typedef struct {
  uint8_t bFormatIndex;
  uint8_t bFrameIndex;
  uint32_t dwFrameInterval;
} UVCL_StreamConfPrivate;

typedef struct {
  UVCL_Conf_t conf;
  UVCL_Callbacks_t *cbs;
  int buffer_nb;
  UVCL_State_t state;
  uint8_t *packet;
  uint32_t frame_start;
  int frame_period_in_ms;
  int is_starting;
  uint8_t *p_frame;
  int frame_size;
  UVCL_OnFlyCtx_t on_fly_storage_ctx;
  UVCL_OnFlyCtx_t *on_fly_ctx;
  UVC_VideoControlTypeDef UVC_VideoCommitControl;
  UVC_VideoControlTypeDef UVC_VideoProbeControl;
  UVCL_StreamConfPrivate streams_p[UVCL_MAX_STREAM_CONF_NB];
  uint32_t desc_buffer_pool[UVCL_MAX_STREAM_CONF_NB * 256];
} UVCL_Ctx_t;

UVCL_OnFlyCtx_t *UVCL_StartNewFrameTransmission(UVCL_Ctx_t *p_ctx, int packet_size);
void UVCL_UpdateOnFlyCtx(UVCL_Ctx_t *p_ctx, int len);
void UVCL_AbortOnFlyCtx(UVCL_Ctx_t *p_ctx);
int UVCL_handle_setup_request(UVCL_Ctx_t *p_ctx, UVCL_SetupReq_t *req);
uint32_t UVCL_ComputedwMaxVideoFrameSize(UVCL_Ctx_t *ctx, int format_idx, int frame_idx);
void UVCL_SetupStreamingStream(UVCL_Ctx_t *ctx, UVCL_StreamConf_t *stream);

#endif
