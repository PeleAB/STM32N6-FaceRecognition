/**
 ******************************************************************************
 * @file    uvcl_desc.h
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

#ifndef _UVCL_DESC_H_
#define _UVCL_DESC_H_

#include "uvcl.h"

typedef struct {
  int is_hs;
  UVCL_StreamConf_t streams[UVCL_MAX_STREAM_CONF_NB];
  int streams_nb;
} UVCL_DescConf;

typedef struct {
  /* Must be aligned on 32 bits */
  void *buffer;
  int buffer_size;
} UVCL_DescBuffer;

int UVCL_get_device_desc(void *p_dst, int dst_len, int idx_manufacturer, int idx_product, int idx_serial);
int UVCL_get_device_qualifier_desc(void *p_dst, int dst_len);
int UVCL_get_lang_string_desc(void *p_dst, int dst_len);
int UVCL_get_manufacturer_string_desc(void *p_dst, int dst_len);
int UVCL_get_product_string_desc(void *p_dst, int dst_len);
int UVCL_get_serial_string_desc(void *p_dst, int dst_len);
int UVCL_get_configuration_desc(void *p_dst, int dst_len, UVCL_DescConf *p_conf, UVCL_DescBuffer *p_buffer);

#endif

