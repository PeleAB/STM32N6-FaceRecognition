/**
 ******************************************************************************
 * @file    uvcl_desc.c
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

#include "uvcl_desc.h"

#include <string.h>

#include "uvcl_codes.h"

#include "uvcl_desc_internal.h"
#include "uvcl_internal.h"
#include <assert.h>

#define container_of(ptr, type, member) (type *) ((unsigned char *)ptr - offsetof(type,member))
#define ARRAY_LEN(a) (sizeof(a)/sizeof(a[0]))

#define DEV_MANUFACTURER_STRING      "STMicroelectronics"
#define DEV_PRODUCT_STRING           "STM32 uvc"

struct buffer_allocator {
  void *buffer;
  int buffer_size;
  void *current;
  void *limit;
};

static int ba_init(struct buffer_allocator *ba)
{
  if (!ba->buffer)
    return -1;
  if ((uintptr_t)ba->buffer & 0x3)
    return -1;
  if (ba->buffer <= 0)
    return  -1;

  ba->current = ba->buffer;
  ba->limit = (uint8_t *)ba->buffer + ba->buffer_size;

  return 0;
}

static void *ba_alloc(struct buffer_allocator *ba, size_t size)
{
  void *res = ba->current;

  if (size <= 0)
    return NULL;

  if ((uint8_t *)ba->current + size >= (uint8_t *)ba->limit)
    return NULL;

  size = (size + 3) & ~0x3;
  ba->current = (uint8_t *)ba->buffer + size;

  return res;
}

static int compute_min_fps(int *fps, int fps_nb)
{
  int min_fps = fps[0];
  int i;

  for (i = 1; i <fps_nb; i++)
    min_fps = MIN(min_fps, fps[i]);

  return min_fps;
}

static int compute_max_fps(int *fps, int fps_nb)
{
  int max_fps = fps[0];
  int i;

  for (i = 1; i <fps_nb; i++)
    max_fps = MAX(max_fps, fps[i]);

  return max_fps;
}

static void gen_serial_int_to_ascii(uint32_t value, char *p_buf, uint8_t len)
{
  uint8_t idx = 0U;

  for (idx = 0U ; idx < len ; idx ++)
  {
    if (((value >> 28)) < 0xAU)
      p_buf[idx] = (value >> 28) + '0';
    else
      p_buf[idx] = (value >> 28) + 'A' - 10U;
    value = value << 4;
  }
}

static void gen_serial(char serial[13])
{
  uint32_t deviceserial0;
  uint32_t deviceserial1;
  uint32_t deviceserial2;

  deviceserial0 = HAL_GetUIDw0();
  deviceserial1 = HAL_GetUIDw1();
  deviceserial2 = HAL_GetUIDw2();
  deviceserial0 += deviceserial2;

  gen_serial_int_to_ascii(deviceserial0, &serial[0], 8);
  gen_serial_int_to_ascii(deviceserial1, &serial[8], 4);
  serial[12] = '\0';
}

static int UVCL_get_string_desc(void *p_dst, int dst_len, char *name)
{
  int desc_len = 2 + strlen(name) * 2;
  uint8_t *dst = p_dst;

  if (dst_len < desc_len)
    return -1;

  *dst++ = desc_len; /* bLength = desc_len */
  *dst++ = 0x03; /* string descriptor */

  while (*name) {
    *dst++ = *name++;
    *dst++ = 0;
  }

  return dst_len;
}

static int get_children_len(struct uvc_desc_head *child)
{
  int total_len = 0;

  while (child) {
    total_len += child->bLength + get_children_len(child->children);
    child = child->next_child;
  }

  return total_len;
}

static void append_as_child(struct uvc_desc_head *parent, struct uvc_desc_head *child)
{
  struct uvc_desc_head *prev_child_head = parent->children;

  parent->children = child;
  child->next_child = prev_child_head;
}

static int gen_default_desc(struct uvc_desc_head *head, uint8_t *p_dst, int dst_len)
{
  if (head->bLength > dst_len)
    return -1;

  memcpy(p_dst, head->raw, head->bLength);
  if (head->next) {
    assert(head->next->gen);
    return head->next->gen(head->next, p_dst + head->bLength, dst_len - head->bLength);
  } else {
    return dst_len - head->bLength;
  }
}

static void update(struct uvc_desc_head *head)
{
  while (head) {
    if (head->update)
      head->update(head);
    head = head->next;
  }
}

static int generate(struct uvc_desc_head *head, uint8_t *p_dst, int dst_len)
{
  int ret;

  ret = head->gen(head, p_dst, dst_len);

  return ret < 0 ? ret : dst_len - ret;
}

static void build_dev_desc(struct uvc_dev_desc *desc, struct uvc_desc_head *next, UVCL_DescConf *p_conf,
                           int idx_manufacturer, int idx_product, int idx_serial)
{
  desc->head.bLength = sizeof(desc->raw);
  desc->head.raw = (uint8_t *) &desc->raw;
  desc->head.gen = gen_default_desc;
  desc->head.next = next;
  desc->raw.bLength = sizeof(desc->raw);
  desc->raw.bDescriptorType = 0x01;
  desc->raw.bcdUSB = 0x0200;
  desc->raw.bDeviceClass = 0xEF;
  desc->raw.bDeviceSubClass = 0x02;
  desc->raw.bDeviceProtocol = 0x01;
  desc->raw.bMaxPacketSize = 64;
  desc->raw.idVendor = 0x0483;
  desc->raw.idProduct = 0x5780;
  desc->raw.bcdDevice = 0x0100;
  desc->raw.iManufacturer = idx_manufacturer;
  desc->raw.iProduct = idx_product;
  desc->raw.iSerialNumber = idx_serial;
  desc->raw.bNumConfigurations = 1;
}

static void update_uvc_conf_desc(struct uvc_desc_head *head)
{
  struct uvc_conf_desc *desc = container_of(head, struct uvc_conf_desc, head);

  desc->raw.wTotalLength += get_children_len(desc->head.children);
}

