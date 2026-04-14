/**
******************************************************************************
* @file    mve_resize.c
* @author  AIS Team
* @brief   MVE Image processing library resize functions with 8-bit
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
*/

#include <arm_math.h>
#include "stm32ipl.h"
#include "stm32ipl_imlib_int.h"

#ifdef IPL_RESIZE_HAS_MVE
#include "mve_resize.h"

#define _BILINEAR_COMPUTE_WEIGHTS_Y(Y, weights) \
    float32_t distDown = Y - (uint32_t) Y; \
    weights[0] = (1.0f - (float32_t)distDown); \
    weights[1] = (float32_t)distDown;

#define _BILINEAR_COMPUTE_WEIGHTS_X(X, weights_Y, weights) \
    float32_t distLeft = X - (uint32_t) X; \
    weights[0] = (1.0f - (float32_t)distLeft) * weights_Y[0]; \
    weights[1] = (       (float32_t)distLeft) * weights_Y[0]; \
    weights[2] = (1.0f - (float32_t)distLeft) * weights_Y[1]; \
    weights[3] = (       (float32_t)distLeft) * weights_Y[1];


static inline
void mve_bilinear_iu8ou8_kernel_RGB(const uint8_t *in_data_ptr,
                                    uint8_t *out_data,
                                    size_t inc[2],
                                    float32_t weights[4])
{
    /* Can be parallelized over channels */
    uint32x4_t  u32x4_read;
    float32x4_t f32x4_read;
    float32x4_t f32x4_tmp;
    uint32x4_t  u32x4_write;

    mve_pred16_t p = vctp32q(3);

    u32x4_read = vldrbq_u32(in_data_ptr);
    f32x4_read = vcvtq_f32_u32(u32x4_read);
    f32x4_tmp  = vmulq_n_f32(f32x4_read, weights[0]);

    u32x4_read = vldrbq_u32(in_data_ptr + inc[0]);
    f32x4_read = vcvtq_f32_u32(u32x4_read);
    f32x4_tmp  = vfmaq_n_f32(f32x4_tmp, f32x4_read, weights[1]);

    u32x4_read = vldrbq_u32(in_data_ptr + inc[1]);
    f32x4_read = vcvtq_f32_u32(u32x4_read);
    f32x4_tmp  = vfmaq_n_f32(f32x4_tmp, f32x4_read, weights[2]);

    u32x4_read = vldrbq_u32(in_data_ptr + inc[1] + inc[0]);
    f32x4_read = vcvtq_f32_u32(u32x4_read);
    f32x4_tmp  = vfmaq_n_f32(f32x4_tmp, f32x4_read, weights[3]);

    /* CLIP to uint8_t values */
    u32x4_write = vminq_u32(vcvtaq_u32_f32(f32x4_tmp), vdupq_n_u32(UINT8_MAX));

    vstrbq_p_u32(out_data, u32x4_write, p);
}


