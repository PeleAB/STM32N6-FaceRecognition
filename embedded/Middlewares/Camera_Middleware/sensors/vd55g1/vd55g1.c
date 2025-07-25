/**
  ******************************************************************************
  * @file    vd55g1.c
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

#include "vd55g1.h"

#include <assert.h>
#include <stdlib.h>

#include "vd55g1_patch_cut_1.c"
#include "vd55g1_patch_cut_2.c"

#define VD55G1_REG_MODEL_ID                           0x0000
  #define VD55G1_MODEL_ID                             0x53354731
#define VD55G1_REG_REVISION                           0x0004
  #define VD55G1_REVISION_CUT_1                       0x1010
  #define VD55G1_REVISION_CUT_2                       0x2020
#define VD55G1_REG_ROM_REVISION                       0x0008
#define VD55G1_ERROR_CODE                             0x0010
#define VD55G1_REG_FWPATCH_REVISION                   0x0012
#define VD55G1_REG_SYSTEM_FSM                         0x001C
  #define VD55G1_SYSTEM_FSM_READY_TO_BOOT             0x01
  #define VD55G1_SYSTEM_FSM_SW_STBY                   0x02
  #define VD55G1_SYSTEM_FSM_STREAMING                 0x03
  #define VD55G1_SYSTEM_FSM_STREAMING_AWU             0x04

#define VD55G1_REG_BOOT                               0x0200
  #define VD55G1_CMD_ACK                              0
  #define VD55G1_BOOT_BOOT                            1
  #define VD55G1_BOOT_PATCH_AND_BOOT                  2
#define VD55G1_REG_STBY                               0x0201
  #define VD55G1_STBY_START_STREAM                    1
  #define VD55G1_STBY_THSENS_READ                     4
  #define VD55G1_STBY_START_AWU                       8
#define VD55G1_REG_STREAMING                          0x0202
  #define VD55G1_STREAMING_STOP_STREAM                1

#define VD55G1_REG_EXT_CLOCK                          0x0220
#define VD55G1_REG_MIPI_DATA_RATE                     0x0224

#define VD55G1_REG_LINE_LENGTH                        0x0300
#define VD55G1_REG_ORIENTATION                        0x0302
#define VD55G1_REG_PATGEN_CTRL                        0x0304
  #define VD55G1_PATGEN_CTRL_DISABLE                  0x0000
  #define VD55G1_PATGEN_CTRL_DIAG_GRAY                0x0221
  #define VD55G1_PATGEN_CTRL_PSN                      0x0281
#define VD55G1_REG_FORMAT_CTRL                        0x030a
#define VD55G1_REG_OIF_CTRL                           0x030c
#define VD55G1_REG_OIF_IMG_CTRL                       0x030f
#define VD55G1_REG_DARKCAL_CTRL                       0x032a
  #define VD55G1_DARKCAL_BYPASS                       0
  #define VD55G1_DARKCAL_BYPASS_DARKAVG               2
#define VD55G1_REG_AWU_CTRL                           0x036c
#define VD55G1_REG_AWU_DETECTION_THRESHOLD            0x0370
#define VD55G1_REG_MAX_COARSE_INTEGRATION_LINES       0x0372
#define VD55G1_REG_DUSTER_CTRL                        0x03ae
  #define VD55G1_DUSTER_DISABLE                       0
#define VD55G1_REG_CONTEXT_NEXT_CONTEXT               0x03e4
#define VD55G1_REG_EXPOSURE_COMPILER_CONTROL_A        0x0482
#define VD55G1_REG_AE_TARGET_PERCENTAGE               0x0486

#define VD55G1_REG_EXP_MODE                           0x0500
  #define VD55G1_EXP_MODE_AUTO                        0
  #define VD55G1_EXP_MODE_FREEZE                      1
  #define VD55G1_EXP_MODE_MANUAL                      2
#define VD55G1_REG_FRAME_LENGTH                       0x050c
#define VD55G1_REG_Y_START                            0x0510
#define VD55G1_REG_Y_HEIGHT                           0x0512
#define VD55G1_REG_X_START                            0x0514
#define VD55G1_REG_X_WIDTH                            0x0516
#define VD55G1_REG_GPIO_x(_i_)                        (0x051d + _i_)
  #define VD55G1_REG_GPIO_INPUT                       0x01
  #define VD55G1_REG_GPIO_AWU                         0x0d
#define VD55G1_REG_READOUT_CTRL                       0x052e

#define VD55G1_REG_EXP_MODE_3                         0x05f0
#define VD55G1_REG_FRAME_LENGTH_3                     0x05fc
#define VD55G1_REG_Y_START_3                          0x0600
#define VD55G1_REG_Y_HEIGHT_3                         0x0602
#define VD55G1_REG_X_START_3                          0x0604
#define VD55G1_REG_X_WIDTH_3                          0x0606
#define VD55G1_REG_GPIO_x_3(_i_)                      (0x060d + _i_)
#define VD55G1_REG_READOUT_CTRL_3                     0x061e
#define VD55G1_REG_MASK_FRAME_CTRL_3                  0x0627

#define VD55G1_DPHYTX_CTRL                            0x0936
  #define DBG_CONT_MODE_DISABLED                      0x00
  #define DBG_CONT_MODE_ENABLED                       0x10

#define VD55G1_REG_FWPATCH_START_ADDR                 0x2000

#define VD55G1_MIPI_DATA_RATE_HZ                      804000000
#define VD55G1_MIN_LINE_LEN_ADC_9                     978
#define VD55G1_MIN_LINE_LEN_ADC_10                    1128
#define VD55G1_MIPI_BPP                               8
#define VD55G1_MIPI_MARGIN                            900
#define VD55G1_MIN_VBLANK                             86

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

enum vd55g1_bin_mode {
  VD55G1_BIN_MODE_NORMAL,
  VD55G1_BIN_MODE_DIGITAL_X2,
  VD55G1_BIN_MODE_DIGITAL_X4,
};

enum {
  VD55G1_ST_IDLE,
  VD55G1_ST_STREAMING,
};

struct vd55g1_rect {
  int32_t left;
  int32_t top;
  uint32_t width;
  uint32_t height;
};

struct vd55g1_mode {
  uint32_t width;
  uint32_t height;
  enum vd55g1_bin_mode bin_mode;
  struct vd55g1_rect crop;
};

static const struct vd55g1_mode vd55g1_supported_modes[] = {
  {
    .width = VD55G1_MAX_WIDTH,
    .height = VD55G1_MAX_HEIGHT,
    .bin_mode = VD55G1_BIN_MODE_NORMAL,
    .crop = {
      .left = 0,
      .top = 0,
      .width = VD55G1_MAX_WIDTH,
      .height = VD55G1_MAX_HEIGHT,
    },
  },
  {
    .width = 800,
    .height = 600,
    .bin_mode = VD55G1_BIN_MODE_NORMAL,
    .crop = {
      .left = 2,
      .top = 52,
      .width = 800,
      .height = 600,
    },
  },
  {
    .width = 640,
    .height = 480,
    .bin_mode = VD55G1_BIN_MODE_NORMAL,
    .crop = {
      .left = 82,
      .top = 112,
      .width = 640,
      .height = 480,
    },
  },
  {
    .width = 320,
    .height = 240,
    .bin_mode = VD55G1_BIN_MODE_DIGITAL_X2,
    .crop = {
      .left = 82,
      .top = 112,
      .width = 640,
      .height = 480,
    },
  },
};

#define VD55G1_TraceError(_ctx_,_ret_) do { \
  if (_ret_) VD55G1_error(_ctx_, "Error on %s:%d : %d\n", __func__, __LINE__, _ret_); \
  if (_ret_) display_error(_ctx_); \
  if (_ret_) return _ret_; \
} while(0)

static const struct vd55g1_mode *VD55G1_Resolution2Mode(VD55G1_Res_t resolution)
{
  switch (resolution) {
  case VD55G1_RES_QVGA_320_240:
    return &vd55g1_supported_modes[3];
    break;
  case VD55G1_RES_VGA_640_480:
    return &vd55g1_supported_modes[2];
    break;
  case VD55G1_RES_SXGA_800_600:
    return &vd55g1_supported_modes[1];
    break;
  case VD55G1_RES_FULL_804_704:
    return &vd55g1_supported_modes[0];
    break;
  default:
    return NULL;
  }
}

static void VD55G1_log_impl(VD55G1_Ctx_t *ctx, int lvl, const char *format, ...)
{
  va_list ap;

  if (!ctx->log)
    return ;

  va_start(ap, format);
  ctx->log(ctx, lvl, format, ap);
  va_end(ap);
}

#define VD55G1_dbg(_ctx_, _lvl_, _fmt_, ...) do { \
  VD55G1_log_impl(_ctx_, VD55G1_LVL_DBG(_lvl_), "VD55G1_DG%d-%d : " _fmt_, _lvl_, __LINE__, ##__VA_ARGS__); \
} while(0)

#define VD55G1_notice(_ctx_, _fmt_, ...) do { \
  VD55G1_log_impl(_ctx_, VD55G1_LVL_NOTICE, "VD55G1_NOT-%d : " _fmt_, __LINE__, ##__VA_ARGS__); \
} while(0)

#define VD55G1_warn(_ctx_, _fmt_, ...) do { \
  VD55G1_log_impl(_ctx_, VD55G1_LVL_WARNING, "VD55G1_WRN-%d : " _fmt_, __LINE__, ##__VA_ARGS__); \
} while(0)

#define VD55G1_error(_ctx_, _fmt_, ...) do { \
  VD55G1_log_impl(_ctx_, VD55G1_LVL_ERROR, "VD55G1_ERR-%d : " _fmt_, __LINE__, ##__VA_ARGS__); \
} while(0)

static void display_error(VD55G1_Ctx_t *ctx)
{
  uint16_t reg16;
  int ret;

  ret = ctx->read16(ctx, VD55G1_ERROR_CODE, &reg16);
  assert(ret == 0);
  VD55G1_error(ctx, "ERROR_CODE : 0x%04x\n", reg16);
}

static int VD55G1_Copy8(VD55G1_Ctx_t *ctx, uint16_t dst, uint16_t src)
{
  uint8_t reg8;
  int ret;

  ret = ctx->read8(ctx, src, &reg8);
  VD55G1_TraceError(ctx, ret);
  ret = ctx->write8(ctx, dst, reg8);
  VD55G1_TraceError(ctx, ret);

  return 0;
}

static int VD55G1_Copy16(VD55G1_Ctx_t *ctx, uint16_t dst, uint16_t src)
{
  uint16_t reg16;
  int ret;

  ret = ctx->read16(ctx, src, &reg16);
  VD55G1_TraceError(ctx, ret);
  ret = ctx->write16(ctx, dst, reg16);
  VD55G1_TraceError(ctx, ret);

  return 0;
}

static int VD55G1_PollReg8(VD55G1_Ctx_t *ctx, uint16_t addr, uint8_t poll_val)
{
  const unsigned int loop_delay_ms = 10;
  const unsigned int timeout_ms = 500;
  int loop_nb = timeout_ms / loop_delay_ms;
  uint8_t val;
  int ret;

  while (--loop_nb) {
    ret = ctx->read8(ctx, addr, &val);
    if (ret < 0)
      return ret;
    if (val == poll_val)
      return 0;
    ctx->delay(ctx, loop_delay_ms);
  }

  VD55G1_dbg(ctx, 0, "current state %d\n", val);

  return -1;
}

static int VD55G1_IsStreaming(VD55G1_Ctx_t *ctx)
{
  uint8_t state;
  int ret;

  ret = ctx->read8(ctx, VD55G1_REG_SYSTEM_FSM, &state);
  if (ret)
    return ret;

  return state == VD55G1_SYSTEM_FSM_STREAMING;
}

static int VD55G1_WaitState(VD55G1_Ctx_t *ctx, int state)
{
  int ret = VD55G1_PollReg8(ctx, VD55G1_REG_SYSTEM_FSM, state);

  if (ret)
    VD55G1_warn(ctx, "Unable to reach state %d\n", state);
  else
    VD55G1_dbg(ctx, 0, "reach state %d\n", state);

  return ret;
}

static int VD55G1_CheckModelId(VD55G1_Ctx_t *ctx)
{
  struct drv_ctx *drv_ctx = &ctx->ctx;
  uint32_t reg32;
  uint16_t reg16;
  int ret;

  ret = ctx->read32(ctx, VD55G1_REG_MODEL_ID, &reg32);
  VD55G1_TraceError(ctx, ret);
  VD55G1_dbg(ctx, 0, "model_id = 0x%04x\n", reg32);
  if (reg32 != VD55G1_MODEL_ID) {
    VD55G1_error(ctx, "Bad model id expected 0x%04x / got 0x%04x\n", VD55G1_MODEL_ID, reg32);
    return -1;
  }

  ret = ctx->read16(ctx, VD55G1_REG_REVISION, &reg16);
  VD55G1_TraceError(ctx, ret);
  VD55G1_dbg(ctx, 0, "revision = 0x%04x\n", reg16);
  switch (reg16) {
  case VD55G1_REVISION_CUT_1:
    drv_ctx->cut_version = VD55G1_REVISION_CUT_1;
    break;
  case VD55G1_REVISION_CUT_2:
    drv_ctx->cut_version = VD55G1_REVISION_CUT_2;
    break;
  default:
    VD55G1_error(ctx, "Unsupported revision0x%04x\n", reg16);
    return -1;
  }

  ret = ctx->read32(ctx, VD55G1_REG_ROM_REVISION, &reg32);
  VD55G1_TraceError(ctx, ret);
  VD55G1_dbg(ctx, 0, "rom = 0x%04x\n", reg32);

  return 0;
}

static int VD55G1_ApplyPatchCommon(VD55G1_Ctx_t *ctx, uint8_t *patch_array, int patch_len, uint8_t patch_major,
                                   uint8_t patch_minor)
{
  uint16_t reg16;
  int ret;

  ret = ctx->write_array(ctx, VD55G1_REG_FWPATCH_START_ADDR, patch_array, patch_len);
  VD55G1_TraceError(ctx, ret);

  ret = ctx->write8(ctx, VD55G1_REG_BOOT, VD55G1_BOOT_PATCH_AND_BOOT);
  VD55G1_TraceError(ctx, ret);

  ret = VD55G1_PollReg8(ctx, VD55G1_REG_BOOT, VD55G1_CMD_ACK);
  VD55G1_TraceError(ctx, ret);

  ret = ctx->read16(ctx, VD55G1_REG_FWPATCH_REVISION, &reg16);
  VD55G1_TraceError(ctx, ret);
  VD55G1_dbg(ctx, 0, "patch = 0x%04x\n", reg16);

  if (reg16 != (patch_major << 8) + patch_minor) {
    VD55G1_error(ctx, "bad patch version expected %d.%d got %d.%d\n", patch_major, patch_minor, reg16 >> 8, reg16 & 0xff);
    return -1;
  }
  VD55G1_notice(ctx, "patch %d.%d applied\n", reg16 >> 8, reg16 & 0xff);

  ret = VD55G1_WaitState(ctx, VD55G1_SYSTEM_FSM_SW_STBY);
  VD55G1_TraceError(ctx, ret);

  VD55G1_notice(ctx, "sensor boot successfully\n");

  return 0;
}

static int VD55G1_ApplyPatch(VD55G1_Ctx_t *ctx)
{
  struct drv_ctx *drv_ctx = &ctx->ctx;

  switch (drv_ctx->cut_version) {
  case VD55G1_REVISION_CUT_1:
    return VD55G1_ApplyPatchCommon(ctx, (uint8_t *) patch_array_cut_1, sizeof(patch_array_cut_1),
                                   VD55G1_FWPATCH_REVISION_MAJOR_CUT_1, VD55G1_FWPATCH_REVISION_MINOR_CUT_1);
  case VD55G1_REVISION_CUT_2:
    return VD55G1_ApplyPatchCommon(ctx, (uint8_t *) patch_array_cut_2, sizeof(patch_array_cut_2),
                                   VD55G1_FWPATCH_REVISION_MAJOR_CUT_2, VD55G1_FWPATCH_REVISION_MINOR_CUT_2);
  default:
    assert(0);
  }

  return -1;
}

static int VD55G1_Gpios(VD55G1_Ctx_t *ctx)
 {
  struct drv_ctx *drv_ctx = &ctx->ctx;
  int ret;
  int i;

  for (i = 0 ; i < VD55G1_GPIO_NB; i++)
  {
    ret = ctx->write8(ctx, VD55G1_REG_GPIO_x(i), drv_ctx->config_save.gpio_ctrl[i]);
    VD55G1_TraceError(ctx, ret);
  }

  return 0;
}

static int VD55G1_Boot(VD55G1_Ctx_t *ctx)
{
  int ret;

  ret = VD55G1_WaitState(ctx, VD55G1_SYSTEM_FSM_READY_TO_BOOT);
  if (ret)
    return ret;

  ret = VD55G1_CheckModelId(ctx);
  if (ret)
    return ret;

  ret = VD55G1_ApplyPatch(ctx);
  if (ret)
    return ret;

  ret = VD55G1_Gpios(ctx);
  if (ret)
    return ret;

  return 0;
}

static uint32_t VD55G1_GetSystemClock(VD55G1_Ctx_t *ctx)
{
  uint32_t mipi_data_rate;
  int ret;

  ret = ctx->read32(ctx, VD55G1_REG_MIPI_DATA_RATE, &mipi_data_rate);
  if (ret)
    return 0;

  if (mipi_data_rate <= 1200000000 && mipi_data_rate > 600000000)
    return mipi_data_rate;
  else if (mipi_data_rate <= 600000000 && mipi_data_rate > 300000000)
    return mipi_data_rate * 2;
  else if (mipi_data_rate <= 300000000 && mipi_data_rate >= 250000000)
    return mipi_data_rate * 4;

  return 0;
}

static uint32_t VD55G1_GetPixelClock(VD55G1_Ctx_t *ctx)
{
  uint32_t system_clk;

  system_clk = VD55G1_GetSystemClock(ctx);
  if (!system_clk)
    return 0;

  if (system_clk <= 1200000000 && system_clk > 900000000)
    return system_clk / 8;
  else if (system_clk <= 900000000 && system_clk > 780000000)
    return system_clk / 6;
  else if (system_clk <= 780000000 && system_clk >= 600000000)
    return system_clk / 5;

  return 0;
}

static int VD55G1_SetupClocks(VD55G1_Ctx_t *ctx)
{
  struct drv_ctx *drv_ctx = &ctx->ctx;
  int ret;

  if (drv_ctx->config_save.out_itf.data_rate_in_mps < VD55G1_MIN_DATARATE ||
      drv_ctx->config_save.out_itf.data_rate_in_mps > VD55G1_MAX_DATARATE)
    return -1;

  ret = ctx->write32(ctx, VD55G1_REG_EXT_CLOCK, drv_ctx->config_save.ext_clock_freq_in_hz);
  VD55G1_TraceError(ctx, ret);

  ret = ctx->write32(ctx, VD55G1_REG_MIPI_DATA_RATE, drv_ctx->config_save.out_itf.data_rate_in_mps);
  VD55G1_TraceError(ctx, ret);

  drv_ctx->pclk = VD55G1_GetPixelClock(ctx);
  if (!drv_ctx->pclk)
    return -1;

  return 0;
}

static int VD55G1_SetupOutput(VD55G1_Ctx_t *ctx)
{
  VD55G1_OutItf_Config_t *out_itf = &ctx->ctx.config_save.out_itf;
  uint16_t oif_ctrl;
  int ret;

  /* Be sure we got value 0 or 1 */
  out_itf->clock_lane_swap_enable = !!out_itf->clock_lane_swap_enable;
  out_itf->data_lane_swap_enable = !!out_itf->data_lane_swap_enable;

  /* raw8 */
  ret = ctx->write8(ctx, VD55G1_REG_FORMAT_CTRL, 8);
  VD55G1_TraceError(ctx, ret);

  /* csi lanes */
  oif_ctrl = out_itf->data_lane_swap_enable << 6 |
             out_itf->clock_lane_swap_enable << 3;
  ret = ctx->write16(ctx, VD55G1_REG_OIF_CTRL, oif_ctrl);
  VD55G1_TraceError(ctx, ret);

  /* data type */
  ret = ctx->write8(ctx, VD55G1_REG_OIF_IMG_CTRL, 0x2a);
  VD55G1_TraceError(ctx, ret);

  return 0;
}

