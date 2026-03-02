/**
 ******************************************************************************
 * @file    app.c
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

#include "app/app.h"

#include <assert.h>
#include <stdio.h>

#include "app/app_config.h"
#include "app/app_pipeline.h"
#include "svc/app_display.h"
#include "svc/app_stats.h"
#include "stm32n6570_discovery.h"

void app_run(void)
{
  UVCL_Conf_t uvcl_conf = { 0 };
  ENC_Conf_t enc_conf = { 0 };
  int ret;

  printf("Init application\n");
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

  ret = BSP_PB_Init(BUTTON_USER1, BUTTON_MODE_GPIO);
  assert(ret == BSP_ERROR_NONE);

  app_stats_init();
  app_display_init();
  app_pipeline_init();

  CAM_Init();

  enc_conf.width = VENC_WIDTH;
  enc_conf.height = VENC_HEIGHT;
  enc_conf.fps = CAMERA_FPS;

  uvcl_conf.streams[0].width = VENC_WIDTH;
  uvcl_conf.streams[0].height = VENC_HEIGHT;
  uvcl_conf.streams[0].fps = CAMERA_FPS;
  uvcl_conf.streams[0].payload_type = UVCL_PAYLOAD_FB_H264;
  uvcl_conf.streams_nb = 1;
  uvcl_conf.is_immediate_mode = 1;

  ret = app_display_setup(&enc_conf, &uvcl_conf);
  assert(ret == 0);

  app_pipeline_start();

  BSP_LED_On(LED_GREEN);
}

