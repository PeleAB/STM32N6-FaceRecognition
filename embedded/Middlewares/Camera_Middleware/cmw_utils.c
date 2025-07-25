 /**
 ******************************************************************************
 * @file    cmw_utils.c
 * @author  GPM Application Team
 *
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

#include "cmw_utils.h"

#include <assert.h>

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

static void CMW_UTILS_get_crop_config(uint32_t cam_width, uint32_t cam_height, uint32_t pipe_width,
                                      uint32_t pipe_height, DCMIPP_CropConfTypeDef *crop);
static void CMW_UTILS_get_crop_config_from_manual(CMW_Manual_roi_area_t *conf, DCMIPP_CropConfTypeDef *crop);
static void CMW_UTILS_get_down_config(float ratio_width, float ratio_height, int width, int height,
                                      DCMIPP_DownsizeTypeDef *down);
static uint32_t CMW_UTILS_get_dec_ratio_and_update(float *ratio, int is_vertical);
static void CMW_UTILS_get_scale_configs(CMW_DCMIPP_Conf_t *p_conf, float ratio_width, float ratio_height,
                                       DCMIPP_DecimationConfTypeDef *dec, DCMIPP_DownsizeTypeDef *down);

void CMW_UTILS_GetPipeConfig(uint32_t cam_width, uint32_t cam_height, CMW_DCMIPP_Conf_t *p_conf,
                             DCMIPP_CropConfTypeDef *crop, DCMIPP_DecimationConfTypeDef *dec,
                             DCMIPP_DownsizeTypeDef *down)
{
  float ratio_height = 0;
  float ratio_width = 0;

  if (p_conf->mode == CMW_Aspect_ratio_crop)
  {
    CMW_UTILS_get_crop_config(cam_width, cam_height, p_conf->output_width, p_conf->output_height, crop);
    ratio_width = (float)crop->HSize / p_conf->output_width;
    ratio_height = (float)crop->VSize / p_conf->output_height;
  }
  else if (p_conf->mode == CMW_Aspect_ratio_fit)
  {
    ratio_width = (float)cam_width / p_conf->output_width;
    ratio_height = (float)cam_height / p_conf->output_height;
  }
  else if (p_conf->mode == CMW_Aspect_ratio_fullscreen)
  {
    ratio_height = (float) cam_height / p_conf->output_height;
    ratio_width = (float) ratio_height;
  }
  else
  {
    CMW_UTILS_get_crop_config_from_manual(&p_conf->manual_conf, crop);
    ratio_width = (float)crop->HSize / p_conf->output_width;
    ratio_height = (float)crop->VSize / p_conf->output_height;
  }

  CMW_UTILS_get_scale_configs(p_conf, ratio_width, ratio_height, dec, down);
}

static void CMW_UTILS_get_crop_config(uint32_t cam_width, uint32_t cam_height, uint32_t pipe_width, uint32_t pipe_height, DCMIPP_CropConfTypeDef *crop)
{
  const float ratio_width = (float)cam_width / pipe_width ;
  const float ratio_height = (float)cam_height / pipe_height;
  const float ratio = MIN(ratio_width, ratio_height);

  assert(ratio >= 1);
  assert(ratio < 64);

  crop->HSize = (uint32_t) MIN(pipe_width * ratio, cam_width);
  crop->VSize = (uint32_t) MIN(pipe_height * ratio, cam_height);
  crop->HStart = (cam_width - crop->HSize + 1) / 2;
  crop->VStart = (cam_height - crop->VSize + 1) / 2;
  crop->PipeArea = DCMIPP_POSITIVE_AREA;
}

static void CMW_UTILS_get_crop_config_from_manual(CMW_Manual_roi_area_t *roi, DCMIPP_CropConfTypeDef *crop)
{
  crop->HSize = roi->width;
  crop->VSize = roi->height;
  crop->HStart = roi->offset_x;
  crop->VStart = roi->offset_y;
}

static void CMW_UTILS_get_down_config(float ratio_width, float ratio_height, int width, int height, DCMIPP_DownsizeTypeDef *down)
{
  down->HRatio = (uint32_t) (8192 * ratio_width);
  down->VRatio = (uint32_t) (8192 * ratio_height);
  down->HDivFactor = (1024 * 8192 - 1) / down->HRatio;
  down->VDivFactor = (1024 * 8192 - 1) / down->VRatio;
  down->HSize = width;
  down->VSize = height;
}

static uint32_t CMW_UTILS_get_dec_ratio_from_decimal_ratio(int dec_ratio, int is_vertical)
{
  switch (dec_ratio) {
  case 1:
    return is_vertical ? DCMIPP_VDEC_ALL : DCMIPP_HDEC_ALL;
  case 2:
    return is_vertical ? DCMIPP_VDEC_1_OUT_2 : DCMIPP_HDEC_1_OUT_2;
  case 4:
    return is_vertical ? DCMIPP_VDEC_1_OUT_4 : DCMIPP_HDEC_1_OUT_4;
  case 8:
    return is_vertical ? DCMIPP_VDEC_1_OUT_8 : DCMIPP_HDEC_1_OUT_8;
  default:
    assert(0);
  }

  return is_vertical ? DCMIPP_VDEC_ALL : DCMIPP_HDEC_ALL;
}

static uint32_t CMW_UTILS_get_dec_ratio_and_update(float *ratio, int is_vertical)
{
  int dec_ratio = 1;

  while (*ratio >= 8) {
    dec_ratio *= 2;
    *ratio /= 2;
  }

  return CMW_UTILS_get_dec_ratio_from_decimal_ratio(dec_ratio, is_vertical);
}

static void CMW_UTILS_get_scale_configs(CMW_DCMIPP_Conf_t *p_conf, float ratio_width, float ratio_height,
                                       DCMIPP_DecimationConfTypeDef *dec, DCMIPP_DownsizeTypeDef *down)
{
  dec->HRatio = CMW_UTILS_get_dec_ratio_and_update(&ratio_width, 0);
  dec->VRatio = CMW_UTILS_get_dec_ratio_and_update(&ratio_height, 1);
  CMW_UTILS_get_down_config(ratio_width, ratio_height, p_conf->output_width, p_conf->output_height, down);
}