static void build_uvc_conf_desc(struct uvc_conf_desc *desc, struct uvc_desc_head *next, UVCL_DescConf *p_conf)
{
  desc->head.bLength = sizeof(desc->raw);
  desc->head.raw = (uint8_t *) &desc->raw;
  desc->head.gen = gen_default_desc;
  desc->head.update = update_uvc_conf_desc;
  desc->head.next = next;
  desc->raw.bLength = sizeof(desc->raw);
  desc->raw.bDescriptorType = 0x02;
  desc->raw.wTotalLength = desc->raw.bLength;
  desc->raw.bNumInterfaces = 2;
  desc->raw.bConfigurationValue = 1;
  desc->raw.iConfiguration = 0;
  desc->raw.bmAttributes = 0xc0; /* self powered */
  desc->raw.bMaxPower = 50; /* 100 ma */
}

static void build_uvc_iad_desc(struct uvc_iad_desc *desc, struct uvc_desc_head *next, struct uvc_desc_head *parent,
                               UVCL_DescConf *p_conf)
{
  desc->head.bLength = sizeof(desc->raw);
  desc->head.raw = (uint8_t *) &desc->raw;
  desc->head.gen = gen_default_desc;
  desc->head.next = next;
  desc->raw.bLength = sizeof(desc->raw);
  desc->raw.bDescriptorType = 0x0b;
  desc->raw.bFirstInterface = 0;
  desc->raw.bInterfaceCount = 2;
  desc->raw.bFunctionClass = CC_VIDEO;
  desc->raw.bFunctionSubClass = SC_VIDEO_INTERFACE_COLLECTION;
  desc->raw.bFunctionProtocol = PC_PROTOCOL_UNDEFINED;
  desc->raw.iFunction = 0;

  append_as_child(parent, &desc->head);
}

static void build_uvc_std_vc_desc(struct uvc_std_vc_desc *desc, struct uvc_desc_head *next, struct uvc_desc_head *parent,
                                  UVCL_DescConf *p_conf)
{
  desc->head.bLength = sizeof(desc->raw);
  desc->head.raw = (uint8_t *) &desc->raw;
  desc->head.gen = gen_default_desc;
  desc->head.next = next;
  desc->raw.bLength = sizeof(desc->raw);
  desc->raw.bDescriptorType = 0x04;
  desc->raw.bInterfaceNumber = 0;
  desc->raw.bAlternateSetting = 0;
  desc->raw.bNumEndpoints = 0;
  desc->raw.bInterfaceClass = CC_VIDEO;
  desc->raw.bInterfaceSubClass = SC_VIDEOCONTROL;
  desc->raw.bInterfaceProtocol = PC_PROTOCOL_UNDEFINED;
  desc->raw.iInterface = 0;

  append_as_child(parent, &desc->head);
}

static void update_uvc_class_vc_desc(struct uvc_desc_head *head)
{
  struct uvc_class_vc_desc *desc = container_of(head, struct uvc_class_vc_desc, head);

  desc->raw.wTotalLength += get_children_len(desc->head.children);
}

static void build_uvc_class_vc_desc(struct uvc_class_vc_desc *desc, struct uvc_desc_head *next,
                                    struct uvc_desc_head *parent, UVCL_DescConf *p_conf)
{
  const int vs_nb = 1;

  desc->head.bLength = sizeof(desc->raw);
  desc->head.raw = (uint8_t *) &desc->raw;
  desc->head.gen = gen_default_desc;
  desc->head.update = update_uvc_class_vc_desc;
  desc->head.next = next;
  desc->raw.bLength = sizeof(desc->raw);
  desc->raw.bDescriptorType = CS_INTERFACE;
  desc->raw.bDescriptorSubType = VC_HEADER;
  desc->raw.bcdUVC = 0x0150;
  desc->raw.wTotalLength = 12 + vs_nb;;
  desc->raw.dwClockFrequency =48000000;
  desc->raw.bInCollection = vs_nb;
  for (int i = 0; i < vs_nb; i++)
    desc->raw.baInterfaceNr[i] = i + 1;

  append_as_child(parent, &desc->head);
}

static void build_uvc_camera_terminal_desc(struct uvc_camera_terminal_desc *desc, struct uvc_desc_head *next,
                                           struct uvc_desc_head *parent, UVCL_DescConf *p_conf)
{
  desc->head.bLength = sizeof(desc->raw);
  desc->head.raw = (uint8_t *) &desc->raw;
  desc->head.gen = gen_default_desc;
  desc->head.next = next;
  desc->raw.bLength = sizeof(desc->raw);
  desc->raw.bDescriptorType = CS_INTERFACE;
  desc->raw.bDescriptorSubType = VC_INPUT_TERMINAL;
  desc->raw.bTerminalID = 1;
  desc->raw.wTerminalType = ITT_CAMERA;
  desc->raw.bAssocTerminal = 0;
  desc->raw.iTerminal = 0;
  desc->raw.wObjectiveFocalLengthMin = 0;
  desc->raw.wObjectiveFocalLengthMax = 0;
  desc->raw.wOcularFocalLength = 0;
  desc->raw.bControlSize = 3;
  for (int i = 0; i < 3; i++)
    desc->raw.bmControls[i] = 0;

  append_as_child(parent, &desc->head);
}

static void build_uvc_output_term_desc(struct uvc_output_term_desc *desc, struct uvc_desc_head *next,
                                       struct uvc_desc_head *parent, UVCL_DescConf *p_conf)
{
  desc->head.bLength = sizeof(desc->raw);
  desc->head.raw = (uint8_t *) &desc->raw;
  desc->head.gen = gen_default_desc;
  desc->head.next = next;
  desc->raw.bLength = sizeof(desc->raw);
  desc->raw.bDescriptorType = CS_INTERFACE;
  desc->raw.bDescriptorSubType = VC_OUTPUT_TERMINAL;
  desc->raw.bTerminalID = 2;
  desc->raw.wTerminalType = TT_STREAMING;
  desc->raw.bAssocTerminal = 0;
  desc->raw.bSourceID = 1;
  desc->raw.iTerminal = 0;