void mve_resize_bilinear_iu8ou8_with_strides(const uint8_t *in_data,
                                             uint8_t *out_data,
                                             const size_t stride_in,
                                             const size_t stride_out,
                                             const size_t width_in,
                                             const size_t height_in,
                                             const size_t width_out,
                                             const size_t height_out,
                                             const size_t channels,
                                             const void *pScratch)
{
    if (pScratch) {
        float32_t inv_width_scale, inv_height_scale;
        inv_width_scale = ((float32_t)width_in) / ((float32_t) width_out);
        inv_height_scale = ((float32_t)height_in) / ((float32_t)height_out);
        float32_t fOffset_raw = -0.5f + 0.5f * inv_width_scale;
        /* Offset for left pixel */
        uint16_t *pU16OffsetL = (uint16_t *)pScratch;
        /* Offset for right pixel */
        uint16_t *pU16OffsetR = (uint16_t *)pU16OffsetL + width_out;
        /* dist from left pixel (i.e. weight for right pixel and 1 - val for left pixel) */
        float16_t *pF16WeightR = (float16_t *)(pU16OffsetR + width_out);
        /* Same for each channel */
        for (size_t w = 0; w < width_out; w++)
        {
            float32_t fOffset = IM_MIN(IM_MAX(fOffset_raw, 0), width_in-1);
            *pU16OffsetL = ((uint16_t)IM_MIN(IM_MAX(fOffset, 0), width_in-1)) * channels;
            *pU16OffsetR++ = *pU16OffsetL + channels;
            pU16OffsetL++;
            *pF16WeightR++ = fOffset - (uint16_t)fOffset; /* decimal part, alpha */
            fOffset_raw += inv_width_scale;
        }
        uint8_t * out_data_current_ptr = out_data;
        uint8_t * out_data_ptr;
        uint16x8_t out_offset = vidupq_n_u16(0, 1);
        out_offset *= (uint16_t)channels;
        fOffset_raw = -0.5f + 0.5f * inv_height_scale;
        for (size_t h = 0; h < height_out; h++)
        {
            float32_t Y = IM_MIN(IM_MAX(fOffset_raw, 0.0f),(float32_t)height_in-1);
            fOffset_raw += inv_height_scale;
            uint32_t Yi = (uint32_t)Y;
            const uintptr_t y_step = (Yi == (height_in - 1)) ? 0 : stride_in;
            float32_t weights_Y[2];
            _BILINEAR_COMPUTE_WEIGHTS_Y(Y, weights_Y);
            uint32_t offset_y = Yi * stride_in;

            out_data_ptr = out_data_current_ptr;

            const uint8_t * in_data_ptr_0 = in_data + offset_y;
            const uint8_t * in_data_ptr_1 = in_data_ptr_0 + y_step;

            pU16OffsetL = (uint16_t *)pScratch;
            pU16OffsetR = (uint16_t *)pU16OffsetL + width_out;
            pF16WeightR = (float16_t *)(pU16OffsetR + width_out);
            int32_t w = (int32_t)width_out;
            while (w > 0)
            {
                mve_pred16_t p = vctp16q(w);
                /* Load weights */
                float16x8_t fweightsR = vldrhq_z_f16(pF16WeightR, p); /* Load 8 f16 weights */
                float16x8_t fweightsL = vsubq_f16(vdupq_n_f16(1.0), fweightsR); /* 1 - alpha */
                /* First line */
                /* apply Y weights */
                for (size_t ch = 0; ch < channels; ch++) {
                    float16x8_t f16x8_data;
                    float16x8_t f16x8_res;
                    uint16x8_t u16x8_data;

                    /* Load data for several channels, line + 0 */
                    /* Top Left */
                    u16x8_data = vldrbq_gather_offset_z_u16(in_data_ptr_0 + ch, vldrhq_u16(pU16OffsetL), p);
                    f16x8_data = vcvtq_f16_u16(u16x8_data);
                    f16x8_data = vmulq_n_f16(f16x8_data, (float16_t)weights_Y[0]);
                    f16x8_res  = vmulq_f16(f16x8_data, fweightsL);

                    /* Top Right */
                    u16x8_data = vldrbq_gather_offset_z_u16(in_data_ptr_0 + ch, vldrhq_u16(pU16OffsetR), p);
                    f16x8_data = vcvtq_f16_u16(u16x8_data);
                    f16x8_data = vmulq_n_f16(f16x8_data, (float16_t)weights_Y[0]);
                    f16x8_res  = vfmaq_f16(f16x8_res, f16x8_data, fweightsR);

                    /* Load data for several channels, line + 1 */
                    /* Down Left */
                    u16x8_data = vldrbq_gather_offset_z_u16(in_data_ptr_1 + ch, vldrhq_u16(pU16OffsetL), p);
                    f16x8_data = vcvtq_f16_u16(u16x8_data);
                    f16x8_data = vmulq_n_f16(f16x8_data, (float16_t)weights_Y[1]);
                    f16x8_res  = vfmaq_f16(f16x8_res, f16x8_data, fweightsL);

                    /* Down Right */
                    u16x8_data = vldrbq_gather_offset_z_u16(in_data_ptr_1 + ch, vldrhq_u16(pU16OffsetR), p);
                    f16x8_data = vcvtq_f16_u16(u16x8_data);
                    f16x8_data = vmulq_n_f16(f16x8_data, (float16_t)weights_Y[1]);
                    f16x8_res  = vfmaq_f16(f16x8_res, f16x8_data, fweightsR);

                    /* float to uint16 + clip to uint8 */
                    uint16x8_t u16x8_out = vcvtaq_u16_f16(f16x8_res);
                    u16x8_out = vminq_u16(u16x8_out, vdupq_n_u16(UINT8_MAX));
                    /* write 8 uint16_t to uint8_t */
                    vstrbq_scatter_offset_p_u16(out_data_ptr + ch, out_offset, u16x8_out, p);
                }
                pU16OffsetL += 8;
                pU16OffsetR += 8;
                pF16WeightR += 8;
                out_data_ptr += 8 * channels;

                w -= 8;
            }
            out_data_current_ptr += stride_out;
        } /* For height */
    }
    else
    {
        /* No scratch: indexes, weights computed each time */
        float32_t inv_width_scale, inv_height_scale;
        inv_width_scale = ((float32_t)width_in) / ((float32_t) width_out);
        inv_height_scale = ((float32_t)height_in) / ((float32_t)height_out);

        uint8_t * out_data_current_ptr = out_data;
        uint8_t * out_data_ptr;
        for (size_t h = 0; h < height_out; h++)
        {
            float32_t Y = IM_MIN(IM_MAX(-0.5f + inv_height_scale * (h + 0.5f), 0.0f),(float32_t)height_in-1);
            uint32_t Yi = (uint32_t)Y;
            const uintptr_t y_step = (Yi == (height_in - 1)) ? 0 : stride_in;
            float32_t weights_Y[2];
            _BILINEAR_COMPUTE_WEIGHTS_Y(Y, weights_Y);
            uint32_t offset_y = Yi * stride_in;

            out_data_ptr = out_data_current_ptr;
            for (size_t w = 0; w < width_out; w++)
            {
                float32_t X = IM_MIN(IM_MAX(-0.5f + inv_width_scale * (w + 0.5f), 0.0f),(float32_t)width_in-1);
                uint32_t Xi = (uint32_t)X;
                const uintptr_t x_step = (Xi == (width_in - 1)) ? 0 : 3;
                uint32_t offset = offset_y + Xi * 3;
                const uint8_t * in_data_ptr = in_data + offset;
                size_t inc[2] = { x_step, y_step};
                float32_t weights[4];
                _BILINEAR_COMPUTE_WEIGHTS_X(X, weights_Y, weights);
                mve_bilinear_iu8ou8_kernel_RGB(in_data_ptr, out_data_ptr, inc, weights);

                out_data_ptr += 3;
            }
            out_data_current_ptr += stride_out;
        }  /* for height */
    } /* else (if scratch) */
}

