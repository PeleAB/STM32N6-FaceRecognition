/**
 ******************************************************************************
 * @file    face_detect.h
 * @brief   CenterFace post-processing output structures and configuration.
 ******************************************************************************
 */
#ifndef FACE_DETECT_H
#define FACE_DETECT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "arm_math.h"
#include "od_pp_output_if.h"

#define FACE_NB_LANDMARKS 5

/**
 * @brief CenterFace output head indices.
 *
 * These correspond to the order in which stedgeai exposes the model's
 * output tensors.  Verify against stai_fd.h after running generate-n6-model.sh
 * and adjust if needed.
 *
 * STEdgeAI output order (verified by value-range analysis on centerface_OE_3_3_1.onnx):
 *   OUT_1 Transpose_293  [1, 32, 32, 2]  -> log scale (height, width)
 *   OUT_2 Transpose_285  [1, 32, 32, 10] -> landmarks
 *   OUT_3 Transpose_300  [1, 32, 32, 1]  -> heatmap   (sigmoid, 0-1)
 *   OUT_4 Transpose_289  [1, 32, 32, 2]  -> offset    (y, x)
 *
 * stai_fd_get_outputs() returns them in this order as indices 0..3.
 */
#define FD_OUT_SCALE     0
#define FD_OUT_LANDMARKS 1
#define FD_OUT_HEATMAP   2
#define FD_OUT_OFFSET    3
#define FD_OUT_NUM       4

/**
 * @brief Static parameters for the CenterFace post-processor.
 */
typedef struct {
  int32_t   grid_width;
  int32_t   grid_height;
  float32_t conf_threshold;
  float32_t iou_threshold;
  int32_t   max_boxes_limit;
} centerface_pp_static_param_t;

/**
 * @brief Combined output: bounding boxes + facial landmarks.
 *
 * boxes.pOutBuff[i] has the bounding box for detection i.
 * landmarks[i] holds the 5 facial keypoints as (x0,y0, x1,y1, ..., x4,y4)
 * in normalised [0,1] coordinates.
 */
typedef struct {
  od_pp_out_t boxes;
  float32_t   landmarks[1][FACE_NB_LANDMARKS * 2]; /* sized at allocation site */
} centerface_pp_out_t;

#ifdef __cplusplus
}
#endif

#endif /* FACE_DETECT_H */