static int VD55G1_SetupSize(VD55G1_Ctx_t *ctx)
{
  struct drv_ctx *drv_ctx = &ctx->ctx;
  const struct vd55g1_mode *mode;
  int ret;

  mode = VD55G1_Resolution2Mode(drv_ctx->config_save.resolution);
  if (!mode)
    return -1;

  ret = ctx->write8(ctx, VD55G1_REG_READOUT_CTRL, mode->bin_mode);
  VD55G1_TraceError(ctx, ret);

  ret = ctx->write16(ctx, VD55G1_REG_X_START, mode->crop.left);
  VD55G1_TraceError(ctx, ret);
  ret = ctx->write16(ctx, VD55G1_REG_X_WIDTH, mode->crop.width);
  VD55G1_TraceError(ctx, ret);
  ret = ctx->write16(ctx, VD55G1_REG_Y_START, mode->crop.top);
  VD55G1_TraceError(ctx, ret);
  ret = ctx->write16(ctx, VD55G1_REG_Y_HEIGHT, mode->crop.height);
  VD55G1_TraceError(ctx, ret);

  return 0;
}

static int VD55G1_SetupLineLen(VD55G1_Ctx_t *ctx)
{
  struct drv_ctx *drv_ctx = &ctx->ctx;
  int min_line_len_mipi;
  uint16_t line_len;
  uint16_t width;
  int ret;

  ret = ctx->read16(ctx, VD55G1_REG_X_WIDTH, &width);
  VD55G1_TraceError(ctx, ret);

  min_line_len_mipi = ((width * VD55G1_MIPI_BPP + VD55G1_MIPI_MARGIN) * (uint64_t)drv_ctx->pclk)
                      / VD55G1_MIPI_DATA_RATE_HZ;
  line_len = MAX(VD55G1_MIN_LINE_LEN_ADC_10, min_line_len_mipi);

  ret = ctx->write16(ctx, VD55G1_REG_LINE_LENGTH, line_len);
  VD55G1_TraceError(ctx, ret);
  VD55G1_dbg(ctx, 1, "line_length = %d\n", line_len);

  return 0;
}

