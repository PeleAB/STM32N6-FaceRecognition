 /**
 ******************************************************************************
 * @file    uvcl_desc_internal.h
 * @author  GPM Application Team
 *
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
#ifndef UVCL_DESC_INTERNAL
#define UVCL_DESC_INTERNAL

#include <stdint.h>

#include "uvcl.h"

#include "cmsis_compiler.h"

struct uvc_desc_head {
  uint8_t bLength;
  uint8_t *raw;
  struct uvc_desc_head *next;
  struct uvc_desc_head *next_child;
  struct uvc_desc_head *children;
  int (*gen)(struct uvc_desc_head *head, uint8_t *p_dst, int dst_len);
  void (*update)(struct uvc_desc_head *head);
};

struct uvc_dev_desc_raw {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint16_t bcdUSB;
  uint8_t bDeviceClass;
  uint8_t bDeviceSubClass;
  uint8_t bDeviceProtocol;
  uint8_t bMaxPacketSize;
  uint16_t idVendor;
  uint16_t idProduct;
  uint16_t bcdDevice;
  uint8_t iManufacturer;
  uint8_t iProduct;
  uint8_t iSerialNumber;
  uint8_t bNumConfigurations;
} __PACKED;

struct uvc_dev_desc {
  struct uvc_desc_head head;
  struct uvc_dev_desc_raw raw;
};

struct uvc_conf_desc_raw {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint16_t wTotalLength;
  uint8_t bNumInterfaces;
  uint8_t bConfigurationValue;
  uint8_t iConfiguration;
  uint8_t bmAttributes;
  uint8_t bMaxPower;
} __PACKED;

struct uvc_conf_desc {
  struct uvc_desc_head head;
  struct uvc_conf_desc_raw raw;
};

struct uvc_iad_desc_raw {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint8_t bFirstInterface;
  uint8_t bInterfaceCount;
  uint8_t bFunctionClass;
  uint8_t bFunctionSubClass;
  uint8_t bFunctionProtocol;
  uint8_t iFunction;
} __PACKED;

struct uvc_iad_desc {
  struct uvc_desc_head head;
  struct uvc_iad_desc_raw raw;
};

struct uvc_std_vc_desc_raw {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint8_t bInterfaceNumber;
  uint8_t bAlternateSetting;
  uint8_t bNumEndpoints;
  uint8_t bInterfaceClass;
  uint8_t bInterfaceSubClass;
  uint8_t bInterfaceProtocol;
  uint8_t iInterface;
} __PACKED;

struct uvc_std_vc_desc {
  struct uvc_desc_head head;
  struct uvc_std_vc_desc_raw raw;
};

struct uvc_class_vc_desc_raw {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint8_t bDescriptorSubType;
  uint16_t bcdUVC;
  uint16_t wTotalLength;
  uint32_t dwClockFrequency;
  uint8_t bInCollection;
  uint8_t baInterfaceNr[1];
} __PACKED;

struct uvc_class_vc_desc {
  struct uvc_desc_head head;
  struct uvc_class_vc_desc_raw raw;
};

struct uvc_camera_terminal_desc_raw {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint8_t bDescriptorSubType;
  uint8_t bTerminalID;
  uint16_t wTerminalType;
  uint8_t bAssocTerminal;
  uint8_t iTerminal;
  uint16_t wObjectiveFocalLengthMin;
  uint16_t wObjectiveFocalLengthMax;
  uint16_t wOcularFocalLength;
  uint8_t bControlSize;
  uint8_t bmControls[3];
} __PACKED;

struct uvc_camera_terminal_desc {
  struct uvc_desc_head head;
  struct uvc_camera_terminal_desc_raw raw;
};

struct uvc_output_term_desc_raw {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint8_t bDescriptorSubType;
  uint8_t bTerminalID;
  uint16_t wTerminalType;
  uint8_t bAssocTerminal;
  uint8_t bSourceID;
  uint8_t iTerminal;
} __PACKED;

struct uvc_output_term_desc {
  struct uvc_desc_head head;
  struct uvc_output_term_desc_raw raw;
};

struct uvc_std_vs_desc_raw {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint8_t bInterfaceNumber;
  uint8_t bAlternateSetting;
  uint8_t bNumEndpoints;
  uint8_t bInterfaceClass;
  uint8_t bInterfaceSubClass;
  uint8_t bInterfaceProtocol;
  uint8_t iInterface;
} __PACKED;

struct uvc_std_vs_desc {
  struct uvc_desc_head head;
  struct uvc_std_vs_desc_raw raw;
};

struct uvc_vs_input_desc_raw {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint8_t bDescriptorSubType;
  uint8_t bNumFormats;
  uint16_t wTotalLength;
  uint8_t bEndpointAddress;
  uint8_t bmInfo;
  uint8_t bTerminalLink;
  uint8_t bStillCaptureMethod;
  uint8_t bTriggerSupport;
  uint8_t bTriggerUsage;
  uint8_t bControlSize;
  uint8_t bmaControls[UVCL_MAX_STREAM_CONF_NB];
} __PACKED;

struct uvc_vs_input_desc {
  struct uvc_desc_head head;
  struct uvc_vs_input_desc_raw raw;
};

struct uvc_yuv422_fmt_desc_raw {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint8_t bDescriptorSubType;
  uint8_t bFormatIndex;
  uint8_t bNumFrameDescriptors;
  uint8_t guidFormat[16];
  uint8_t bBitsPerPixel;
  uint8_t bDefaultFrameIndex;
  uint8_t bAspectRatioX;
  uint8_t bAspectRatioY;
  uint8_t bmInterlaceFlags;
  uint8_t bCopyProtect;
} __PACKED;

struct uvc_yuv422_fmt_desc {
  struct uvc_desc_head head;
  struct uvc_yuv422_fmt_desc_raw raw;
};

struct uvc_yuv422_frame_desc_raw {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint8_t bDescriptorSubType;
  uint8_t bFrameIndex;
  uint8_t bmCapabilities;
  uint16_t wWidth;
  uint16_t wHeight;
  uint32_t dwMinBitRate;
  uint32_t dwMaxBitRate;
  uint32_t dwMaxVideoFrameBufferSize;
  uint32_t dwDefaultFrameInterval;
  uint8_t bFrameIntervalType;
  uint32_t dwFrameInterval[UVCL_MAX_STREAM_CONF_NB];
} __PACKED;

struct uvc_yuv422_frame_desc {
  struct uvc_desc_head head;
  struct uvc_yuv422_frame_desc_raw raw;
};

struct uvc_color_desc_raw {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint8_t bDescriptorSubType;
  uint8_t bColorPrimaries;
  uint8_t bTransferCharacteristics;
  uint8_t bMatrixCoefficients;
} __PACKED;

struct uvc_color_desc {
  struct uvc_desc_head head;
  struct uvc_color_desc_raw raw;
};

struct uvc_vs_ep_desc_raw {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint8_t bEndpointAddress;
  uint8_t bmAttributes;
  uint16_t wMaxPacketSize;
  uint8_t bInterval;
} __PACKED;

struct uvc_vs_ep_desc {
  struct uvc_desc_head head;
  struct uvc_vs_ep_desc_raw raw;
};

struct uvc_jpeg_fmt_desc_raw {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint8_t bDescriptorSubType;
  uint8_t bFormatIndex;
  uint8_t bNumFrameDescriptors;
  uint8_t bmFlags;
  uint8_t bDefaultFrameIndex;
  uint8_t bAspectRatioX;
  uint8_t bAspectRatioY;
  uint8_t bmInterlaceFlags;
  uint8_t bCopyProtect;
} __PACKED;

struct uvc_jpeg_fmt_desc {
  struct uvc_desc_head head;
  struct uvc_jpeg_fmt_desc_raw raw;
};

struct uvc_jpeg_frame_desc_raw {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint8_t bDescriptorSubType;
  uint8_t bFrameIndex;
  uint8_t bmCapabilities;
  uint16_t wWidth;
  uint16_t wHeight;
  uint32_t dwMinBitRate;
  uint32_t dwMaxBitRate;
  uint32_t dwMaxVideoFrameBufferSize;
  uint32_t dwDefaultFrameInterval;
  uint8_t bFrameIntervalType;
  uint32_t dwFrameInterval[UVCL_MAX_STREAM_CONF_NB];
} __PACKED;

struct uvc_jpeg_frame_desc {
  struct uvc_desc_head head;
  struct uvc_jpeg_frame_desc_raw raw;
};

struct uvc_fb_fmt_desc_raw {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint8_t bDescriptorSubType;
  uint8_t bFormatIndex;
  uint8_t bNumFrameDescriptors;
  uint8_t guidFormat[16];
  uint8_t bBitsPerPixel;
  uint8_t bDefaultFrameIndex;
  uint8_t bAspectRatioX;
  uint8_t bAspectRatioY;
  uint8_t bmInterlaceFlags;
  uint8_t bCopyProtect;
  uint8_t bVariableSize;
} __PACKED;

struct uvc_fb_fmt_desc {
  struct uvc_desc_head head;
  struct uvc_fb_fmt_desc_raw raw;
};

struct uvc_fb_frame_desc_raw {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint8_t bDescriptorSubType;
  uint8_t bFrameIndex;
  uint8_t bmCapabilities;
  uint16_t wWidth;
  uint16_t wHeight;
  uint32_t dwMinBitRate;
  uint32_t dwMaxBitRate;
  uint32_t dwDefaultFrameInterval;
  uint8_t bFrameIntervalType;
  uint32_t dwBytesPerLine;
  uint32_t dwFrameInterval[UVCL_MAX_STREAM_CONF_NB];
} __PACKED;

struct uvc_fb_frame_desc {
  struct uvc_desc_head head;
  struct uvc_fb_frame_desc_raw raw;
};

struct uvc_head_conf_desc {
  struct uvc_conf_desc conf_desc;
   struct uvc_iad_desc iad_desc;
    struct uvc_std_vc_desc std_vc_desc;
     struct uvc_class_vc_desc class_vc_desc;
      struct uvc_camera_terminal_desc cam_desc;
      struct uvc_output_term_desc tt_desc;
    struct uvc_std_vs_desc std_vs_alt0_desc;
     struct uvc_vs_input_desc vs_input_desc;
};

struct uvc_middle_yuv422_conf_desc {
      struct uvc_yuv422_fmt_desc fmt;
        struct uvc_yuv422_frame_desc frame[UVCL_MAX_STREAM_CONF_NB];
      struct uvc_color_desc color_desc;
};

struct uvc_middle_jpeg_conf_desc {
      struct uvc_jpeg_fmt_desc fmt;
        struct uvc_jpeg_frame_desc frame[UVCL_MAX_STREAM_CONF_NB];
      struct uvc_color_desc color_desc;
};

struct uvc_middle_fb_conf_desc {
      struct uvc_fb_fmt_desc fmt;
        struct uvc_fb_frame_desc frame[UVCL_MAX_STREAM_CONF_NB];
      struct uvc_color_desc color_desc;
};

struct uvc_middle_conf_desc {
  union {
    struct uvc_middle_yuv422_conf_desc yuv422;
    struct uvc_middle_jpeg_conf_desc jpeg;
    struct uvc_middle_fb_conf_desc fb;
  };
};

struct uvc_tail_conf_desc {
    struct uvc_std_vs_desc std_vs_alt1_desc;
     struct uvc_vs_ep_desc ep_desc;
};

#endif

