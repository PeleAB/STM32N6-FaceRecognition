/*---------------------------------------------------------------------------------------------
 * Copyright (c) 2023 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file in
 * the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *--------------------------------------------------------------------------------------------*/

#include "od_pp_loc.h"
#include "od_yolov2_pp_if.h"
#include "vision_models_pp.h"


/* Can't be removed if qsort is not re-written... */
static int32_t AI_YOLOV2_PP_SORT_CLASS;


int32_t yolov2_nms_comparator(const void *pa, const void *pb)
{
    float32_t a = *((float32_t *)pa + AI_YOLOV2_PP_CLASSPROB + AI_YOLOV2_PP_SORT_CLASS);
    float32_t b = *((float32_t *)pb + AI_YOLOV2_PP_CLASSPROB + AI_YOLOV2_PP_SORT_CLASS);
    float32_t diff = 0;

    diff = a - b;

    if (diff < 0) return 1;
    else if (diff > 0) return -1;
    return 0;
}


int32_t yolov2_pp_nmsFiltering_centroid(yolov2_pp_in_t  *pInput,
                                        yolov2_pp_static_param_t *pInput_static_param)
{
    int32_t i, j, k, limit_counter;
    int32_t anch_stride = (pInput_static_param->nb_classes + AI_YOLOV2_PP_CLASSPROB);
    float32_t *pInbuff = (float32_t *)pInput->pRaw_detections;

    for (k = 0; k < pInput_static_param->nb_classes; ++k)
    {
        limit_counter = 0;
        AI_YOLOV2_PP_SORT_CLASS = k;

        qsort(pInbuff,
              pInput_static_param->nb_detect,
              anch_stride * sizeof(float32_t),
              (_Cmpfun *)yolov2_nms_comparator);
        for (i = 0; i < (pInput_static_param->nb_detect * anch_stride) ; i += anch_stride)
        {
            if (pInbuff[i + AI_YOLOV2_PP_CLASSPROB + k] == 0) continue;
            float32_t *a = &(pInbuff[i + AI_YOLOV2_PP_XCENTER]);
            for (j = i + anch_stride; j < (pInput_static_param->nb_detect * anch_stride); j += anch_stride)
            {
                float32_t *b = &(pInbuff[j + AI_YOLOV2_PP_XCENTER]);
                if (vision_models_box_iou(a, b) > pInput_static_param->iou_threshold)
                {
                    pInbuff[j + AI_YOLOV2_PP_CLASSPROB + k] = 0;
                }
            }
        }
        for (int32_t y = 0; y <= (pInput_static_param->nb_detect * anch_stride); y += anch_stride)
        {
            if ((limit_counter < pInput_static_param->max_boxes_limit) &&
                (pInbuff[y + AI_YOLOV2_PP_CLASSPROB + k] != 0))
            {
                limit_counter++;
            }
            else
            {
                pInbuff[y + AI_YOLOV2_PP_CLASSPROB + k] = 0;
            }
        }
    }

    return (AI_OD_POSTPROCESS_ERROR_NO);
}


int32_t yolov2_pp_scoreFiltering_centroid(yolov2_pp_in_t *pInput,
                                          od_pp_out_t *pOutput,
                                          yolov2_pp_static_param_t *pInput_static_param)
{
    float32_t best_score;
    uint32_t class_index;
    int32_t det_count = 0;
    int32_t anch_stride = (pInput_static_param->nb_classes + AI_YOLOV2_PP_CLASSPROB);

    if (pOutput->pOutBuff == NULL)
    {
      pOutput->pOutBuff = (od_pp_outBuffer_t *)pInput->pRaw_detections;
    }
    float32_t *pInbuff = (float32_t *)pInput->pRaw_detections;

    for (int32_t i = 0; i < (pInput_static_param->nb_detect * anch_stride); i += anch_stride)
    {
        vision_models_maxi_if32ou32(&pInbuff[i + AI_YOLOV2_PP_CLASSPROB],
             pInput_static_param->nb_classes,
             &best_score,
             &class_index);

        if (best_score >= pInput_static_param->conf_threshold)
        {
            pOutput->pOutBuff[det_count].x_center = pInbuff[i + AI_YOLOV2_PP_XCENTER];
            pOutput->pOutBuff[det_count].y_center = pInbuff[i + AI_YOLOV2_PP_YCENTER];
            pOutput->pOutBuff[det_count].width = pInbuff[i + AI_YOLOV2_PP_WIDTHREL];
            pOutput->pOutBuff[det_count].height = pInbuff[i + AI_YOLOV2_PP_HEIGHTREL];
            pOutput->pOutBuff[det_count].class_index = class_index;
            pOutput->pOutBuff[det_count].conf = best_score;
            det_count++;
        }
    }
    pOutput->nb_detect = det_count;

    return (AI_OD_POSTPROCESS_ERROR_NO);
}