  append_as_child(parent, &desc->head);
}

static void build_uvc_std_vs_desc(struct uvc_std_vs_desc *desc, struct uvc_desc_head *next, struct uvc_desc_head *parent,
                                  UVCL_DescConf *p_conf, int alt_nb, int ep_nb)
{
  desc->head.bLength = sizeof(desc->raw);
  desc->head.raw = (uint8_t *) &desc->raw;
  desc->head.gen = gen_default_desc;
  desc->head.next = next;
  desc->raw.bLength = sizeof(desc->raw);
  desc->raw.bDescriptorType = 0x04;
  desc->raw.bInterfaceNumber = 1;
  desc->raw.bAlternateSetting = alt_nb;
  desc->raw.bNumEndpoints = ep_nb;
  desc->raw.bInterfaceClass = CC_VIDEO;
  desc->raw.bInterfaceSubClass = SC_VIDEOSTREAMING;
  desc->raw.bInterfaceProtocol = PC_PROTOCOL_UNDEFINED;
  desc->raw.iInterface = 0;

  append_as_child(parent, &desc->head);
}

static void update_uvc_vs_input_desc(struct uvc_desc_head *head)
{
  struct uvc_vs_input_desc *desc = container_of(head, struct uvc_vs_input_desc, head);

  desc->raw.wTotalLength += get_children_len(desc->head.children);
}

static void build_uvc_vs_input_desc(struct uvc_vs_input_desc *desc, struct uvc_desc_head *next,
                                    struct uvc_desc_head *parent, UVCL_DescConf *p_conf, int format_nb)
{
  int i;

  assert(format_nb > 0 && format_nb <= ARRAY_LEN(desc->raw.bmaControls));

  desc->head.bLength = 13 + format_nb;
  desc->head.raw = (uint8_t *) &desc->raw;
  desc->head.gen = gen_default_desc;
  desc->head.update = update_uvc_vs_input_desc;
  desc->head.next = next;
  desc->raw.bLength = 13 + format_nb;
  desc->raw.bDescriptorType = CS_INTERFACE;
  desc->raw.bDescriptorSubType = VS_INPUT_HEADER;
  desc->raw.bNumFormats = format_nb;
  desc->raw.wTotalLength = desc->head.bLength;
  desc->raw.bEndpointAddress = 0x81;
  desc->raw.bmInfo = 0;
  desc->raw.bTerminalLink = 2;
  desc->raw.bStillCaptureMethod = 0;
  desc->raw.bTriggerSupport = 0;
  desc->raw.bTriggerUsage = 0;
  desc->raw.bControlSize = 1;
  for (i = 0; i < format_nb; i++)
    desc->raw.bmaControls[i] = 0;

  append_as_child(parent, &desc->head);
}

static void build_uvc_yuv422_fmt_desc(struct uvc_yuv422_fmt_desc *desc, struct uvc_desc_head *next,
                                      struct uvc_desc_head *parent, UVCL_DescConf *p_conf, uint8_t frame_desc_nb,
                                      int idx)
{
  const uint8_t YUV422_guid[] = {'Y', 'U', 'Y', '2', 0x00, 0x00, 0x10, 0x00,
                 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71};
                 int i;

  desc->head.bLength = sizeof(desc->raw);
  desc->head.raw = (uint8_t *) &desc->raw;
  desc->head.gen = gen_default_desc;
  desc->head.next = next;
  desc->raw.bLength = sizeof(desc->raw);
  desc->raw.bDescriptorType = CS_INTERFACE;
  desc->raw.bDescriptorSubType = VS_FORMAT_UNCOMPRESSED;
  desc->raw.bFormatIndex = idx + 1;
  desc->raw.bNumFrameDescriptors = frame_desc_nb;
  for (i = 0; i < 16; i++)
    desc->raw.guidFormat[i] = YUV422_guid[i];
  desc->raw.bBitsPerPixel = 16;
  desc->raw.bDefaultFrameIndex = 1;
  desc->raw.bAspectRatioX = 0;
  desc->raw.bAspectRatioY = 0;
  desc->raw.bmInterlaceFlags = 0;
  desc->raw.bCopyProtect = 0;

  append_as_child(parent, &desc->head);
}

static void build_uvc_yuv422_frame_desc(struct uvc_yuv422_frame_desc *desc, struct uvc_desc_head *next,
                                         struct uvc_desc_head *parent, UVCL_DescConf *p_conf, int idx,
                                         uint16_t wWidth, uint16_t wHeight, int fps[UVCL_MAX_STREAM_CONF_NB],
                                         int fps_nb)
{
  int min_fps = compute_min_fps(fps, fps_nb);
  int max_fps = compute_max_fps(fps, fps_nb);
  int i;

  assert(fps_nb > 0 && fps_nb <= UVCL_MAX_STREAM_CONF_NB);
  assert(wWidth);
  assert(wHeight);

  desc->head.bLength = 26 + 4 * fps_nb;
  desc->head.raw = (uint8_t *) &desc->raw;
  desc->head.gen = gen_default_desc;
  desc->head.next = next;
  desc->raw.bLength = 26 + 4 * fps_nb;
  desc->raw.bDescriptorType = CS_INTERFACE;
  desc->raw.bDescriptorSubType = VS_FRAME_UNCOMPRESSED;
  desc->raw.bFrameIndex = idx + 1;
  desc->raw.bmCapabilities = 2;
  desc->raw.wWidth = wWidth;
  desc->raw.wHeight = wHeight;
  desc->raw.dwMinBitRate = desc->raw.wWidth * desc->raw.wHeight * min_fps * 16;
  desc->raw.dwMaxBitRate = desc->raw.wWidth * desc->raw.wHeight * max_fps * 16;
  desc->raw.dwMaxVideoFrameBufferSize = desc->raw.wWidth * desc->raw.wHeight * 2;
  desc->raw.dwDefaultFrameInterval = 10000000 / fps[0];
  desc->raw.bFrameIntervalType = fps_nb;
  for (i = 0; i < fps_nb; i++)
    desc->raw.dwFrameInterval[i] = 10000000 / fps[i];

  append_as_child(parent, &desc->head);
}

