/*---------------------------------------------------------------------------------------------
 * Copyright (c) 2023 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file in
 * the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *--------------------------------------------------------------------------------------------*/

#include "od_pp_loc.h"
#include "od_st_yolox_pp_if.h"
#include "vision_models_pp.h"


/* Can't be removed if qsort is not re-written... */
static int32_t AI_ST_YOLOX_PP_SORT_CLASS;


int32_t st_yolox_nms_comparator(const void *pa, const void *pb)
{
    od_pp_outBuffer_t a = *(od_pp_outBuffer_t *)pa;
    od_pp_outBuffer_t b = *(od_pp_outBuffer_t *)pb;

    float32_t diff = 0.0;
    float32_t a_weighted_conf = 0.0;
    float32_t b_weighted_conf = 0.0;

    if (a.class_index == AI_ST_YOLOX_PP_SORT_CLASS)
    {
        a_weighted_conf = a.conf;
    }
    else
    {
         a_weighted_conf = 0.0;
    }

    if (b.class_index == AI_ST_YOLOX_PP_SORT_CLASS)
    {
        b_weighted_conf = b.conf;
    }
    else
    {
         b_weighted_conf = 0.0;
    }

    diff = a_weighted_conf - b_weighted_conf;

    if (diff < 0) return 1;
    else if (diff > 0) return -1;
    return 0;
}

int32_t st_yolox_pp_nmsFiltering_centroid(od_pp_out_t *pOutput,
                                          st_yolox_pp_static_param_t *pInput_static_param)
{
    int32_t j, k, limit_counter, detections_per_class;
    for (k = 0; k < pInput_static_param->nb_classes; ++k)
    {
        limit_counter = 0;
        detections_per_class = 0;
        AI_ST_YOLOX_PP_SORT_CLASS = k;


        /* Counts the number of detections with class k */
        for (int32_t i = 0; i < pInput_static_param->nb_detect ; i ++)
        {
            if(pOutput->pOutBuff[i].class_index == k)
            {
                detections_per_class++;
            }
        }

        if (detections_per_class > 0)
        {
            /* Sorts detections based on class k */
            qsort(pOutput->pOutBuff,
                  pInput_static_param->nb_detect,
                  sizeof(od_pp_outBuffer_t),
                  st_yolox_nms_comparator);

            for (int32_t i = 0; i < detections_per_class ; i ++)
            {
                if (pOutput->pOutBuff[i].conf == 0) continue;
                float32_t *a = &(pOutput->pOutBuff[i].x_center);
                for (j = i + 1; j < detections_per_class; j ++)
                {
                    float32_t *b = &(pOutput->pOutBuff[j].x_center);
                    if (vision_models_box_iou(a, b) > pInput_static_param->iou_threshold)
                    {
                        pOutput->pOutBuff[j].conf = 0;
                    }
                }
            }

            /* Limits detections count */
            for (int32_t i = 0; i < detections_per_class; i++)
            {
                if ((limit_counter < pInput_static_param->max_boxes_limit) &&
                    (pOutput->pOutBuff[i].conf != 0))
                {
                    limit_counter++;
                }
                else
                {
                    pOutput->pOutBuff[i].conf = 0;
                }
            }
        }
    }
    return (AI_OD_POSTPROCESS_ERROR_NO);
}


int32_t st_yolox_pp_scoreFiltering_centroid(od_pp_out_t *pOutput,
                                            st_yolox_pp_static_param_t *pInput_static_param)
{
    int32_t det_count = 0;

    for (int32_t i = 0; i < pInput_static_param->nb_detect; i++)
    {
        if (pOutput->pOutBuff[i].conf >= pInput_static_param->conf_threshold)
        {
            pOutput->pOutBuff[det_count].x_center = pOutput->pOutBuff[i].x_center;
            pOutput->pOutBuff[det_count].y_center = pOutput->pOutBuff[i].y_center;
            pOutput->pOutBuff[det_count].width = pOutput->pOutBuff[i].width;
            pOutput->pOutBuff[det_count].height = pOutput->pOutBuff[i].height;
            pOutput->pOutBuff[det_count].conf = pOutput->pOutBuff[i].conf;
            pOutput->pOutBuff[det_count].class_index = pOutput->pOutBuff[i].class_index;
            det_count++;
        }
    }
    pOutput->nb_detect = det_count;

    return (AI_OD_POSTPROCESS_ERROR_NO);
}



int32_t st_yolox_pp_level_decode(float32_t *pInbuff, float32_t *pOutbuff, float32_t *pAnchors, int32_t grid_width, int32_t grid_height,
                                 float32_t *tmp_a, 
                                 st_yolox_pp_static_param_t *pInput_static_param)