static int VD55G1_ComputeFrameLength(VD55G1_Ctx_t *ctx, int fps, uint16_t *frame_length)
{
  struct drv_ctx *drv_ctx = &ctx->ctx;
  int min_frame_length;
  int req_frame_length;
  uint16_t line_length;
  uint16_t height;
  int ret;

  ret = ctx->read16(ctx, VD55G1_REG_LINE_LENGTH, &line_length);
  VD55G1_TraceError(ctx, ret);

  ret = ctx->read16(ctx, VD55G1_REG_Y_HEIGHT, &height);
  VD55G1_TraceError(ctx, ret);

  min_frame_length = height + VD55G1_MIN_VBLANK;
  req_frame_length = drv_ctx->pclk / (line_length * fps);
  *frame_length = MIN(MAX(min_frame_length, req_frame_length), 65535);

  VD55G1_dbg(ctx, 1, "frame_length to MAX(%d, %d) = %d to reach %d fps\n", min_frame_length, req_frame_length,
             *frame_length, fps);

  return 0;
}

static int VD55G1_SetupFrameRate(VD55G1_Ctx_t *ctx)
{
  struct drv_ctx *drv_ctx = &ctx->ctx;
  uint16_t frame_length;
  int ret;

  ret = VD55G1_SetupLineLen(ctx);
  if (ret)
    return ret;

  ret = VD55G1_ComputeFrameLength(ctx, drv_ctx->config_save.frame_rate, &frame_length);
  if (ret)
    return ret;

  VD55G1_dbg(ctx, 1, "Set frame_length to %d to reach %d fps\n", frame_length, drv_ctx->config_save.frame_rate);
  ret = ctx->write16(ctx, VD55G1_REG_FRAME_LENGTH, frame_length);
  VD55G1_TraceError(ctx, ret);

  return 0;
}

