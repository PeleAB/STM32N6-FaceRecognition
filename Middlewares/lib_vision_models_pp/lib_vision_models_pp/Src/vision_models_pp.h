/*---------------------------------------------------------------------------------------------
 * Copyright (c) 2022 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file in
 * the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *--------------------------------------------------------------------------------------------*/

#ifndef __VISION_MODELS_PP_LOC_H__
#define __VISION_MODELS_PP_LOC_H__


#ifdef __cplusplus
 extern "C" {
#endif

/* Error return codes */
#define AI_VISION_MODELS_PP_ERROR_NO                    (0)
#define AI_VISION_MODELS_PP_ERROR_BAD_HW                (-1)
#define AI_VISION_MODELS_PP_ERROR                       (-2)


#include "arm_math.h"

#ifdef ARM_MATH_MVEF
#define AI_OD_YOLOV5_PP_MVEF_OPTIM
#define AI_OD_YOLOV8_PP_MVEF_OPTIM
#define AI_SPE_MOVENET_PP_MVEF_OPTIM
#define AI_SSEG_DEEPLAB_PP_MVEF_OPTIM
//#define AI_MPE_YOLOV8_PP_MVEF_OPTIM
#endif
#ifdef ARM_MATH_MVEI
#define AI_OD_YOLOV5_PP_MVEI_OPTIM
#define AI_OD_YOLOV8_PP_MVEI_OPTIM
#endif

#ifndef MIN
  #define MIN(x,y) ((x) < (y) ? (x) : (y))
#endif

#ifndef MAX
  #define MAX(x,y) ((x) > (y) ? (x) : (y))
#endif

typedef int32_t _Cmpfun(const void *, const void *);
extern void qsort(void *, size_t, size_t, _Cmpfun *);


void vision_models_maxi_if32ou32(float32_t *arr, uint32_t len_arr, float32_t *maxim, uint32_t *index);

void vision_models_maxi_p_if32ou32(float32_t *arr, uint32_t len_arr, uint32_t offset, float32_t *maxim, uint32_t *index, uint32_t parallelize);
void vision_models_maxi_p_if32ou16(float32_t *arr, uint32_t len_arr, uint16_t offset, float32_t *maxim, uint16_t *index, uint32_t parallelize);
void vision_models_maxi_p_if32ou8(float32_t *arr, uint32_t len_arr, uint32_t offset, float32_t *maxim, uint8_t *index, uint32_t parallelize);

void vision_models_maxi_tr_if32ou32(float32_t *arr, uint32_t len_arr, uint32_t nb_total_boxes, float32_t *maxim, uint32_t *index);

void vision_models_maxi_tr_p_if32ou32(float32_t *arr, uint32_t len_arr, uint32_t nb_total_boxes, float32_t *maxim, uint32_t *index, uint32_t parallelize);

void vision_models_maxi_iu8ou8(uint8_t *arr, uint32_t len_arr, uint8_t *maxim, uint8_t *index);
void vision_models_maxi_iu8ou16(uint8_t *arr, uint32_t len_arr, uint8_t *maxim, uint16_t *index);

void vision_models_maxi_p_iu8ou8(uint8_t *arr, uint32_t len_arr, uint32_t offset, uint8_t *maxim, uint8_t *index, uint32_t parallelize);
void vision_models_maxi_p_iu8ou16(uint8_t *arr, uint32_t len_arr, uint32_t offset, uint8_t *maxim, uint16_t *index, uint32_t parallelize);


void vision_models_maxi_p_is8ou8(int8_t *arr, uint32_t len_arr, uint32_t offset, int8_t *maxim, uint8_t *index, uint32_t parallelize);
void vision_models_maxi_p_is8ou16(int8_t *arr, uint32_t len_arr, uint32_t offset, int8_t *maxim, uint16_t *index, uint32_t parallelize);

void vision_models_maxi_offset_if32ou32(float32_t *arr, uint32_t nb_elem_arr, uint32_t offset, float32_t *maxim, uint32_t *index);

void vision_models_maxi_tr_p_is8ou16(int8_t *arr, uint32_t len_arr, uint32_t offset, int8_t *maxim, uint16_t *index, uint32_t parallelize);
void vision_models_maxi_tr_p_is8ou8(int8_t *arr, uint32_t len_arr, uint32_t offset, int8_t *maxim, uint8_t *index, uint32_t parallelize);

void vision_models_maxi_tr_is8ou8(int8_t *arr, uint32_t len_arr, uint32_t nb_total_boxes, int8_t *maxim, uint8_t *index);
void vision_models_maxi_tr_is8ou16(int8_t *arr, uint32_t len_arr, uint32_t nb_total_boxes, int8_t *maxim, uint16_t *index);

float32_t vision_models_sigmoid_f(float32_t x);
void vision_models_softmax_f(float32_t *input_x, float32_t *output_x, int32_t len_x, float32_t *tmp_x);
float32_t vision_models_box_iou(float32_t *a, float32_t *b);
float32_t vision_models_box_iou_is8(int8_t *a, int8_t *b, int8_t zp);

void transpose_flattened_2D(float32_t *arr, int32_t rows, int32_t cols, float32_t *tmp_x);
void dequantize(int32_t* arr, float32_t* tmp, int32_t n, int32_t zero_point, float32_t scale);


#ifdef __cplusplus
  }
#endif

#endif   /*  __VISION_MODELS_PP_LOC_H__  */
