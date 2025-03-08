/*---------------------------------------------------------------------------------------------
 * Copyright (c) 2023 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file in
 * the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *--------------------------------------------------------------------------------------------*/

#include "od_pp_loc.h"
#include "od_ssd_pp_if.h"
#include "vision_models_pp.h"


static int32_t SSD_quick_sort_partition(float32_t *pScores,
                                        float32_t *pBoxes,
                                        int32_t first,
                                        int32_t last,
                                        int32_t dir,
                                        int32_t ssd_sort_class,
                                        int32_t ssd_nb_classes,
                                        float32_t *pTmp)
{
    int32_t i, j, pivot_index;
    float32_t pivot;
    pivot_index = first;
    pivot = pScores[pivot_index * ssd_nb_classes + ssd_sort_class];
    i = first - 1;
    j = last + 1;

    while (i < j)
    {
        if (dir)
        {
            do
            {
                i++;
            } while ((pScores[i * ssd_nb_classes + ssd_sort_class] < pivot) &&
                     (i < last));
            do
            {
                j--;
            } while (pScores[j * ssd_nb_classes + ssd_sort_class] > pivot);
        }
        else
        {
            do
            {
                i++;
            } while ((pScores[i * ssd_nb_classes + ssd_sort_class] > pivot) &&
                     (i < last));
            do
            {
                j--;
            } while (pScores[j * ssd_nb_classes + ssd_sort_class] < pivot);
        }

        if (i < j)
        {
            memcpy(pTmp, &pScores[i * ssd_nb_classes], ssd_nb_classes * sizeof(*pScores));
            memcpy(&pScores[i * ssd_nb_classes], &pScores[j * ssd_nb_classes], ssd_nb_classes * sizeof(*pScores));
            memcpy(&pScores[j * ssd_nb_classes], pTmp, ssd_nb_classes * sizeof(*pScores));

            memcpy(pTmp, &pBoxes[i * AI_SSD_PP_BOX_STRIDE], AI_SSD_PP_BOX_STRIDE * sizeof(*pTmp));
            memcpy(&pBoxes[i * AI_SSD_PP_BOX_STRIDE], &pBoxes[j * AI_SSD_PP_BOX_STRIDE], AI_SSD_PP_BOX_STRIDE * sizeof(*pTmp));
            memcpy(&pBoxes[j * AI_SSD_PP_BOX_STRIDE], pTmp, AI_SSD_PP_BOX_STRIDE * sizeof(*pTmp));
        }
    }
    return j;
}

static void SSD_quick_sort_core(float32_t *pScores,
                                float32_t *pBoxes,
                                int32_t first,
                                int32_t last,
                                int32_t dir,
                                int32_t ssd_sort_class,
                                int32_t ssd_nb_classes,
                                float32_t *pTmp)
{
    /*
     dir 0  : descending
     dir 1  : ascending
    */
    if (first < last)
    {
        int32_t pivot;
        pivot = SSD_quick_sort_partition(pScores,
                                         pBoxes,
                                         first,
                                         last,
                                         dir,
                                         ssd_sort_class,
                                         ssd_nb_classes,
                                         pTmp);
        SSD_quick_sort_core(pScores,
                            pBoxes,
                            first,
                            pivot,
                            dir,
                            ssd_sort_class,
                            ssd_nb_classes,
                            pTmp);
        SSD_quick_sort_core(pScores,
                            pBoxes,
                            pivot + 1,
                            last,
                            dir,
                            ssd_sort_class,
                            ssd_nb_classes,
                            pTmp);
    }
}