static int VD55G1_SetupExposure(VD55G1_Ctx_t *ctx)
{
  VD55G1_AWUConfig_t *awu = &ctx->ctx.config_save.awu;
  struct drv_ctx *drv_ctx = &ctx->ctx;
  uint16_t frame_length;
  int max_fps;
  uint8_t reg;
  int ret;

  /* max integration lines */
   /* first get minimum frame len */
  max_fps = drv_ctx->config_save.frame_rate;
  if (awu->is_enable)
  {
    max_fps = MAX(max_fps, awu->convergence_frame_rate);
    max_fps = MAX(max_fps, awu->awu_frame_rate);
  }
  ret = VD55G1_ComputeFrameLength(ctx, max_fps, &frame_length);
  if (ret)
    return ret;
   /* set max integration lines to this value minus 10 lines */
  ret = ctx->write16(ctx, VD55G1_REG_MAX_COARSE_INTEGRATION_LINES, frame_length - 10);
  VD55G1_TraceError(ctx, ret);
  VD55G1_dbg(ctx, 1, "Max coarse lines = %d\n", frame_length - 10);

  /* turn on auto exposure except when patgen is active */
  reg = drv_ctx->config_save.patgen != VD55G1_PATGEN_CTRL_DISABLE ? VD55G1_EXP_MODE_MANUAL : VD55G1_EXP_MODE_AUTO;
  ret = ctx->write8(ctx, VD55G1_REG_EXP_MODE, reg);
  VD55G1_TraceError(ctx, ret);

  return 0;
}