static void build_uvc_jpeg_fmt_desc(struct uvc_jpeg_fmt_desc *desc, struct uvc_desc_head *next,
                                    struct uvc_desc_head *parent, UVCL_DescConf *p_conf, uint8_t frame_desc_nb, int idx)
{
  desc->head.bLength = sizeof(desc->raw);
  desc->head.raw = (uint8_t *) &desc->raw;
  desc->head.gen = gen_default_desc;
  desc->head.next = next;
  desc->raw.bLength = sizeof(desc->raw);
  desc->raw.bDescriptorType = CS_INTERFACE;
  desc->raw.bDescriptorSubType = VS_FORMAT_MJPEG;
  desc->raw.bFormatIndex = idx + 1;
  desc->raw.bNumFrameDescriptors = frame_desc_nb;
  desc->raw.bmFlags = 1;
  desc->raw.bDefaultFrameIndex = 1;
  desc->raw.bAspectRatioX = 0;
  desc->raw.bAspectRatioY = 0;
  desc->raw.bmInterlaceFlags = 0;
  desc->raw.bCopyProtect = 0;

  append_as_child(parent, &desc->head);
}

static void build_uvc_jpeg_frame_desc(struct uvc_jpeg_frame_desc *desc, struct uvc_desc_head *next,
                                      struct uvc_desc_head *parent, UVCL_DescConf *p_conf, int idx, uint16_t wWidth,
                                      uint16_t wHeight, int fps[UVCL_MAX_STREAM_CONF_NB], int fps_nb,
                                      uint32_t dwMaxVideoFrameSize)
{
  int min_fps = compute_min_fps(fps, fps_nb);
  int max_fps = compute_max_fps(fps, fps_nb);
  int i;

  assert(fps_nb > 0 && fps_nb <= UVCL_MAX_STREAM_CONF_NB);
  assert(wWidth);
  assert(wHeight);

  desc->head.bLength = 26 + 4 * fps_nb;
  desc->head.raw = (uint8_t *) &desc->raw;
  desc->head.gen = gen_default_desc;
  desc->head.next = next;
  desc->raw.bLength = 26 + 4 * fps_nb;
  desc->raw.bDescriptorType = CS_INTERFACE;
  desc->raw.bDescriptorSubType = VS_FRAME_MJPEG;
  desc->raw.bFrameIndex = idx + 1;
  desc->raw.bmCapabilities = 2;
  desc->raw.wWidth = wWidth;
  desc->raw.wHeight = wHeight;
  desc->raw.dwMinBitRate = dwMaxVideoFrameSize * min_fps * 8;
  desc->raw.dwMaxBitRate = dwMaxVideoFrameSize * max_fps * 8;
  desc->raw.dwMaxVideoFrameBufferSize = dwMaxVideoFrameSize;
  desc->raw.dwDefaultFrameInterval = 10000000 / fps[0];
  desc->raw.bFrameIntervalType = fps_nb;
  for (i = 0; i < fps_nb; i++)
    desc->raw.dwFrameInterval[i] = 10000000 / fps[i];

  append_as_child(parent, &desc->head);
}

static void build_uvc_fb_rgb565_fmt_desc(struct uvc_fb_fmt_desc *desc, UVCL_DescConf *p_conf)
{
  const uint8_t fb_rgb565_guid[] = {'R', 'G', 'B', 'P', 0x00, 0x00, 0x10, 0x00,
                 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71};
  int i;

  for (i = 0; i < 16; i++)
    desc->raw.guidFormat[i] = fb_rgb565_guid[i];
  desc->raw.bBitsPerPixel = 16;
  desc->raw.bVariableSize = 0;
}

static void build_uvc_fb_grey_fmt_desc(struct uvc_fb_fmt_desc *desc, UVCL_DescConf *p_conf, int payload_type)
{
  const uint8_t fb_grey_guid_l8[] = {0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00,
                 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71};
  const uint8_t fb_grey_guid[] = {'Y', '8', ' ', ' ', 0x00, 0x00, 0x10, 0x00,
                 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71};
  int i;

  for (i = 0; i < 16; i++)
    desc->raw.guidFormat[i] = payload_type == UVCL_PAYLOAD_FB_GREY ? fb_grey_guid[i] : fb_grey_guid_l8[i];
  desc->raw.bBitsPerPixel = 8;
  desc->raw.bVariableSize = 0;
}

static void build_uvc_fb_h264_fmt_desc(struct uvc_fb_fmt_desc *desc, UVCL_DescConf *p_conf)
{
  const uint8_t fb_h264_guid[] = {'H', '2', '6', '4', 0x00, 0x00, 0x10, 0x00,
                 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71};
  int i;

  for (i = 0; i < 16; i++)
    desc->raw.guidFormat[i] = fb_h264_guid[i];
  desc->raw.bBitsPerPixel = 0;
  desc->raw.bVariableSize = 1;
}

static void build_uvc_fb_bgr3_fmt_desc(struct uvc_fb_fmt_desc *desc, UVCL_DescConf *p_conf)
{
  const uint8_t fb_rgb888_guid[] = {0x7d, 0xeb, 0x36, 0xe4, 0x4f, 0x52, 0xce, 0x11,
                 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70};
  int i;

  for (i = 0; i < 16; i++)
    desc->raw.guidFormat[i] = fb_rgb888_guid[i];
  desc->raw.bBitsPerPixel = 24;
  desc->raw.bVariableSize = 0;
}

