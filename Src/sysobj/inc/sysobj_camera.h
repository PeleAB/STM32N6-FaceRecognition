/**
 ******************************************************************************
 * @file    sysobj_camera.h
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

#ifndef SYSOBJ_CAMERA_H
#define SYSOBJ_CAMERA_H

#include <stdint.h>

#include "cmw_camera.h"

void CAM_Init(void);
void CAM_DisplayPipe_Start(uint8_t *display_pipe_dst, uint32_t cam_mode);
void CAM_NNPipe_Start(uint8_t *nn_pipe_dst, uint32_t cam_mode);
void CAM_IspUpdate(void);
int CAM_DisplayPipe_UpdateAddress(uint8_t *display_pipe_dst);
int CAM_NNPipe_UpdateAddress(uint8_t *nn_pipe_dst);

int CAM_GetVencWidth(void);
int CAM_GetVencHeight(void);

#endif /* SYSOBJ_CAMERA_H */