static int VD55G1_SetupMirrorFlip(VD55G1_Ctx_t *ctx)
{
  struct drv_ctx *drv_ctx = &ctx->ctx;
  uint8_t mode;
  int ret;

  switch (drv_ctx->config_save.flip_mirror_mode) {
  case VD55G1_MIRROR_FLIP_NONE:
    mode = 0;
    break;
  case VD55G1_FLIP:
    mode = 2;
    break;
  case VD55G1_MIRROR:
    mode = 1;
    break;
  case VD55G1_MIRROR_FLIP:
    mode = 3;
    break;
  default:
    return -1;
  }

  ret = ctx->write8(ctx, VD55G1_REG_ORIENTATION, mode);
  VD55G1_TraceError(ctx, ret);

  return 0;
}

static int VD55G1_SetupPatGen(VD55G1_Ctx_t *ctx)
{
  struct drv_ctx *drv_ctx = &ctx->ctx;
  uint16_t value = VD55G1_PATGEN_CTRL_DISABLE;
  int ret;

  switch (drv_ctx->config_save.patgen) {
  case VD55G1_PATGEN_DISABLE:
    value = VD55G1_PATGEN_CTRL_DISABLE;
    break;
  case VD55G1_PATGEN_DIAGONAL_GRAYSCALE:
    value = VD55G1_PATGEN_CTRL_DIAG_GRAY;
    break;
  case VD55G1_PATGEN_PSEUDO_RANDOM:
    value = VD55G1_PATGEN_CTRL_PSN;
    break;
  default:
    return -1;
  }

  if (drv_ctx->config_save.patgen != VD55G1_PATGEN_CTRL_DISABLE)
  {
    ret = ctx->write8(ctx, VD55G1_REG_DUSTER_CTRL, VD55G1_DUSTER_DISABLE);
    VD55G1_TraceError(ctx, ret);
    ret = ctx->write8(ctx, VD55G1_REG_DARKCAL_CTRL, VD55G1_DARKCAL_BYPASS_DARKAVG);
    VD55G1_TraceError(ctx, ret);
  }

  ret = ctx->write16(ctx, VD55G1_REG_PATGEN_CTRL, value);
  VD55G1_TraceError(ctx, ret);

  return 0;
}