static void build_uvc_fb_jpeg_fmt_desc(struct uvc_fb_fmt_desc *desc, UVCL_DescConf *p_conf)
{
  const uint8_t fb_h264_guid[] = {'M', 'J', 'P', 'G', 0x00, 0x00, 0x10, 0x00,
                 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71};
  int i;

  for (i = 0; i < 16; i++)
    desc->raw.guidFormat[i] = fb_h264_guid[i];
  desc->raw.bBitsPerPixel = 0;
  desc->raw.bVariableSize = 1;
}

static void build_uvc_fb_fmt_desc(struct uvc_fb_fmt_desc *desc, struct uvc_desc_head *next,
                                  struct uvc_desc_head *parent, UVCL_DescConf *p_conf, uint8_t frame_desc_nb,
                                  int payload_type, int idx)
{
  desc->head.bLength = sizeof(desc->raw);
  desc->head.raw = (uint8_t *) &desc->raw;
  desc->head.gen = gen_default_desc;
  desc->head.next = next;
  desc->raw.bLength = sizeof(desc->raw);
  desc->raw.bDescriptorType = CS_INTERFACE;
  desc->raw.bDescriptorSubType = VS_FORMAT_FRAME_BASED;
  desc->raw.bFormatIndex = idx + 1;
  desc->raw.bNumFrameDescriptors = frame_desc_nb;
  desc->raw.bDefaultFrameIndex = 1;
  desc->raw.bAspectRatioX = 0;
  desc->raw.bAspectRatioY = 0;
  desc->raw.bmInterlaceFlags = 0;
  desc->raw.bCopyProtect = 0;

  /* Fill guidFormat, bBitsPerPixel and bVariableSize according to payload type */
  switch (payload_type) {
  case UVCL_PAYLOAD_FB_RGB565:
    build_uvc_fb_rgb565_fmt_desc(desc, p_conf);
    break;
  case UVCL_PAYLOAD_FB_GREY:
  case UVCL_PAYLOAD_FB_GREY_D3DFMT_L8:
    build_uvc_fb_grey_fmt_desc(desc, p_conf, payload_type);
    break;
  case UVCL_PAYLOAD_FB_H264:
    build_uvc_fb_h264_fmt_desc(desc, p_conf);
    break;
  case UVCL_PAYLOAD_FB_BGR3:
    build_uvc_fb_bgr3_fmt_desc(desc, p_conf);
    break;
  case UVCL_PAYLOAD_FB_JPEG:
    build_uvc_fb_jpeg_fmt_desc(desc, p_conf);
    break;
  default:
    assert(0);
  }

  append_as_child(parent, &desc->head);
}

static void build_uvc_fb_rgb565_frame_desc(struct uvc_fb_frame_desc *desc, int min_fps, int max_fps)
{
  desc->raw.dwMinBitRate = desc->raw.wWidth * desc->raw.wHeight * min_fps * 16;
  desc->raw.dwMaxBitRate = desc->raw.wWidth * desc->raw.wHeight * max_fps * 16;
  desc->raw.dwBytesPerLine = desc->raw.wWidth * 2;
}

static void build_uvc_fb_grey_frame_desc(struct uvc_fb_frame_desc *desc, int min_fps, int max_fps)
{
  desc->raw.dwMinBitRate = desc->raw.wWidth * desc->raw.wHeight * min_fps * 8;
  desc->raw.dwMaxBitRate = desc->raw.wWidth * desc->raw.wHeight * max_fps * 8;
  desc->raw.dwBytesPerLine = desc->raw.wWidth * 1;
}

static void build_uvc_fb_h264_frame_desc(struct uvc_fb_frame_desc *desc, int min_fps, int max_fps)
{
  desc->raw.dwMinBitRate = desc->raw.wWidth * desc->raw.wHeight * min_fps;
  desc->raw.dwMaxBitRate = desc->raw.wWidth * desc->raw.wHeight * max_fps;
  desc->raw.dwBytesPerLine = 0;
}

static void build_uvc_fb_bgr3_frame_desc(struct uvc_fb_frame_desc *desc, int min_fps, int max_fps)
{
  desc->raw.dwMinBitRate = desc->raw.wWidth * desc->raw.wHeight * min_fps * 24;
  desc->raw.dwMaxBitRate = desc->raw.wWidth * desc->raw.wHeight * max_fps * 24;
  desc->raw.dwBytesPerLine = desc->raw.wWidth * 3;
}

static void build_uvc_fb_jpeg_frame_desc(struct uvc_fb_frame_desc *desc, int min_fps, int max_fps)
{
  desc->raw.dwMinBitRate = desc->raw.wWidth * desc->raw.wHeight * min_fps;
  desc->raw.dwMaxBitRate = desc->raw.wWidth * desc->raw.wHeight * max_fps;
  desc->raw.dwBytesPerLine = 0;
}

