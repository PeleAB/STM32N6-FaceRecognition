 /**
 ******************************************************************************
 * @file    app_cam.h
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
#ifndef APP_CAM
#define APP_CAM

#define CAMERA_FPS 30

void CAM_Init(uint32_t *lcd_bg_width, uint32_t *lcd_bg_height, uint32_t *pitch_nn);
void CAM_DeInit(void);
void CAM_Start(void);
void CAM_DisplayPipe_Start(uint8_t *display_pipe_dst, uint32_t cam_mode);
void CAM_DisplayPipe_Stop(void);
void CAM_NNPipe_Start(uint8_t *nn_pipe_dst, uint32_t cam_mode);
void CAM_IspUpdate(void);

#endif