int32_t ssd_pp_getNNBoxes(ssd_pp_in_centroid_t *pInput,
                          ssd_pp_static_param_t *pInput_static_param)
{

    pInput_static_param->nb_detect = 0;
    float32_t best_score = 0;
    uint32_t class_index = 0;

    for (int32_t i = 0; i < pInput_static_param->nb_detections; ++i)
    {
        vision_models_maxi_if32ou32(&(pInput->pScores[i * pInput_static_param->nb_classes]),
                       pInput_static_param->nb_classes,
                       &best_score,
                       &class_index);
        if (best_score >= pInput_static_param->conf_threshold)
        {
            for (int32_t k = 0; k < pInput_static_param->nb_classes; ++k)
            {
                pInput->pScores[pInput_static_param->nb_detect * pInput_static_param->nb_classes + k] = pInput->pScores[i * pInput_static_param->nb_classes + k];
            }
            pInput->pBoxes[pInput_static_param->nb_detect * AI_SSD_PP_BOX_STRIDE + AI_SSD_PP_CENTROID_XCENTER] = pInput->pBoxes[i * AI_SSD_PP_BOX_STRIDE + AI_SSD_PP_CENTROID_XCENTER] / pInput_static_param->XY_scale * pInput->pAnchors[i * AI_SSD_PP_BOX_STRIDE + AI_SSD_PP_CENTROID_WIDTHREL] + pInput->pAnchors[i * AI_SSD_PP_BOX_STRIDE + AI_SSD_PP_CENTROID_XCENTER];
            pInput->pBoxes[pInput_static_param->nb_detect * AI_SSD_PP_BOX_STRIDE + AI_SSD_PP_CENTROID_YCENTER] = pInput->pBoxes[i * AI_SSD_PP_BOX_STRIDE + AI_SSD_PP_CENTROID_YCENTER] / pInput_static_param->XY_scale * pInput->pAnchors[i * AI_SSD_PP_BOX_STRIDE + AI_SSD_PP_CENTROID_HEIGHTREL] + pInput->pAnchors[i * AI_SSD_PP_BOX_STRIDE + AI_SSD_PP_CENTROID_YCENTER];
            pInput->pBoxes[pInput_static_param->nb_detect * AI_SSD_PP_BOX_STRIDE + AI_SSD_PP_CENTROID_WIDTHREL] = expf(pInput->pBoxes[i * AI_SSD_PP_BOX_STRIDE + AI_SSD_PP_CENTROID_WIDTHREL] / pInput_static_param->WH_scale) * pInput->pAnchors[i * AI_SSD_PP_BOX_STRIDE + AI_SSD_PP_CENTROID_WIDTHREL];
            pInput->pBoxes[pInput_static_param->nb_detect * AI_SSD_PP_BOX_STRIDE + AI_SSD_PP_CENTROID_HEIGHTREL] = expf(pInput->pBoxes[i * AI_SSD_PP_BOX_STRIDE + AI_SSD_PP_CENTROID_HEIGHTREL] / pInput_static_param->WH_scale) * pInput->pAnchors[i * AI_SSD_PP_BOX_STRIDE + AI_SSD_PP_CENTROID_HEIGHTREL];

            (pInput_static_param->nb_detect)++;
        }
    }

    return (AI_OD_POSTPROCESS_ERROR_NO);
}


int32_t ssd_pp_nms_filtering(ssd_pp_in_centroid_t *pInput,
                             ssd_pp_static_param_t *pInput_static_param)
{
    int32_t i, j, k, ssd_sort_class, limit_counter;
    float32_t tmp[pInput_static_param->nb_classes];

    for (k = 0; k < pInput_static_param->nb_classes; ++k)
    {
        limit_counter = 0;
        ssd_sort_class = k;

        SSD_quick_sort_core(pInput->pScores,
                            pInput->pBoxes,
                            0,
                            pInput_static_param->nb_detect - 1,
                            0,
                            ssd_sort_class,
                            pInput_static_param->nb_classes,
                            tmp);

        for (i = 0; i < pInput_static_param->nb_detect; ++i)
        {
            if (pInput->pScores[i * pInput_static_param->nb_classes + k] == 0)
            {
                continue;
            }
            float32_t *pA = &(pInput->pBoxes[AI_SSD_PP_BOX_STRIDE * i + AI_SSD_PP_CENTROID_YCENTER]);
            for (j = i + 1; j < pInput_static_param->nb_detect; ++j)
            {
                float32_t *pB = &(pInput->pBoxes[AI_SSD_PP_BOX_STRIDE * j + AI_SSD_PP_CENTROID_YCENTER]);
                if (vision_models_box_iou(pA, pB) > pInput_static_param->iou_threshold)
                {
                    pInput->pScores[j * pInput_static_param->nb_classes + k] = 0;
                }
            }
        }

        for (int32_t it = 0; it < pInput_static_param->nb_detect; ++it)
        {
            if ((pInput->pScores[it * pInput_static_param->nb_classes + k] != 0) &&
                (limit_counter < pInput_static_param->max_boxes_limit))
            {
                limit_counter++;
            }
            else
            {
                pInput->pScores[it * pInput_static_param->nb_classes + k] = 0;
            }
        }
    }

    return (AI_OD_POSTPROCESS_ERROR_NO);
}