static void build_uvc_fb_frame_desc(struct uvc_fb_frame_desc *desc, struct uvc_desc_head *next,
                                    struct uvc_desc_head *parent, UVCL_DescConf *p_conf, int idx, uint16_t wWidth,
                                    uint16_t wHeight, int fps[UVCL_MAX_STREAM_CONF_NB], int fps_nb, int payload_type)
{
  int min_fps = compute_min_fps(fps, fps_nb);
  int max_fps = compute_max_fps(fps, fps_nb);
  int i;

  desc->head.bLength = 26 + 4 * fps_nb;
  desc->head.raw = (uint8_t *) &desc->raw;
  desc->head.gen = gen_default_desc;
  desc->head.next = next;
  desc->raw.bLength = 26 + 4 * fps_nb;
  desc->raw.bDescriptorType = CS_INTERFACE;
  desc->raw.bDescriptorSubType = VS_FRAME_FRAME_BASED;
  desc->raw.bFrameIndex = idx + 1;
  desc->raw.bmCapabilities = 2;
  desc->raw.wWidth = wWidth;
  desc->raw.wHeight = wHeight;
  desc->raw.dwDefaultFrameInterval = 10000000 / fps[0];
  desc->raw.bFrameIntervalType = fps_nb;
  for (i = 0; i < fps_nb; i++)
    desc->raw.dwFrameInterval[i] = 10000000 / fps[i];

  /* Fill dwMinBitRate, dwMaxBitRate and dwBytesPerLine according to payload type */
  switch (payload_type) {
  case UVCL_PAYLOAD_FB_RGB565:
    build_uvc_fb_rgb565_frame_desc(desc, min_fps, max_fps);
    break;
  case UVCL_PAYLOAD_FB_GREY:
  case UVCL_PAYLOAD_FB_GREY_D3DFMT_L8:
    build_uvc_fb_grey_frame_desc(desc, min_fps, max_fps);
    break;
  case UVCL_PAYLOAD_FB_H264:
    build_uvc_fb_h264_frame_desc(desc, min_fps, max_fps);
    break;
  case UVCL_PAYLOAD_FB_BGR3:
    build_uvc_fb_bgr3_frame_desc(desc, min_fps, max_fps);
    break;
  case UVCL_PAYLOAD_FB_JPEG:
    build_uvc_fb_jpeg_frame_desc(desc, min_fps, max_fps);
    break;
  default:
    assert(0);
  }

  append_as_child(parent, &desc->head);
}

static void build_uvc_color_desc(struct uvc_color_desc *desc, struct uvc_desc_head *next, struct uvc_desc_head *parent,
                                 UVCL_DescConf *p_conf)
{
  desc->head.bLength = sizeof(desc->raw);
  desc->head.raw = (uint8_t *) &desc->raw;
  desc->head.gen = gen_default_desc;
  desc->head.next = next;
  desc->raw.bLength = sizeof(desc->raw);
  desc->raw.bDescriptorType = CS_INTERFACE;
  desc->raw.bDescriptorSubType = VS_COLORFORMAT;
  desc->raw.bColorPrimaries = 1;
  desc->raw.bTransferCharacteristics = 1;
  desc->raw.bMatrixCoefficients = 4;

  append_as_child(parent, &desc->head);
}

static void build_uvc_vs_ep_desc(struct uvc_vs_ep_desc *desc, struct uvc_desc_head *next, struct uvc_desc_head *parent,
                                 UVCL_DescConf *p_conf, int wMaxPacketSize)
{
  desc->head.bLength = sizeof(desc->raw);
  desc->head.raw = (uint8_t *) &desc->raw;
  desc->head.gen = gen_default_desc;
  desc->head.next = next;
  desc->raw.bLength = sizeof(desc->raw);
  desc->raw.bDescriptorType = 5;
  desc->raw.bEndpointAddress = 0x81;
  desc->raw.bmAttributes = 0x05;
  desc->raw.wMaxPacketSize = wMaxPacketSize;
  desc->raw.bInterval = 1;

  append_as_child(parent, &desc->head);
}

static void build_head_conf_desc(struct uvc_head_conf_desc *desc, UVCL_DescConf *p_conf, uint16_t wMaxPacketSize, 
                                 uint8_t bNumFormats, struct uvc_desc_head *next)
{
  build_uvc_conf_desc(&desc->conf_desc, &desc->iad_desc.head, p_conf);
    build_uvc_iad_desc(&desc->iad_desc, &desc->std_vc_desc.head, &desc->conf_desc.head, p_conf);
      build_uvc_std_vc_desc(&desc->std_vc_desc, &desc->class_vc_desc.head, &desc->iad_desc.head, p_conf);
        build_uvc_class_vc_desc(&desc->class_vc_desc, &desc->cam_desc.head, &desc->std_vc_desc.head, p_conf);
          build_uvc_camera_terminal_desc(&desc->cam_desc, &desc->tt_desc.head, &desc->class_vc_desc.head, p_conf);
          build_uvc_output_term_desc(&desc->tt_desc, &desc->std_vs_alt0_desc.head, &desc->class_vc_desc.head, p_conf);
      build_uvc_std_vs_desc(&desc->std_vs_alt0_desc, &desc->vs_input_desc.head, &desc->iad_desc.head, p_conf, 0, 0);
        build_uvc_vs_input_desc(&desc->vs_input_desc, next, &desc->std_vs_alt0_desc.head, p_conf,
                                bNumFormats);

}

static void compute_payload_res_and_fps(UVCL_DescConf *p_conf, uint8_t frame_desc_idx, uint16_t *wHeight,
                                        uint16_t *wWidth, int *fps_nb, int fps[UVCL_MAX_STREAM_CONF_NB],
                                        int payload_type, uint32_t *dwMaxVideoFrameSize)
{
  uint8_t current_frame_desc_idx = 0xff;
  uint32_t maxVideoFrameSize = 0;
  int height = -1;
  int width = -1;
  int i;

  *fps_nb = 0;
  *wWidth = 0;
  *wHeight = 0;
  for (i = 0; i < p_conf->streams_nb; i++) {
    if (p_conf->streams[i].payload_type != payload_type)
      continue;

    if (p_conf->streams[i].width == *wWidth && p_conf->streams[i].height == *wHeight) {
      fps[*fps_nb] = p_conf->streams[i].fps;
      *fps_nb += 1;
      continue;
    }

    current_frame_desc_idx++;
    height = p_conf->streams[i].height;
    width = p_conf->streams[i].width;
    maxVideoFrameSize = p_conf->streams[i].dwMaxVideoFrameSize;

    if (frame_desc_idx != current_frame_desc_idx)
      continue;

    *wWidth = width;
    *wHeight = height;
    if (dwMaxVideoFrameSize)
      *dwMaxVideoFrameSize = maxVideoFrameSize ? maxVideoFrameSize : width * height;
    fps[*fps_nb] = p_conf->streams[i].fps;
    *fps_nb += 1;
  }

  assert(*fps_nb);
  assert(*wWidth);
  assert(*wHeight);
}

