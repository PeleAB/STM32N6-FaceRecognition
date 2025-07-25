 /**
 ******************************************************************************
 * @file    app_config.h
 * @author  PeleAB
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

#ifndef APP_CONFIG
#define APP_CONFIG

#include "arm_math.h"

#define USE_DCACHE


/* ========================================================================= */
/* DUMMY INPUT BUFFER CONFIGURATION                                          */
/* ========================================================================= */
/* Enable DUMMY_INPUT_BUFFER to use a predefined test image for debugging.  */
/* This overrides the camera/PC stream input with a constant test pattern.  */
/* Useful for students to test their implementations with known input.      */
/* ========================================================================= */
//#define DUMMY_INPUT_BUFFER


/*Defines: CMW_MIRRORFLIP_NONE; CMW_MIRRORFLIP_FLIP; CMW_MIRRORFLIP_MIRROR; CMW_MIRRORFLIP_FLIP_MIRROR;*/
#define CAMERA_FLIP CMW_MIRRORFLIP_NONE

#define ASPECT_RATIO_CROP (1) /* Crop both pipes to nn input aspect ratio; Original aspect ratio kept */
#define ASPECT_RATIO_FIT (2) /* Resize both pipe to NN input aspect ratio; Original aspect ratio not kept */
#define ASPECT_RATIO_FULLSCREEN (3) /* Resize camera image to NN input size and display a fullscreen image */
#define ASPECT_RATIO_MODE ASPECT_RATIO_CROP

#define CAPTURE_FORMAT DCMIPP_PIXEL_PACKER_FORMAT_RGB565_1
#define CAPTURE_BPP 2
/* Leave the driver use the default resolution */
#define CAMERA_WIDTH 0
#define CAMERA_HEIGHT 0

#define LCD_FG_WIDTH             800
#define LCD_FG_HEIGHT            480
#define LCD_FG_FRAMEBUFFER_SIZE  (LCD_FG_WIDTH * LCD_FG_HEIGHT * 2)

/* Model Related Info */
#define POSTPROCESS_TYPE POSTPROCESS_MPE_PD_UF

#define NN_WIDTH (128)
#define NN_HEIGHT (128)
#define NN_BPP (3)

#define COLOR_BGR (0)
#define COLOR_RGB (1)
#define COLOR_MODE COLOR_RGB

#define NB_CLASSES 1
#define CLASSES_TABLE const char* classes_table[NB_CLASSES] = {\
"face"}
/* Enable or disable LCD and PC streaming features */
#define ENABLE_LCD_DISPLAY
//#define ENABLE_PC_STREAM  // Disabled: using Enhanced_PC_STREAM instead
/* Application input source configuration */
#define INPUT_SRC_CAMERA 0
#define INPUT_SRC_PC     1
/* Select input source: INPUT_SRC_CAMERA or INPUT_SRC_PC */
#ifndef INPUT_SRC_MODE
#define INPUT_SRC_MODE INPUT_SRC_CAMERA
#endif

/* --------  Tuning below can be modified by the application --------- */
#define AI_OBJDETECT_YOLOV2_PP_CONF_THRESHOLD    (0.6f)
#define AI_OBJDETECT_YOLOV2_PP_IOU_THRESHOLD     (0.3f)
#define AI_OBJDETECT_YOLOV2_PP_MAX_BOXES_LIMIT   (10)


/* CenterFace detection parameters */
#define AI_PD_MODEL_PP_WIDTH              (NN_WIDTH)
#define AI_PD_MODEL_PP_HEIGHT             (NN_HEIGHT)
#define AI_PD_MODEL_PP_NB_KEYPOINTS       (5)
#define AI_PD_MODEL_PP_TOTAL_DETECTIONS   (1024)
#define AI_PD_MODEL_PP_CONF_THRESHOLD     (0.5f)
#define AI_PD_MODEL_PP_IOU_THRESHOLD      (0.3f)
#define AI_PD_MODEL_PP_MAX_BOXES_LIMIT    (10)

/* MediaPipe face detection */
#define MP_FACE_PP_CONF_THRESHOLD (0.5f)

/* Display */
#define WELCOME_MSG_1         "STM32N6 Face Recognition"
#define WELCOME_MSG_2         "AI-Powered Smart Vision System"

#endif