int32_t ssd_pp_score_filtering(ssd_pp_in_centroid_t *pInput,
                               od_pp_out_t *pOutput,
                               ssd_pp_static_param_t *pInput_static_param)
{

    if (pOutput->pOutBuff == NULL)
    {
        pOutput->pOutBuff = (od_pp_outBuffer_t *)pInput->pScores;
    }

    float32_t best_score = 0;
    uint32_t class_index = 0;
    int32_t count = 0;
    for (int32_t i = 0; i < pInput_static_param->nb_detect; ++i)
    {
        vision_models_maxi_if32ou32(&(pInput->pScores[i * pInput_static_param->nb_classes]),
                       pInput_static_param->nb_classes,
                       &best_score,
                       &class_index);
        if (best_score >= pInput_static_param->conf_threshold)
        {
            pOutput->pOutBuff[count].class_index = class_index;
            pOutput->pOutBuff[count].conf = best_score;
            pOutput->pOutBuff[count].x_center = pInput->pBoxes[i * AI_SSD_PP_BOX_STRIDE + AI_SSD_PP_CENTROID_XCENTER];
            pOutput->pOutBuff[count].y_center = pInput->pBoxes[i * AI_SSD_PP_BOX_STRIDE + AI_SSD_PP_CENTROID_YCENTER];
            pOutput->pOutBuff[count].width = pInput->pBoxes[i * AI_SSD_PP_BOX_STRIDE + AI_SSD_PP_CENTROID_WIDTHREL];
            pOutput->pOutBuff[count].height = pInput->pBoxes[i * AI_SSD_PP_BOX_STRIDE + AI_SSD_PP_CENTROID_HEIGHTREL];

            count++;
        }
    }

    pOutput->nb_detect = count;
    return (AI_OD_POSTPROCESS_ERROR_NO);
}



/* ----------------------       Exported routines      ---------------------- */

int32_t od_ssd_pp_reset(ssd_pp_static_param_t *pInput_static_param)
{
    /* Initializations */
    pInput_static_param->nb_detect = 0;

    return (AI_OD_POSTPROCESS_ERROR_NO);
}


int32_t od_ssd_pp_process(ssd_pp_in_centroid_t *pInput,
                                 od_pp_out_t *pOutput,
                                 ssd_pp_static_param_t *pInput_static_param)
{
    int32_t error = AI_OD_POSTPROCESS_ERROR_NO;

    /* Calls Get NN boxes first */
    error = ssd_pp_getNNBoxes(pInput,
                              pInput_static_param);
    if (error != AI_OD_POSTPROCESS_ERROR_NO) return (error);

    /* Then NMS */
    error = ssd_pp_nms_filtering(pInput,
                                 pInput_static_param);
    if (error != AI_OD_POSTPROCESS_ERROR_NO) return (error);

    /* And score re-filtering */
    error = ssd_pp_score_filtering(pInput,
                                   pOutput,
                                   pInput_static_param);

    return (error);
}