{
    int32_t el_offset = 0;
    int32_t count = 0;
    int32_t count_detect = 0;
    float32_t best_score=0;
    uint32_t class_index;
    int32_t anch_stride = (pInput_static_param->nb_classes + AI_YOLOV2_PP_CLASSPROB);
    float32_t grid_width_inv = 1.0f / grid_width;
    float32_t grid_height_inv = 1.0f / grid_height;

    for (int32_t row = 0; row < grid_width; ++row)
    {
        for (int32_t col = 0; col < grid_height; ++col)
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
                    pOutbuff[count + AI_YOLOV2_PP_WIDTHREL] = (pAnchors[2 * anch] * expf(pInbuff[el_offset + AI_YOLOV2_PP_WIDTHREL])) * grid_width_inv;
                    pOutbuff[count + AI_YOLOV2_PP_HEIGHTREL] = (pAnchors[2 * anch + 1] * expf(pInbuff[el_offset + AI_YOLOV2_PP_HEIGHTREL])) * grid_height_inv;

                    count += anch_stride;
                    count_detect++;
                }

                el_offset += anch_stride;
            }
        }
    }

    return count_detect;

}


int32_t st_yolox_pp_store_detections(float32_t *pInbuff,
                                     od_pp_out_t *pOutput,
                                     int32_t level_count_detect,
                                     int32_t det_count,
                                     st_yolox_pp_static_param_t *pInput_static_param)
{
    float32_t best_score = 0.0;
    uint32_t class_index = 0;
    int32_t anch_stride = (pInput_static_param->nb_classes + AI_YOLOV2_PP_CLASSPROB);

    for (int32_t i = 0; i < (level_count_detect * anch_stride); i += anch_stride)
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

    return det_count;

}

int32_t st_yolox_pp_getNNBoxes_centroid(st_yolox_pp_in_t *pInput,
                                        od_pp_out_t *pOut,
                                        st_yolox_pp_static_param_t *pInput_static_param)
{

    int32_t error   = AI_OD_POSTPROCESS_ERROR_NO;

    int32_t count_detect_L = 0;
    int32_t count_detect_M = 0;
    int32_t count_detect_S = 0;

    float32_t tmp_a[pInput_static_param->nb_classes];

    int32_t grid_width, grid_height;
    float32_t *pInbuff, *pAnchors;

    //level L
    grid_width = pInput_static_param->grid_width_L;
    grid_height = pInput_static_param->grid_height_L;
    pInbuff = (float32_t *)pInput->pRaw_detections_L;
    pAnchors = (float32_t *)pInput_static_param->pAnchors_L;
    count_detect_L = st_yolox_pp_level_decode(pInbuff, pInbuff, pAnchors, grid_width, grid_height,tmp_a,pInput_static_param);

    //==============================================================================================================================================================
    int32_t init_det_count = 0;
    int32_t det_count_L = 0;
    int32_t det_count_M = 0;
    int32_t det_count_S = 0;
    
    if (pOut->pOutBuff == NULL)
    {
      pOut->pOutBuff = (od_pp_outBuffer_t *)pInput->pRaw_detections_L;
    }

    det_count_L = st_yolox_pp_store_detections(pInbuff,pOut,count_detect_L,init_det_count,pInput_static_param);

    //==============================================================================================================================================================

    //level M
    grid_width = pInput_static_param->grid_width_M;
    grid_height = pInput_static_param->grid_height_M;
    pInbuff = (float32_t *)pInput->pRaw_detections_M;
    pAnchors = (float32_t *)pInput_static_param->pAnchors_M;
    count_detect_M = st_yolox_pp_level_decode(pInbuff, pInbuff, pAnchors, grid_width, grid_height,tmp_a,pInput_static_param);

    det_count_M = st_yolox_pp_store_detections(pInbuff,pOut,count_detect_M,det_count_L,pInput_static_param);


    //level S
    grid_width = pInput_static_param->grid_width_S;
    grid_height = pInput_static_param->grid_height_S;
    pInbuff = (float32_t *)pInput->pRaw_detections_S;
    pAnchors = (float32_t *)pInput_static_param->pAnchors_S;
    count_detect_S = st_yolox_pp_level_decode(pInbuff, pInbuff, pAnchors, grid_width, grid_height,tmp_a,pInput_static_param);

    det_count_S = st_yolox_pp_store_detections(pInbuff,pOut,count_detect_S,det_count_M,pInput_static_param);

    pInput_static_param->nb_detect = det_count_S;

    return (error);
}



/* ----------------------       Exported routines      ---------------------- */

int32_t od_st_yolox_pp_reset(st_yolox_pp_static_param_t *pInput_static_param)
{
    /* Initializations */
    pInput_static_param->nb_detect = 0;

	return (AI_OD_POSTPROCESS_ERROR_NO);
}


int32_t od_st_yolox_pp_process(st_yolox_pp_in_t *pInput,
                               od_pp_out_t *pOutput,
                               st_yolox_pp_static_param_t *pInput_static_param)
{
    int32_t error   = AI_OD_POSTPROCESS_ERROR_NO;

    /* Call Get NN boxes first */
    error = st_yolox_pp_getNNBoxes_centroid(pInput,
                                            pOutput,
                                            pInput_static_param);
    if (error != AI_OD_POSTPROCESS_ERROR_NO) return (error);

    /* Then NMS */
    error = st_yolox_pp_nmsFiltering_centroid(pOutput,
                                              pInput_static_param);
    if (error != AI_OD_POSTPROCESS_ERROR_NO) return (error);

    /* And score re-filtering */
    error = st_yolox_pp_scoreFiltering_centroid(pOutput,
                                                pInput_static_param);

    return (error);
}