static int VD55G1_SetFlicker(VD55G1_Ctx_t *ctx, VD55G1_Flicker_t flicker)
{
  uint16_t mode;
  int ret;

  switch (flicker) {
  case VD55G1_FLICKER_FREE_NONE:
    mode = 0;
    break;
  case VD55G1_FLICKER_FREE_50HZ:
    mode = 1;
    break;
  case VD55G1_FLICKER_FREE_60HZ:
    mode = 3;
    break;
  default:
    return -1;
  }

  ret = ctx->write16(ctx, VD55G1_REG_EXPOSURE_COMPILER_CONTROL_A, mode);
  VD55G1_TraceError(ctx, ret);

  return 0;
}

static int VD55G1_Flicker(VD55G1_Ctx_t *ctx)
{
  struct drv_ctx *drv_ctx = &ctx->ctx;

  return VD55G1_SetFlicker(ctx, drv_ctx->config_save.flicker);
}

static int VD55G1_Setup(VD55G1_Ctx_t *ctx)
{
  int ret;

  ret = VD55G1_SetupClocks(ctx);
  VD55G1_TraceError(ctx, ret);

  ret = VD55G1_SetupOutput(ctx);
  VD55G1_TraceError(ctx, ret);

  ret = VD55G1_SetupSize(ctx);
  VD55G1_TraceError(ctx, ret);

  ret = VD55G1_SetupFrameRate(ctx);
  VD55G1_TraceError(ctx, ret);

  ret = VD55G1_SetupExposure(ctx);
  VD55G1_TraceError(ctx, ret);

  ret = VD55G1_SetupMirrorFlip(ctx);
  VD55G1_TraceError(ctx, ret);

  ret = VD55G1_SetupPatGen(ctx);
  VD55G1_TraceError(ctx, ret);

  ret = VD55G1_Flicker(ctx);
  VD55G1_TraceError(ctx, ret);

  return 0;
}

static int VD55G1_StartStreaming(VD55G1_Ctx_t *ctx)
{
  int ret;

  ret = ctx->write8(ctx, VD55G1_REG_STBY, VD55G1_STBY_START_STREAM);
  VD55G1_TraceError(ctx, ret);

  ret = VD55G1_PollReg8(ctx, VD55G1_REG_STBY, VD55G1_CMD_ACK);
  VD55G1_TraceError(ctx, ret);

  ret = VD55G1_WaitState(ctx, VD55G1_SYSTEM_FSM_STREAMING);
  VD55G1_TraceError(ctx, ret);

  VD55G1_notice(ctx, "Streaming is on\n");

  return 0;
}

static int VD55G1_StopStreaming(VD55G1_Ctx_t *ctx)
{
  int ret;

  ret = ctx->write8(ctx, VD55G1_REG_STREAMING, VD55G1_STREAMING_STOP_STREAM);
  VD55G1_TraceError(ctx, ret);

  ret = VD55G1_PollReg8(ctx, VD55G1_REG_STREAMING, VD55G1_CMD_ACK);
  VD55G1_TraceError(ctx, ret);

  ret = VD55G1_WaitState(ctx, VD55G1_SYSTEM_FSM_SW_STBY);
  VD55G1_TraceError(ctx, ret);

  VD55G1_notice(ctx, "Streaming is off\n");

  return 0;
}