int32_t yolov2_pp_getNNBoxes_centroid(yolov2_pp_in_t *pInput,
                                      yolov2_pp_static_param_t *pInput_static_param)
{
    int32_t error   = AI_OD_POSTPROCESS_ERROR_NO;
    int32_t count = 0;
    int32_t count_detect = 0;
    float32_t best_score=0;
    uint32_t class_index;
    int32_t anch_stride = (pInput_static_param->nb_classes + AI_YOLOV2_PP_CLASSPROB);

    yolov2_pp_in_t *pOutput = pInput;
    float32_t grid_width_inv = 1.0f / pInput_static_param->grid_width;
    float32_t grid_height_inv = 1.0f / pInput_static_param->grid_height;
    float32_t tmp_a[pInput_static_param->nb_classes];
    int32_t el_offset = 0;
    float32_t *pInbuff = (float32_t *)pInput->pRaw_detections;
    float32_t *pOutbuff = (float32_t *)pOutput->pRaw_detections;
    for (int32_t row = 0; row < pInput_static_param->grid_width; ++row)
    {
        for (int32_t col = 0; col < pInput_static_param->grid_height; ++col)
        {
            for (int32_t anch = 0; anch < pInput_static_param->nb_anchors; ++anch)
            {
                /* read and activate objectness */
                pOutbuff[el_offset + AI_YOLOV2_PP_OBJECTNESS] = vision_models_sigmoid_f(pInbuff[el_offset + AI_YOLOV2_PP_OBJECTNESS]);

                /* activate array of classes pred */
                vision_models_softmax_f(&pInbuff[el_offset + AI_YOLOV2_PP_CLASSPROB],
                        &pOutbuff[el_offset + AI_YOLOV2_PP_CLASSPROB],
                        pInput_static_param->nb_classes,
                        tmp_a);
                for (int32_t k = 0; k < pInput_static_param->nb_classes; k++)
                {
                    pOutbuff[el_offset + AI_YOLOV2_PP_CLASSPROB + k] = pOutbuff[el_offset + AI_YOLOV2_PP_OBJECTNESS] *
                                                                       pOutbuff[el_offset + AI_YOLOV2_PP_CLASSPROB + k];
                }

                vision_models_maxi_if32ou32(&pOutbuff[el_offset + AI_YOLOV2_PP_CLASSPROB],
                     pInput_static_param->nb_classes,
                     &best_score,
                     &class_index);

                if (best_score >= pInput_static_param->conf_threshold)
                {
                    pOutbuff[count + AI_YOLOV2_PP_OBJECTNESS] = pOutbuff[el_offset + AI_YOLOV2_PP_OBJECTNESS];
                    for (int32_t k = 0; k < pInput_static_param->nb_classes; ++k)
                    {
                        pOutbuff[count + AI_YOLOV2_PP_CLASSPROB + k] = pOutbuff[el_offset + AI_YOLOV2_PP_CLASSPROB + k];
                    }

                    pOutbuff[count + AI_YOLOV2_PP_XCENTER] = (col + vision_models_sigmoid_f(pInbuff[el_offset + AI_YOLOV2_PP_XCENTER])) * grid_width_inv;
                    pOutbuff[count + AI_YOLOV2_PP_YCENTER] = (row + vision_models_sigmoid_f(pInbuff[el_offset + AI_YOLOV2_PP_YCENTER])) * grid_height_inv;
                    pOutbuff[count + AI_YOLOV2_PP_WIDTHREL] = (pInput_static_param->pAnchors[2 * anch] * expf(pInbuff[el_offset + AI_YOLOV2_PP_WIDTHREL])) * grid_width_inv;
                    pOutbuff[count + AI_YOLOV2_PP_HEIGHTREL] = (pInput_static_param->pAnchors[2 * anch + 1] * expf(pInbuff[el_offset + AI_YOLOV2_PP_HEIGHTREL])) * grid_height_inv;

                    count += anch_stride;
                    count_detect++;
                }

                el_offset += anch_stride;
            }
        }
    }

    pInput_static_param->nb_detect = count_detect;
    return (error);
}



/* ----------------------       Exported routines      ---------------------- */

int32_t od_yolov2_pp_reset(yolov2_pp_static_param_t *pInput_static_param)
{
    /* Initializations */
    pInput_static_param->nb_detect = 0;

	return (AI_OD_POSTPROCESS_ERROR_NO);
}


int32_t od_yolov2_pp_process(yolov2_pp_in_t *pInput,
                                    od_pp_out_t *pOutput,
                                    yolov2_pp_static_param_t *pInput_static_param)
{
    int32_t error   = AI_OD_POSTPROCESS_ERROR_NO;

    /* Call Get NN boxes first */
    error = yolov2_pp_getNNBoxes_centroid(pInput,
                                          pInput_static_param);
    if (error != AI_OD_POSTPROCESS_ERROR_NO) return (error);

    /* Then NMS */
    error = yolov2_pp_nmsFiltering_centroid(pInput,
                                            pInput_static_param);
    if (error != AI_OD_POSTPROCESS_ERROR_NO) return (error);

    /* And score re-filtering */
    error = yolov2_pp_scoreFiltering_centroid(pInput,
                                              pOutput,
                                              pInput_static_param);

    return (error);
}

