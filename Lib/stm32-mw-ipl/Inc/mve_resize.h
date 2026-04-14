/**
  ******************************************************************************
  * @file    mve_resize.h
  * @author  AIS Team
  * @brief   MVE Image processing library resize frunctions with 8-bits
  *          inputs/outputs

  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  @verbatim
  @endverbatim
  ******************************************************************************
  */

#ifndef __MVE_RESIZE__
#define __MVE_RESIZE__

#include "arm_math.h"

/*!
 * @brief Bilinear resize from image (uint8) specifying strides
 *
 * @param [IN]  in_data: Pointer on input data
 * @param [OUT] out_ata: Pointer on output data
 * @param [IN]  stride_in: input stride for next line
 * @param [IN]  stride_out: output stride for next line
 * @param [IN]  width_in: input width
 * @param [IN]  height_in: input height
 * @param [IN]  width_out: output width
 * @param [IN]  height_out: output height
 * @param [IN]  n_channels: number of channels
 * @param [IN]  pScratch: scratch buffer to hold indexes (i.e. integer part for left and right neighbor used)
 *              and weights (i.e. decimal part) for each output uint8_t width:
 *              width_out * 2 * sizeof(uint16_t) + idth_out * * sizeof(float16_t)

 *
 * @retval None
 */
void mve_resize_bilinear_iu8ou8_with_strides(const uint8_t *in_data,
                                             uint8_t *out_data,
                                             const size_t stride_in,
                                             const size_t stride_out,
                                             const size_t width_in,
                                             const size_t height_in,
                                             const size_t width_out,
                                             const size_t height_out,
                                             const size_t n_channels,
                                             const void* pScratch);

/*!
 * @brief Nearest resize from image (uint8) specifying strides
 *
 * @param [IN]  in_data: Pointer on input data
 * @param [OUT] out_ata: Pointer on output data
 * @param [IN]  stride_in: input stride for next line
 * @param [IN]  stride_out: output stride for next line
 * @param [IN]  width_in: input width
 * @param [IN]  height_in: input height
 * @param [IN]  width_out: output width
 * @param [IN]  height_out: output height
 * @param [IN]  n_channels: number of channels
 * @param [IN]  pScratch: scratch buffer to hold indexes split in 2: offset for each of the 16 elements on 8-bits
 *              and indexes to jump to next 16 element group: for each output element:
 *              width_out * n_ch *sizeof(uint8_t) + (width_out * n_ch + 15) / 16 * sizeof(uint16_t)
 *
 * @retval None
 */
void mve_resize_nearest_iu8ou8_with_strides(const uint8_t *in_data,
                                            uint8_t *out_data,
                                            const size_t stride_in,
                                            const size_t stride_out,
                                            const size_t width_in,
                                            const size_t height_in,
                                            const size_t width_out,
                                            const size_t height_out,
                                            const size_t n_channels,
                                            const void *scratch);


#endif /* __MVE_RESIZE__ */