static int VD55G1_StartAWU(VD55G1_Ctx_t *ctx)
{
  int ret;

  ret = ctx->write8(ctx, VD55G1_REG_STBY, VD55G1_STBY_START_AWU);
  VD55G1_TraceError(ctx, ret);

  ret = VD55G1_PollReg8(ctx, VD55G1_REG_STBY, VD55G1_CMD_ACK);
  VD55G1_TraceError(ctx, ret);

  ret = VD55G1_WaitState(ctx, VD55G1_SYSTEM_FSM_STREAMING_AWU);
  VD55G1_TraceError(ctx, ret);

  VD55G1_notice(ctx, "Awu is on\n");

  return 0;
}

static int VD55G1_AutoWakeCopyCtx(VD55G1_Ctx_t *ctx)
{
  int ret;

  ret = VD55G1_Copy8(ctx, VD55G1_REG_EXP_MODE_3, VD55G1_REG_EXP_MODE);
  if (ret)
    return ret;
  ret = VD55G1_Copy16(ctx, VD55G1_REG_FRAME_LENGTH_3, VD55G1_REG_FRAME_LENGTH);
  if (ret)
    return ret;
  ret = VD55G1_Copy16(ctx, VD55G1_REG_Y_START_3, VD55G1_REG_Y_START);
  if (ret)
    return ret;
  ret = VD55G1_Copy16(ctx, VD55G1_REG_Y_HEIGHT_3, VD55G1_REG_Y_HEIGHT);
  if (ret)
    return ret;
  ret = VD55G1_Copy16(ctx, VD55G1_REG_X_START_3, VD55G1_REG_X_START);
  if (ret)
    return ret;
  ret = VD55G1_Copy16(ctx, VD55G1_REG_X_WIDTH_3, VD55G1_REG_X_WIDTH);
  if (ret)
    return ret;
  ret = VD55G1_Copy8(ctx, VD55G1_REG_READOUT_CTRL_3, VD55G1_REG_READOUT_CTRL);
  if (ret)
    return ret;

  return 0;
}

static int VD55G1_AutoWakeSetupGpio(VD55G1_Ctx_t *ctx)
{
  struct drv_ctx *drv_ctx = &ctx->ctx;
  int ret;
  int i;

  for (i = 0 ; i < VD55G1_GPIO_NB; i++)
  {
    ret = ctx->write8(ctx, VD55G1_REG_GPIO_x_3(i), drv_ctx->config_save.gpio_ctrl[i]);
    VD55G1_TraceError(ctx, ret);
  }

  return 0;
}

static int VD55G1_AutoWakeNextContextInvalid(VD55G1_Ctx_t *ctx)
{
  uint16_t reg16;
  int ret;

  ret = ctx->read16(ctx, VD55G1_REG_CONTEXT_NEXT_CONTEXT, &reg16);
  VD55G1_TraceError(ctx, ret);

  reg16 |= 0xf000;
  ret = ctx->write16(ctx, VD55G1_REG_CONTEXT_NEXT_CONTEXT, reg16);
  VD55G1_TraceError(ctx, ret);

  return 0;
}

static int VD55G1_AutoWakeFineTune(VD55G1_Ctx_t *ctx)
{
  VD55G1_AWUConfig_t *awu = &ctx->ctx.config_save.awu;
  uint16_t frame_length;
  uint32_t reg32;
  int ret;

  /* setup fps for convergence state */
  ret = VD55G1_ComputeFrameLength(ctx, awu->convergence_frame_rate, &frame_length);
  if (ret)
    return ret;

  ret = ctx->read32(ctx, VD55G1_REG_AWU_CTRL, &reg32);
  VD55G1_TraceError(ctx, ret);

  reg32 &= ~0xfffff800;
  reg32 |= (frame_length << 16) |
           (awu->zone_nb << 11);
  ret = ctx->write32(ctx, VD55G1_REG_AWU_CTRL, reg32);
  VD55G1_TraceError(ctx, ret);

  /* and fps for awu mode */
  ret = VD55G1_ComputeFrameLength(ctx, awu->awu_frame_rate, &frame_length);
  if (ret)
    return ret;

  ret = ctx->write32(ctx, VD55G1_REG_FRAME_LENGTH_3, frame_length);
  VD55G1_TraceError(ctx, ret);

  /* threshold */
  if (awu->threshold != VD55G1_AWU_THRESHOLD_DEFAULT) {
    ret = ctx->write16(ctx, VD55G1_REG_AWU_DETECTION_THRESHOLD, (awu->threshold << 8));
    VD55G1_TraceError(ctx, ret);
  }

  /* disable frame output */
  ret = ctx->write8(ctx, VD55G1_REG_MASK_FRAME_CTRL_3, 1);
  VD55G1_TraceError(ctx, ret);

  return 0;
}

static int VD55G1_AutoWakeConfigure(VD55G1_Ctx_t *ctx)
{
  int ret;

  ret = VD55G1_AutoWakeNextContextInvalid(ctx);
  if (ret)
    return ret;

  ret = VD55G1_AutoWakeFineTune(ctx);
  if (ret)
    return ret;

  ret = VD55G1_AutoWakeSetupGpio(ctx);
  if (ret)
    return ret;

  return 0;
}

