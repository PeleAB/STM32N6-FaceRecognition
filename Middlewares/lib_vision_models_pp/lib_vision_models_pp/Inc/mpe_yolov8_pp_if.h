/*---------------------------------------------------------------------------------------------
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file in
  * the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *--------------------------------------------------------------------------------------------*/

#ifndef __POSE_YOLOV8_PP_IF_H__
#define __POSE_YOLOV8_PP_IF_H__


#ifdef __cplusplus
 extern "C" {
#endif

#include "mpe_pp_output_if.h"


/* I/O structures for YoloV8 POSE type */
/* --------------------------------------- */
typedef struct mpe_yolov8_pp_in_centroid
{
  float32_t *pRaw_detections;
} mpe_yolov8_pp_in_centroid_t;

typedef struct mpe_yolov8_pp_in_centroid_int8
{
  int8_t *pRaw_detections;
} mpe_yolov8_pp_in_centroid_int8_t;


typedef struct mpe_yolov8_pp_static_param {
  int32_t  nb_classes;
  int32_t  nb_total_boxes;
  int32_t  max_boxes_limit;
  float32_t conf_threshold;
  float32_t iou_threshold;
  int32_t nb_detect;
	uint32_t nb_keypoints;
} mpe_yolov8_pp_static_param_t;


/* Exported functions ------------------------------------------------------- */

/*!
 * @brief Resets object detection YoloV8 post processing
 *
 * @param [IN] Input static parameters
 * @retval Error code
 */
int32_t mpe_yolov8_pp_reset(mpe_yolov8_pp_static_param_t *pInput_static_param);


/*!
 * @brief Object detector post processing : includes output detector remapping,
 *        nms and score filtering for YoloV8.
 *
 * @param [IN] Pointer on input data
 *             Pointer on output data
 *             pointer on static parameters
 * @retval Error code
 */
int32_t mpe_yolov8_pp_process(mpe_yolov8_pp_in_centroid_t *pInput,
                              mpe_pp_out_t *pOutput,
                              mpe_yolov8_pp_static_param_t *pInput_static_param);



#ifdef __cplusplus
  }
#endif

#endif      /* __POSE_YOLOV8_PP_IF_H__  */