void mve_resize_nearest_iu8ou8_with_strides(const uint8_t *in_data,
                                            uint8_t *out_data,
                                            const size_t stride_in,
                                            const size_t stride_out,
                                            const size_t width_in,
                                            const size_t height_in,
                                            const size_t width_out,
                                            const size_t height_out,
                                            const size_t channels,
                                            const void *scratch)
{
    int32_t wRatio = (int32_t) (( width_in << 16) /  width_out) + 1;
    int32_t hRatio = (int32_t) ((height_in << 16) / height_out) + 1;

    uint8_t *out_data_current_ptr = out_data;
    uint8_t *out_data_ptr;
    if(scratch)
    {
        size_t w;

        uint16_t *pLargeReadOffs = (uint16_t *)scratch;
        uint8_t *pElOffs = (uint8_t *)(pLargeReadOffs + (width_out * channels + 15) / 16);
        uint16_t vReadOffset = 0;
        uint32_t diff_offset_x = IPL_RESIZE_PEL_IDX_ROUNDING;
        uint32_t idx = 0;
        for (w = 0; w < width_out; w++)
        {
            for (size_t ch = 0; ch < channels; ch++)
            {
                if (0 == (idx % 16)) /* Filled for a whole vector */
                {
                    vReadOffset = (diff_offset_x>>16) * channels;
                    *pLargeReadOffs++ = vReadOffset;
                }
                *pElOffs++ = (diff_offset_x >> 16) * channels - vReadOffset + ch;

                idx++;
            } /* for channels */
            diff_offset_x += wRatio;
        } /* for width */

        for (size_t h = 0; h < height_out; h++)
        {
            uint32_t offset_y = (((h * hRatio) + IPL_RESIZE_PEL_IDX_ROUNDING)>> 16) * stride_in;
            const uint8_t *in_data_curr_ptr = in_data + offset_y;

            out_data_ptr = out_data_current_ptr;
            pLargeReadOffs = (uint16_t *)scratch;
            pElOffs = (uint8_t *)(pLargeReadOffs + (width_out * channels + 15) / 16);
            int32_t w = width_out * channels;
            while(w > 0)
            {
                mve_pred16_t p = vctp8q(w);
                const uint8_t *in_data_ptr = in_data_curr_ptr + *pLargeReadOffs;

                uint8x16_t u8x16_add_offset = vldrbq_z_u8(pElOffs, p);

                /* build scatter offsets for reading */
                uint8x16_t u8x16_read = vldrbq_gather_offset_z_u8(in_data_ptr, u8x16_add_offset, p);

                vstrbq_p_u8(out_data_ptr, u8x16_read, p);

                pElOffs += 16;
                pLargeReadOffs++;

                out_data_ptr += 16;
                w -= 16;
            } /* while width */
            out_data_current_ptr += stride_out;
        } /* for height */
    }
    else
    {
        switch (channels)
        {
            case 1:
            {
                for (size_t h = 0; h < height_out; h++)
                {
                    int32_t offset_y = (((h * hRatio) + IPL_RESIZE_PEL_IDX_ROUNDING) >> 16) * stride_in;
                    const uint8_t *in_data_curr_ptr = in_data + offset_y;

                    out_data_ptr = out_data_current_ptr;
                    uint32_t offset_x = IPL_RESIZE_PEL_IDX_ROUNDING;
                    for (size_t w = 0; w < width_out; w++)
                    {
                        *out_data_ptr++ = in_data_curr_ptr[offset_x >> 16];
                        offset_x += wRatio;
                    }
                    out_data_current_ptr += stride_out;
                }
            }
            break;
            case 2:
            {
                for (size_t h = 0; h < height_out; h++)
                {
                    int32_t offset_y = (((h * hRatio) + IPL_RESIZE_PEL_IDX_ROUNDING) >> 16) * stride_in;
                    const uint8_t *in_data_curr_ptr = in_data + offset_y;
                    out_data_ptr = out_data_current_ptr;
                    uint32_t offset_x = IPL_RESIZE_PEL_IDX_ROUNDING;
                    for (size_t w = 0; w < width_out; w++) {
                        *out_data_ptr++ = in_data_curr_ptr[(offset_x >> 16) * 2 + 0];
                        *out_data_ptr++ = in_data_curr_ptr[(offset_x >> 16) * 2 + 1];
                        offset_x += wRatio;
                    }
                    out_data_current_ptr += stride_out;
                }
            }
            break;
            case 3:
            {
                for (size_t h = 0; h < height_out; h++)
                {
                    int32_t offset_y = (((h * hRatio) + IPL_RESIZE_PEL_IDX_ROUNDING) >> 16) * stride_in;
                    const uint8_t *in_data_curr_ptr = in_data + offset_y;
                    out_data_ptr = out_data_current_ptr;
                    uint32_t offset_x = IPL_RESIZE_PEL_IDX_ROUNDING;
                    for (size_t w = 0; w < width_out; w++)
                    {
                        *out_data_ptr++ = in_data_curr_ptr[(offset_x >> 16) * 3 + 0];
                        *out_data_ptr++ = in_data_curr_ptr[(offset_x >> 16) * 3 + 1];
                        *out_data_ptr++ = in_data_curr_ptr[(offset_x >> 16) * 3 + 2];
                        offset_x += wRatio;
                    }
                    out_data_current_ptr += stride_out;
                }
            }
            break;
        } /* switch */
    } /* else if scratch */
}
#endif /* IPL_RESIZE_HAS_MVE */
