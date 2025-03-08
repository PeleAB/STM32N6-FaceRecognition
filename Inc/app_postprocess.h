 /**
 ******************************************************************************
 * @file    app_postprocess.h
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
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __APP_POSTPROCESS_H
#define __APP_POSTPROCESS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "od_yolov2_pp_if.h"
#include "od_yolov5_pp_if.h"
#include "od_yolov8_pp_if.h"
#include "od_st_yolox_pp_if.h"
#include "od_centernet_pp_if.h"
#include "od_ssd_pp_if.h"
#include "od_ssd_st_pp_if.h"
#include "od_pp_output_if.h"
#include "mpe_yolov8_pp_if.h"
#include "mpe_pp_output_if.h"
#include "pd_model_pp_if.h"
#include "pd_pp_output_if.h"
#include "spe_movenet_pp_if.h"
#include "spe_pp_output_if.h"
#include "iseg_yolov8_pp_if.h"
#include "iseg_pp_output_if.h"
#include "sseg_deeplabv3_pp_if.h"
#include "sseg_pp_output_if.h"

#define POSTPROCESS_OD_YOLO_V2_UF       (10) /* Yolov2 postprocessing; Input model: uint8; output: float32         */
#define POSTPROCESS_OD_YOLO_V5_UU       (11) /* Yolov5 postprocessing; Input model: uint8; output: uint8           */
#define POSTPROCESS_OD_YOLO_V8_UF       (12) /* Yolov8 postprocessing; Input model: uint8; output: float32         */
#define POSTPROCESS_OD_YOLO_V8_UI       (13) /* Yolov8 postprocessing; Input model: uint8; output: int8            */
#define POSTPROCESS_OD_ST_YOLOX_UF      (14) /* ST YoloX postprocessing; Input model: uint8; output: float32       */
#define POSTPROCESS_OD_ST_SSD_UF        (15) /* ST SSD postprocessing; Input model: uint8; output: float32         */
#define POSTPROCESS_MPE_YOLO_V8_UF      (20) /* Yolov8 postprocessing; Input model: uint8; output: float32         */
#define POSTPROCESS_MPE_PD_UF           (21) /* Palm detector postprocessing; Input model: uint8; output: float32  */
#define POSTPROCESS_SPE_MOVENET_UF      (22) /* Movenet postprocessing; Input model: uint8; output: float32        */
#define POSTPROCESS_ISEG_YOLO_V8_UI     (30) /* Yolov8 Seg postprocessing; Input model: uint8; output: int8        */
#define POSTPROCESS_SSEG_DEEPLAB_V3_UF  (40) /* Deeplabv3 Seg postprocessing; Input model: uint8; output: float32  */

/* Exported functions ------------------------------------------------------- */
int32_t app_postprocess_init(void *params_postprocess);
int32_t app_postprocess_run(void *pInput[], int nb_input, void *pOutput, void *pInput_param);

#ifdef __cplusplus
}
#endif

#endif /*__APP_POSTPROCESS_H */
