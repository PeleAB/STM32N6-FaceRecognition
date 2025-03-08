/**
  ******************************************************************************
  * @file    vd55g1.h
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

#ifndef VD55G1_H
#define VD55G1_H

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdarg.h>
#include <stdint.h>

#define VD55G1_LVL_ERROR 0
#define VD55G1_LVL_WARNING 1
#define VD55G1_LVL_NOTICE 2
#define VD55G1_LVL_DBG(l) (3 + (l))
#define VD55G1_MAX_WIDTH  804
#define VD55G1_MAX_HEIGHT 704

enum {
  VD55G1_MIN_FPS = 2,
  VD55G1_MAX_FPS = 168,
};

enum {
  VD55G1_AWU_THRESHOLD_DEFAULT = 0,
  VD55G1_AWU_THRESHOLD_MIN = 2,
  VD55G1_AWU_THRESHOLD_MAX = 128,
};

typedef enum {
  VD55G1_RES_QVGA_320_240,
  VD55G1_RES_VGA_640_480,
  VD55G1_RES_SXGA_800_600,
  VD55G1_RES_FULL_804_704,
} VD55G1_Res_t;

typedef enum {
  VD55G1_MIRROR_FLIP_NONE,
  VD55G1_FLIP,
  VD55G1_MIRROR,
  VD55G1_MIRROR_FLIP
} VD55G1_MirrorFlip_t;

typedef enum {
  VD55G1_PATGEN_DISABLE,
  VD55G1_PATGEN_DIAGONAL_GRAYSCALE,
  VD55G1_PATGEN_PSEUDO_RANDOM,
} VD55G1_PatGen_t;

typedef enum {
  VD55G1_FLICKER_FREE_NONE,
  VD55G1_FLICKER_FREE_50HZ,
  VD55G1_FLICKER_FREE_60HZ
} VD55G1_Flicker_t;

enum {
  VD55G1_MIN_BRIGHTNESS = 0,
  VD55G1_MAX_BRIGHTNESS = 100,
};

enum {
  VD55G1_MIN_DATARATE = 250000000,
  VD55G1_DEFAULT_DATARATE = 804000000,
  VD55G1_MAX_DATARATE = 1200000000,
};

typedef enum {
  VD55G1_GPIO_FSYNC_OUT = 0,
  VD55G1_GPIO_GPIO_IN = 1,
  VD55G1_GPIO_STROBE = 2,
  VD55G1_GPIO_PWM_STROBE = 3,
  VD55G1_GPIO_PWM = 4,
  VD55G1_GPIO_OUT = 5,
  VD55G1_GPIO_VSYNC_OUT_MODE0 = 6,
  VD55G1_GPIO_VSYNC_OUT_MODE1 = 7,
  VD55G1_GPIO_VSYNC_OUT_MODE2 = 8,
  VD55G1_GPIO_EVENT_TRACKER = 9,
  VD55G1_GPIO_VT_SLAVE_MODE = 0xa,
  VD55G1_GPIO_IMAGE_READOUT = 0xc,
  VD55G1_GPIO_AWU_DETECTION = 0xd,
} VD55G1_GPIO_Mode_t;

typedef enum {
  VD55G1_GPIO_LOW = (0 << 4),
  VD55G1_GPIO_HIGH = (1 << 4),
} VD55G1_GPIO_Value_t;

typedef enum {
  VD55G1_GPIO_NO_INVERSION = (0 << 5),
  VD55G1_GPIO_INVERTED = (1 << 5),
} VD55G1_GPIO_Polarity_t;

typedef enum {
  VD55G1_GPIO_0,
  VD55G1_GPIO_1,
  VD55G1_GPIO_2,
  VD55G1_GPIO_3,
  VD55G1_GPIO_NB
} VD55G1_GPIO_t;

/* Output interface configuration */
typedef struct {
  int data_rate_in_mps;
  int clock_lane_swap_enable;
  int data_lane_swap_enable;
} VD55G1_OutItf_Config_t;

/* Auto wakeup configuration */
typedef struct {
  int is_enable;
  int convergence_frame_rate;
  int awu_frame_rate;
  int zone_nb;
  int threshold;
} VD55G1_AWUConfig_t;

/* VD55G1 configuration */
typedef struct {
  int ext_clock_freq_in_hz;
  VD55G1_Res_t resolution;
  int frame_rate;
  VD55G1_MirrorFlip_t flip_mirror_mode;
  VD55G1_PatGen_t patgen;
  VD55G1_Flicker_t flicker;
  VD55G1_OutItf_Config_t out_itf;
  VD55G1_AWUConfig_t awu;
  /* VD55G1_GPIO_Mode_t | VD55G1_GPIO_Value_t | VD55G1_GPIO_Polarity_t */
  uint8_t gpio_ctrl[VD55G1_GPIO_NB];
} VD55G1_Config_t;

typedef struct VD55G1_Ctx
{
  /* API client must set these values */
  void (*shutdown_pin)(struct VD55G1_Ctx *ctx, int value);
  int (*read8)(struct VD55G1_Ctx *ctx, uint16_t addr, uint8_t *value);
  int (*read16)(struct VD55G1_Ctx *ctx, uint16_t addr, uint16_t *value);
  int (*read32)(struct VD55G1_Ctx *ctx, uint16_t addr, uint32_t *value);
  int (*write8)(struct VD55G1_Ctx *ctx, uint16_t addr, uint8_t value);
  int (*write16)(struct VD55G1_Ctx *ctx, uint16_t addr, uint16_t value);
  int (*write32)(struct VD55G1_Ctx *ctx, uint16_t addr, uint32_t value);
  int (*write_array)(struct VD55G1_Ctx *ctx, uint16_t addr, uint8_t *data, int data_len);
  void (*delay)(struct VD55G1_Ctx *ctx, uint32_t delay_in_ms);
  void (*log)(struct VD55G1_Ctx *ctx, int lvl, const char *format, va_list ap);
  /* driver internals. do not touch */
  struct drv_ctx {
    int state;
    int cut_version;
    uint32_t pclk;
    VD55G1_Config_t config_save;
  } ctx;
} VD55G1_Ctx_t;

int VD55G1_Init(VD55G1_Ctx_t *ctx, VD55G1_Config_t *config);
int VD55G1_DeInit(VD55G1_Ctx_t *ctx);
int VD55G1_Start(VD55G1_Ctx_t *ctx);
int VD55G1_Stop(VD55G1_Ctx_t *ctx);
int VD55G1_StartAutoWakeUp(VD55G1_Ctx_t *ctx);
int VD55G1_SetFlipMirrorMode(VD55G1_Ctx_t *ctx, VD55G1_MirrorFlip_t mode);
int VD55G1_GetBrightnessLevel(VD55G1_Ctx_t *ctx, int *level);
int VD55G1_SetBrightnessLevel(VD55G1_Ctx_t *ctx, int level);
int VD55G1_SetFlickerMode(VD55G1_Ctx_t *ctx, VD55G1_Flicker_t mode);

#ifdef __cplusplus
}
#endif

#endif