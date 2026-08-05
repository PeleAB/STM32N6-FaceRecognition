/**
 ******************************************************************************
 * @file    app_display.h
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

#ifndef APP_DISPLAY_H
#define APP_DISPLAY_H

#include <stdint.h>

#include "sysobj_encoder.h"
#include "app_postprocess.h"
#include "svc/face_detect.h"
#include "uvcl.h"

void app_display_init(void);
int app_display_setup(const ENC_Conf_t *enc_conf, const UVCL_Conf_t *uvcl_conf);
int app_display_render(uint8_t *frame_buffer, od_pp_out_t *pp_out,
                       float landmarks[][FACE_NB_LANDMARKS * 2], int nb_faces);

#endif