static void build_middle_yuv422_conf_desc(struct uvc_middle_yuv422_conf_desc *desc, UVCL_DescConf *p_conf,
                                          struct uvc_desc_head *parent, uint8_t frame_desc_nb,
                                          struct uvc_desc_head *next, int idx)
{
  struct uvc_desc_head *next_for_frame;
  int fps[UVCL_MAX_STREAM_CONF_NB];
  uint16_t wHeight = 0;
  uint16_t wWidth = 0;
  int fps_nb;
  int i;

  build_uvc_yuv422_fmt_desc(&desc->fmt, &desc->frame[0].head, parent, p_conf, frame_desc_nb, idx);
  for (i = 0; i < frame_desc_nb; i++) {
    next_for_frame = (i == frame_desc_nb - 1) ? &desc->color_desc.head : &desc->frame[i + 1].head;
    compute_payload_res_and_fps(p_conf, i, &wHeight, &wWidth, &fps_nb, fps, UVCL_PAYLOAD_UNCOMPRESSED_YUY2, NULL);
    build_uvc_yuv422_frame_desc(&desc->frame[i], next_for_frame, &desc->fmt.head, p_conf, i, wWidth, wHeight, fps,
                                fps_nb);
  }
  /* FIXME : check parent is &desc->fmt.head */
  build_uvc_color_desc(&desc->color_desc, next, &desc->fmt.head, p_conf);
}

static void build_middle_jpeg_conf_desc(struct uvc_middle_jpeg_conf_desc *desc, UVCL_DescConf *p_conf,
                                      struct uvc_desc_head *parent, uint8_t frame_desc_nb,
                                      struct uvc_desc_head *next, int idx)
{
  struct uvc_desc_head *next_for_frame;
  int fps[UVCL_MAX_STREAM_CONF_NB];
  uint32_t dwMaxVideoFrameSize;
  uint16_t wHeight = 0;
  uint16_t wWidth = 0;
  int fps_nb;
  int i;

  build_uvc_jpeg_fmt_desc(&desc->fmt, &desc->frame[0].head, parent, p_conf, frame_desc_nb, idx);
  for (i = 0; i < frame_desc_nb; i++) {
    next_for_frame = (i == frame_desc_nb - 1) ? &desc->color_desc.head : &desc->frame[i + 1].head;
    compute_payload_res_and_fps(p_conf, i, &wHeight, &wWidth, &fps_nb, fps, UVCL_PAYLOAD_JPEG,
                                &dwMaxVideoFrameSize);
    build_uvc_jpeg_frame_desc(&desc->frame[i], next_for_frame, &desc->fmt.head, p_conf, i, wWidth, wHeight, fps,
                                fps_nb,dwMaxVideoFrameSize);
  }
  /* FIXME : check parent is &desc->fmt.head */
  build_uvc_color_desc(&desc->color_desc, next, &desc->fmt.head, p_conf);
}

static void build_middle_fb_conf_desc(struct uvc_middle_fb_conf_desc *desc, UVCL_DescConf *p_conf,
                                      struct uvc_desc_head *parent, uint8_t frame_desc_nb,
                                      struct uvc_desc_head *next, int payload_type, int idx)
{
  struct uvc_desc_head *next_for_frame;
  int fps[UVCL_MAX_STREAM_CONF_NB];
  uint16_t wHeight = 0;
  uint16_t wWidth = 0;
  int fps_nb;
  int i;

  build_uvc_fb_fmt_desc(&desc->fmt, &desc->frame[0].head, parent, p_conf, frame_desc_nb, payload_type, idx);
  for (i = 0; i < frame_desc_nb; i++) {
    next_for_frame = (i == frame_desc_nb - 1) ? &desc->color_desc.head : &desc->frame[i + 1].head;
    compute_payload_res_and_fps(p_conf, i, &wHeight, &wWidth, &fps_nb, fps, payload_type, NULL);
    build_uvc_fb_frame_desc(&desc->frame[i], next_for_frame,  &desc->fmt.head, p_conf, i, wWidth, wHeight, fps,
                            fps_nb, payload_type);
  }
  /* FIXME : check parent is &desc->fmt.head */
  build_uvc_color_desc(&desc->color_desc, next, &desc->fmt.head, p_conf);
}

static void build_tail_conf_desc(struct uvc_tail_conf_desc *desc, UVCL_DescConf *p_conf, struct uvc_desc_head *parent,
                                 uint16_t wMaxPacketSize)
{
  build_uvc_std_vs_desc(&desc->std_vs_alt1_desc, &desc->ep_desc.head, parent, p_conf, 1, 1);
    build_uvc_vs_ep_desc(&desc->ep_desc, NULL, &desc->std_vs_alt1_desc.head, p_conf, wMaxPacketSize);
}

static uint8_t compute_format_nb(UVCL_DescConf *p_conf, int payloads_type[UVCL_MAX_STREAM_CONF_NB])
{
  int current_payload_type = p_conf->streams[0].payload_type;
  uint8_t bNumFormats = 1;
  int i;

  payloads_type[0] = current_payload_type;
  for (i = 1; i < p_conf->streams_nb; i++) {
    if (p_conf->streams[i].payload_type == current_payload_type)
      continue;

    current_payload_type = p_conf->streams[i].payload_type;
    payloads_type[bNumFormats++] = current_payload_type;
  }

  return bNumFormats;
}

