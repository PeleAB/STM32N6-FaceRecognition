/**
 ******************************************************************************
 * @file    uvcl.h
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

#ifndef _UVCL_H_
#define _UVCL_H_

#include <stdint.h>

#include "stm32n6xx_hal.h"

#ifndef UVCL_MAX_STREAM_CONF_NB
#define UVCL_MAX_STREAM_CONF_NB 8
#endif

/* Use UVCL_PAYLOAD_UNCOMPRESSED_YUY2 or UVCL_PAYLOAD_JPEG for maximal compatibility */
#define UVCL_PAYLOAD_UNCOMPRESSED_YUY2 0
#define UVCL_PAYLOAD_JPEG 1
/* Frame based formats */
#define UVCL_PAYLOAD_FB_RGB565 2
#define UVCL_PAYLOAD_FB_BGR3 3
#define UVCL_PAYLOAD_FB_GREY 4
#define UVCL_PAYLOAD_FB_H264 5
#define UVCL_PAYLOAD_FB_JPEG 6
#define UVCL_PAYLOAD_FB_GREY_D3DFMT_L8 7

typedef struct {
  int payload_type;
  int width;
  int height;
  int fps;
  /* Only for variable size payload. If zero a default value is choose */
  uint32_t dwMaxVideoFrameSize;
} UVCL_StreamConf_t;

typedef struct {
  UVCL_StreamConf_t streams[UVCL_MAX_STREAM_CONF_NB];
  int streams_nb;
  int is_immediate_mode;
} UVCL_Conf_t;

typedef struct uvcl_callbacks {
  void (*streaming_active)(struct uvcl_callbacks *cbs, UVCL_StreamConf_t stream);
  void (*streaming_inactive)(struct uvcl_callbacks *cbs);
  void (*frame_release)(struct uvcl_callbacks *cbs, void *frame);
} UVCL_Callbacks_t;

extern PCD_HandleTypeDef uvcl_pcd_handle;

int UVCL_Init(PCD_TypeDef *pcd_instance, UVCL_Conf_t *conf, UVCL_Callbacks_t *cbs);
int UVCL_Deinit(void);
void UVCL_IRQHandler(void);
/* return 0 if frame will be displayed. else it won't be displayed */
int UVCL_ShowFrame(void *frame, int frame_size);

#endif

