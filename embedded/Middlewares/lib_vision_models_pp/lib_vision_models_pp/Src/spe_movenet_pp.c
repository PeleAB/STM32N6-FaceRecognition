/*---------------------------------------------------------------------------------------------
#  * Copyright (c) 2023 STMicroelectronics.
#  * All rights reserved.
#  *
#  * This software is licensed under terms that can be found in the LICENSE file in
#  * the root directory of this software component.
#  * If no LICENSE file comes with this software, it is provided AS-IS.
#  *--------------------------------------------------------------------------------------------*/

#include "spe_movenet_pp_loc.h"
#include "spe_movenet_pp_if.h"
#include "vision_models_pp.h"



int32_t movenet_heatmap_max(spe_movenet_pp_in_t *pInput,
                            spe_pp_out_t *pOutput,
                            spe_movenet_pp_static_param_t *pInput_static_param)
{
  uint32_t i;
  float32_t x_center;
  float32_t y_center;
  float32_t proba;
  uint32_t index;
  uint32_t width = pInput_static_param->heatmap_width;
  uint32_t height = pInput_static_param->heatmap_height;
#ifdef _OLD_IMPL
  float32_t *pTmpHeatMap = pInput_static_param->pTmpHeatMap;
#endif
  float32_t *pInputKp = pInput->inBuff;

  for (i = 0; i < pInput_static_param->nb_keypoints; i++)
  {
#ifdef _OLD_IMPL
     /* De-interleaves the heat maps */
     pTmpHeatMap = pInput_static_param->pTmpHeatMap;
     pInputKp = &(pInput->inBuff[i]);
     for (uint32_t hm = 0; hm < (width * height); hm++)
     {
       *pTmpHeatMap++ = *pInputKp;
       pInputKp += pInput_static_param->nb_keypoints;
     }

     /* Computes the argmax on the temporary heat map */
     pTmpHeatMap = pInput_static_param->pTmpHeatMap;
     vision_models_maxi_if32ou32((float32_t *) pTmpHeatMap,
                        (uint32_t) width * height,
                        (float32_t *) &proba,
                        (uint32_t *) &index);
#else

    pInputKp = &(pInput->inBuff[i]);
    vision_models_maxi_tr_if32ou32((float32_t *) pInputKp,
                                   (uint32_t) width * height,
                                   pInput_static_param->nb_keypoints,
                                   (float32_t *) &proba,
                                   (uint32_t *) &index);
#endif

    /* This is not cartesian referential, to be aligned with Python code */
    y_center = ((index % height + 0.5f) / height);
    x_center = ((index / height + 0.5f) / width);

    /* Coordinates inversion for the application code that uses cartesian referential */
    pOutput->pOutBuff[i].x_center = y_center;
    pOutput->pOutBuff[i].y_center = x_center;
    pOutput->pOutBuff[i].proba = proba;
  }

  return (AI_SPE_POSTPROCESS_ERROR_NO);
}


/* ----------------------       Exported routines      ---------------------- */

int32_t spe_movenet_pp_reset(spe_movenet_pp_static_param_t *pInput_static_param)
{
  /* Initializations */
  // pInput_static_param->... no initialization

	return (AI_SPE_POSTPROCESS_ERROR_NO);
}


int32_t spe_movenet_pp_process(spe_movenet_pp_in_t *pInput,
                                     spe_pp_out_t *pOutput,
                                     spe_movenet_pp_static_param_t *pInput_static_param)
{
    int32_t error   = AI_SPE_POSTPROCESS_ERROR_NO;

     /* Extracts max value and indexes of each heatmap */
    error = movenet_heatmap_max(pInput,
                                pOutput,
                                pInput_static_param);

    return (error);
}