static uint8_t compute_frame_nb(UVCL_DescConf *p_conf, int payload_type)
{
  uint8_t frame_desc_nb = 0;
  int height = 0;
  int width = 0;
  int i;

  for (i = 0; i < p_conf->streams_nb; i++) {
    if (p_conf->streams[i].payload_type != payload_type)
      continue;

    if (p_conf->streams[i].width == width && p_conf->streams[i].height == height)
      continue;

    frame_desc_nb++;
    height = p_conf->streams[i].height;
    width = p_conf->streams[i].width;
  }

  return frame_desc_nb;
}

/* Public API */
int UVCL_get_configuration_desc(void *p_dst, int dst_len, UVCL_DescConf *p_conf, UVCL_DescBuffer *p_buffer)
{
  int payloads_type[UVCL_MAX_STREAM_CONF_NB];
  struct uvc_desc_head *next_for_format;
  struct uvc_middle_conf_desc *middle;
  uint16_t wMaxPacketSize = 1023;
  struct uvc_head_conf_desc head;
  struct uvc_tail_conf_desc tail;
  struct buffer_allocator ba;
  uint8_t bNumFormats;
  int ret;
  int i;

  if (p_conf->is_hs) {
    assert(USBL_PACKET_PER_MICRO_FRAME >= 1 && USBL_PACKET_PER_MICRO_FRAME <= 3);
    wMaxPacketSize = 1024 | ((USBL_PACKET_PER_MICRO_FRAME - 1) << 11);
  }

  ba.buffer = p_buffer->buffer;
  ba.buffer_size = p_buffer->buffer_size;
  ret = ba_init(&ba);
  if (ret)
    return ret;
  bNumFormats = compute_format_nb(p_conf, payloads_type);
  middle = ba_alloc(&ba, bNumFormats * sizeof(struct uvc_middle_conf_desc));
  if (!middle)
    return -1;

  memset(middle, 0, bNumFormats * sizeof(struct uvc_middle_conf_desc));
  memset(&head, 0, sizeof(head));
  memset(&tail, 0, sizeof(tail));

  /* FIXME : not very clean to select yuv422.fmt.head even if it will be correct */
  build_head_conf_desc(&head, p_conf, wMaxPacketSize, bNumFormats, &middle[0].yuv422.fmt.head);
  for (i = 0; i < bNumFormats; i++) {
    /* FIXME : not very clean to select yuv422.fmt.head even if it will be correct */
    next_for_format = i == bNumFormats - 1 ? &tail.std_vs_alt1_desc.head : &middle[i + 1].yuv422.fmt.head;
    uint8_t frame_desc_nb = compute_frame_nb(p_conf, payloads_type[i]);
    switch (payloads_type[i]) {
    case UVCL_PAYLOAD_UNCOMPRESSED_YUY2:
      build_middle_yuv422_conf_desc(&middle[i].yuv422, p_conf, &head.vs_input_desc.head, frame_desc_nb, next_for_format, i);
      break;
    case UVCL_PAYLOAD_JPEG:
      build_middle_jpeg_conf_desc(&middle[i].jpeg, p_conf, &head.vs_input_desc.head, frame_desc_nb, next_for_format, i);
      break;
    case UVCL_PAYLOAD_FB_RGB565:
    case UVCL_PAYLOAD_FB_BGR3:
    case UVCL_PAYLOAD_FB_GREY:
    case UVCL_PAYLOAD_FB_H264:
    case UVCL_PAYLOAD_FB_JPEG:
    case UVCL_PAYLOAD_FB_GREY_D3DFMT_L8:
      build_middle_fb_conf_desc(&middle[i].fb, p_conf, &head.vs_input_desc.head, frame_desc_nb, next_for_format,
                                payloads_type[i], i);
      break;
    default:
      assert(0);
    }
  }
  build_tail_conf_desc(&tail, p_conf, &head.iad_desc.head, wMaxPacketSize);

  update(&head.conf_desc.head);
  return generate(&head.conf_desc.head, p_dst, dst_len);
}

int UVCL_get_device_desc(void *p_dst, int dst_len, int idx_manufacturer, int idx_product, int idx_serial)
{
  struct uvc_dev_desc dev_desc = { 0 };

  build_dev_desc(&dev_desc, NULL, NULL, idx_manufacturer, idx_product, idx_serial);
  update(&dev_desc.head);

  return generate(&dev_desc.head, p_dst, dst_len);
}

int UVCL_get_device_qualifier_desc(void *p_dst, int dst_len)
{
  uint8_t *dst = p_dst;

  if (dst_len < 10)
    return -1;

  dst[0] = 0x0a; /* bLength = 10 */
  dst[1] = 0x06; /* dev qualifier desc type */
  dst[2] = 0x00; /* bcdUSB low */
  dst[3] = 0x02; /* bcdUSB high */
  dst[4] = 0xEF; /* bDeviceClass */
  dst[5] = 0x02; /* bDeviceSubClass */
  dst[6] = 0x01; /* bDeviceProtocol */
  dst[7] = 0x40; /* bMaxPacketSize0 64 */
  dst[8] = 0x01; /* bNumConfigurations */
  dst[9] = 0x00; /* bReserved */

  return dst[0];
}

int UVCL_get_lang_string_desc(void *p_dst, int dst_len)
{
  uint8_t *dst = p_dst;

  if (dst_len < 4)
    return -1;

  dst[0] = 0x04; /* bLength = 4 */
  dst[1] = 0x03; /* string descriptor */
  dst[2] = 0x09; /* United States */
  dst[3] = 0x04; /* United States */

  return dst[0];
}

int UVCL_get_manufacturer_string_desc(void *p_dst, int dst_len)
{
  return UVCL_get_string_desc(p_dst, dst_len, DEV_MANUFACTURER_STRING);
}

int UVCL_get_product_string_desc(void *p_dst, int dst_len)
{
  return UVCL_get_string_desc(p_dst, dst_len, DEV_PRODUCT_STRING);
}

int UVCL_get_serial_string_desc(void *p_dst, int dst_len)
{
  char serial[13];

  gen_serial(serial);
  return UVCL_get_string_desc(p_dst, dst_len, serial);
}