static int VD55G1_AWUStreaming(VD55G1_Ctx_t *ctx)
{
  struct drv_ctx *drv_ctx = &ctx->ctx;
  int ret;

  if (drv_ctx->state == VD55G1_ST_STREAMING) {
    ret = VD55G1_StopStreaming(ctx);
    if (ret)
      return ret;
    drv_ctx->state = VD55G1_ST_IDLE;
  }

  return VD55G1_StartAWU(ctx);
}

int VD55G1_Init(VD55G1_Ctx_t *ctx, VD55G1_Config_t *config)
{
  VD55G1_AWUConfig_t *awu = &config->awu;
  struct drv_ctx *drv_ctx = &ctx->ctx;
  int ret;

  if (config->frame_rate < VD55G1_MIN_FPS)
    return -1;
  if (config->frame_rate > VD55G1_MAX_FPS)
    return -1;

  if ((config->resolution != VD55G1_RES_QVGA_320_240) &&
      (config->resolution != VD55G1_RES_VGA_640_480) &&
      (config->resolution != VD55G1_RES_SXGA_800_600) &&
      (config->resolution != VD55G1_RES_FULL_804_704)) {
    return -1;
  }

  if (awu->is_enable && awu->threshold != VD55G1_AWU_THRESHOLD_DEFAULT) {
    if (awu->threshold < VD55G1_AWU_THRESHOLD_MIN)
      return -1;
    if (awu->threshold > VD55G1_AWU_THRESHOLD_MAX)
      return -1;
  }

  drv_ctx->config_save = *config;

  ctx->shutdown_pin(ctx, 0);
  ctx->delay(ctx, 10);
  ctx->shutdown_pin(ctx, 1);
  ctx->delay(ctx, 10);

  ret = VD55G1_Boot(ctx);
  if (ret)
    return ret;

  drv_ctx->state = VD55G1_ST_IDLE;

  return 0;
}

int VD55G1_DeInit(VD55G1_Ctx_t *ctx)
{
  struct drv_ctx *drv_ctx = &ctx->ctx;

  if (drv_ctx->state == VD55G1_ST_STREAMING)
    return -1;

  ctx->shutdown_pin(ctx, 0);
  ctx->delay(ctx, 10);

  return 0;
}

int VD55G1_Start(VD55G1_Ctx_t *ctx)
{
  struct drv_ctx *drv_ctx = &ctx->ctx;
  int ret;

  ret = VD55G1_Setup(ctx);
  if (ret)
    return ret;

  ret = VD55G1_StartStreaming(ctx);
  if (ret)
    return ret;
  drv_ctx->state = VD55G1_ST_STREAMING;

  return 0;
}

int VD55G1_Stop(VD55G1_Ctx_t *ctx)
{
  struct drv_ctx *drv_ctx = &ctx->ctx;
  int ret;

  ret = VD55G1_StopStreaming(ctx);
  if (ret)
    return ret;
  drv_ctx->state = VD55G1_ST_IDLE;

  return 0;
}

int VD55G1_StartAutoWakeUp(VD55G1_Ctx_t *ctx)
{
  int ret;

  if (!ctx->ctx.config_save.awu.is_enable)
    return -1;

  ret = VD55G1_AutoWakeCopyCtx(ctx);
  if (ret)
    return ret;

  ret = VD55G1_AutoWakeConfigure(ctx);
  if (ret)
    return ret;

  ret = VD55G1_AWUStreaming(ctx);
  if (ret)
    return ret;

  return 0;
}

int VD55G1_SetFlipMirrorMode(VD55G1_Ctx_t *ctx, VD55G1_MirrorFlip_t mode)
{
  struct drv_ctx *drv_ctx = &ctx->ctx;
  int is_streaming;
  int ret;

  is_streaming = VD55G1_IsStreaming(ctx);
  if (is_streaming < 0)
    return is_streaming;

  if (is_streaming) {
    ret = VD55G1_Stop(ctx);
    if (ret)
      return ret;
  }

  drv_ctx->config_save.flip_mirror_mode = mode;

  if (is_streaming) {
    ret = VD55G1_Start(ctx);
    if (ret)
      return ret;
  }

  return 0;
}

int VD55G1_GetBrightnessLevel(VD55G1_Ctx_t *ctx, int *level)
{
  uint16_t value;
  int ret;

  ret = ctx->read16(ctx, VD55G1_REG_AE_TARGET_PERCENTAGE, &value);
  VD55G1_TraceError(ctx, ret);
  *level = value;

  return 0;
}

int VD55G1_SetBrightnessLevel(VD55G1_Ctx_t *ctx, int level)
{
  uint16_t value = level;
  int ret;

  if (level < VD55G1_MIN_BRIGHTNESS || level > VD55G1_MAX_BRIGHTNESS)
    return -1;

  ret = ctx->write16(ctx, VD55G1_REG_AE_TARGET_PERCENTAGE, value);
  VD55G1_TraceError(ctx, ret);

  return 0;
}

int VD55G1_SetFlickerMode(VD55G1_Ctx_t *ctx, VD55G1_Flicker_t mode)
{
  struct drv_ctx *drv_ctx = &ctx->ctx;
  int ret;

  ret = VD55G1_SetFlicker(ctx, mode);
  if (ret)
    return ret;

  drv_ctx->config_save.flicker = mode;

  return 0;
}
