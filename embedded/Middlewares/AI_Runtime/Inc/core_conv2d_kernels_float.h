/**
  ******************************************************************************
  * @file    core_conv2d_kernels_float.h
  * @author  AST Embedded Analytics Research Platform
  * @brief   implementation file of float conv2D kernels routines
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2018 STMicroelectronics.
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
#ifndef __CORE_CONV2D_KERNELS_FLOAT_H_
#define __CORE_CONV2D_KERNELS_FLOAT_H_


#include "ai_datatypes_defines.h"
#include "ai_math_helpers.h"
#include "core_common.h"
#include "ai_math_helpers.h"

//#if defined(HAS_X86) || defined(__CC_ARM) || defined(CM4) || defined(CM7)
//#endif

#if defined(HAS_STM32)
/*
 *  When enabled, this optimization checks that the address of
 *  the weights is located or not in the STM32 embedded/internal
 *  flash ([0x8000000, 0x9000000[). If true, weights 
 *  are not pre-fetched before to use them.
 *
 *  _OPTIM is a trade-off between the extra code size and
 *  the perf. _OPTIM_0 uses the previous kernels
 *  (not optimized for act/weighs in external memory).
 *
 *  Extra code size (GCC -Ofast, kernels are inlined:
 *    _OPTIM    500bytes
 *    _OPTIM_0  1600butes
 */
// #define _AI_STM32_CHECK_INTERNAL_FLASH_ADDRESS_OPTIM_0
#define _AI_STM32_CHECK_INTERNAL_FLASH_ADDRESS_OPTIM
#define _AI_STM32_INTERNAL_FLASH_BASE_ADDRESS   0x8000000
#define _AI_STM32_INTERNAL_FLASH_END_ADDRESS    0x9000000
#endif


/*!
 * @defgroup core_conv2d_kernels_float
 * @brief Conv2D Float Kernels Optimized Code
 * @details Conv2d kernel with weights stored following this schema (OHWI):
 * [ch_out, y, x, ch_in]
 */

/*!
  * @brief ai_conv2d_kernel_simple_f32
  * @details element wise conv2d internal kernel
  * @param node
  * @param out_data
  * @param in_data
  * @param weights_data
  * @param bias_data
  * @param beta multiplier of C
  * @param n_channel_in
  * @param n_channel_out
  * @param width_in
  * @param filt_width
  * @param filt_height
  * @param y_start
  * @param y_size
  * @param x_start
  * @param x_size
  */
AI_DECLARE_STATIC
void ai_conv2d_kernel_simple_f32(ai_node* node,
                                 ai_handle out_data,
                                 const ai_handle in_data,
                                 const ai_handle weights_data,
                                 const ai_handle bias,
                                 const ai_size n_channel_in,
                                 const ai_size n_channel_out,
                                 const ai_size width_in,
                                 const ai_size filt_width,
                                 const ai_size filt_height,
                                 ai_i32 y_start, const ai_i32 y_size,
                                 ai_i32 x_start, const ai_i32 x_size)
{
  /* Pre-calculate offsets */
  const ai_offset ch_y_offset = (width_in - x_size) * n_channel_in;
  const ai_offset ch_weights_offset = n_channel_in * (filt_width - x_size);
  const ai_offset ch_weights_offset_2 = n_channel_in * filt_width *
                                              (filt_height - y_size);
  const ai_float *ch_weights = ((const ai_float*)weights_data) +
                (y_start * filt_width + x_start) * n_channel_in;
  const ai_offset inner_loop_size = n_channel_in * x_size;

  /* Filtering */
  const ai_float bias_factor = (bias==out_data) ? 0.0f : 1.0f;

  const ai_float* in_ptr = ((const ai_float*)in_data) +
                            y_start *width_in * n_channel_in;

  for (ai_size out = 0; out < n_channel_out; out++)
  {
    ai_float conv_out=((const ai_float*)bias)[out] * bias_factor;
    const ai_float *ch_in = in_ptr;

    for (ai_i32 y_filt = 0; y_filt < y_size; y_filt++) {
      ai_i32 x_filt = 0;
#if defined(_AI_CONV_2D_LOOP_UNROLLING_OPTIM)
      for ( ; x_filt<(inner_loop_size&(~0x3)); x_filt+=4 )
      {
        register ai_vec4_float ch_in_f = AI_VEC4_FLOAT(ch_in);
        register ai_vec4_float weights_in_f = AI_VEC4_FLOAT(ch_weights);
        conv_out += weights_in_f.a1 * ch_in_f.a1;
        conv_out += weights_in_f.a2 * ch_in_f.a2;
        conv_out += weights_in_f.a3 * ch_in_f.a3;
        conv_out += weights_in_f.a4 * ch_in_f.a4;
        ch_in    += 4;
        ch_weights += 4;
      }
#endif
     for ( ; x_filt<inner_loop_size; x_filt++) {
        conv_out += (*ch_weights++) * (*ch_in++);
      }
      ch_in      += ch_y_offset;
      ch_weights += ch_weights_offset;
    }
    ch_weights += ch_weights_offset_2;

    AI_PUSH_CONV_OUT(node, &conv_out, ((ai_float*)out_data)+out, ai_float*)
    AI_UNUSED(conv_out)
  }
}


/*!
  * @brief ai_conv2d_kernel_simple_opt_f32
  * @details element wise conv2d internal kernel
  * @param node
  * @param out_data
  * @param in_data
  * @param weights_data
  * @param bias
  * @param beta multiplier of C
  * @param n_channel_in
  * @param n_channel_out
  * @param width_in
  * @param width_out
  * @param filt_width
  * @param filt_height
  * @param y_start
  * @param y_size
  * @param x_in_init
  * @param filt_stride_x
  */
AI_DECLARE_STATIC
void ai_conv2d_kernel_simple_opt_f32(ai_node* node,
                                 ai_handle out_data,
                                 const ai_handle in_data,
                                 const ai_handle weights_data,
                                 const ai_float bias,
                                 const ai_size n_channel_in,
                                 const ai_size n_channel_out,
                                 const ai_size width_in,
                                 const ai_size width_out,
                                 const ai_size filt_width,
                                 const ai_size filt_height,
                                 ai_i32 y_start, const ai_i32 y_size,
                                 ai_i32 x_in_init, ai_u16 filt_stride_x)
{
  ai_i32 x_in = x_in_init;
  const ai_float *w_base = ((const ai_float*)weights_data) + 
                                      (y_start * filt_width) * n_channel_in;
  for (ai_size out = 0; out < width_out; out++) 
  {
    /* Handle X padding */
    const ai_i32 x_start = AI_MAX(-x_in, 0);
    const ai_i32 x_end = AI_MIN(width_in-x_in, filt_width);
    ai_i32 o_base = (x_in + x_start) * n_channel_in;
    const ai_float *ch_in = ((const ai_float *)in_data) + o_base;
    const ai_float *ch_weights = ((const ai_float*)w_base) +
                                        (x_start * n_channel_in);
      
    const ai_i32 x_size = (x_end - x_start);
    AI_ASSERT(x_size>=0)
 
    /* Pre-calculate offsets */
    const ai_offset ch_y_offset = (width_in - x_size) * n_channel_in;
    const ai_offset ch_weights_offset = n_channel_in * (filt_width - x_size);
    const ai_offset inner_loop_size = n_channel_in * x_size;
    ai_float conv_out = bias;

    for (ai_i32 y_filt = 0; y_filt < y_size; y_filt++) 
    {
      ai_i32 x_filt = 0;
#if defined(_AI_CONV_2D_LOOP_UNROLLING_OPTIM)
      for ( ; x_filt < (inner_loop_size&(~0xF)); x_filt += 16 )
      {
        register ai_vec4_float ch_in_f = AI_VEC4_FLOAT(ch_in);
        register ai_vec4_float weights_in_f = AI_VEC4_FLOAT(ch_weights);
        conv_out += weights_in_f.a1 * ch_in_f.a1;
        conv_out += weights_in_f.a2 * ch_in_f.a2;
        conv_out += weights_in_f.a3 * ch_in_f.a3;
        conv_out += weights_in_f.a4 * ch_in_f.a4;
        ch_in    += 4;
        ch_weights += 4;

        ch_in_f = AI_VEC4_FLOAT(ch_in);
        weights_in_f = AI_VEC4_FLOAT(ch_weights);
        conv_out += weights_in_f.a1 * ch_in_f.a1;
        conv_out += weights_in_f.a2 * ch_in_f.a2;
        conv_out += weights_in_f.a3 * ch_in_f.a3;
        conv_out += weights_in_f.a4 * ch_in_f.a4;
        ch_in    += 4;
        ch_weights += 4;

        ch_in_f = AI_VEC4_FLOAT(ch_in);
        weights_in_f = AI_VEC4_FLOAT(ch_weights);
        conv_out += weights_in_f.a1 * ch_in_f.a1;
        conv_out += weights_in_f.a2 * ch_in_f.a2;
        conv_out += weights_in_f.a3 * ch_in_f.a3;
        conv_out += weights_in_f.a4 * ch_in_f.a4;
        ch_in    += 4;
        ch_weights += 4;
        
        ch_in_f = AI_VEC4_FLOAT(ch_in);
        weights_in_f = AI_VEC4_FLOAT(ch_weights);
        conv_out += weights_in_f.a1 * ch_in_f.a1;
        conv_out += weights_in_f.a2 * ch_in_f.a2;
        conv_out += weights_in_f.a3 * ch_in_f.a3;
        conv_out += weights_in_f.a4 * ch_in_f.a4;
        ch_in    += 4;
        ch_weights += 4;
      }
#else
      for ( ; x_filt < (inner_loop_size&(~0x3)); x_filt += 4 )
      {
        register ai_vec4_float ch_in_f = AI_VEC4_FLOAT(ch_in);
        register ai_vec4_float weights_in_f = AI_VEC4_FLOAT(ch_weights);
        conv_out += weights_in_f.a1 * ch_in_f.a1;
        conv_out += weights_in_f.a2 * ch_in_f.a2;
        conv_out += weights_in_f.a3 * ch_in_f.a3;
        conv_out += weights_in_f.a4 * ch_in_f.a4;
        ch_in    += 4;
        ch_weights += 4;
      }
#endif
      for ( ; x_filt < inner_loop_size; x_filt++) 
      {
        conv_out += (*ch_weights++) * (*ch_in++);
      }
      ch_in      += ch_y_offset;
      ch_weights += ch_weights_offset;
    }

    AI_PUSH_CONV_OUT(node, &conv_out, ((ai_float *)out_data), ai_float *)
    out_data = (ai_float *)out_data + n_channel_out;
    SECTION_SERIAL(x_in += filt_stride_x;)
    AI_UNUSED(conv_out)
  }
}


/*!
  * @brief ai_conv2d_kernel_depthwise_f32
  * @details 
  * @param node
  * @param out_data
  * @param in_data
  * @param weights_data
  * @param bias_data
  * @param beta multiplier of C
  * @param n_channel_in
  * @param n_channel_out
  * @param width_in
  * @param filt_width
  * @param filt_height
  * @param y_start
  * @param y_size
  * @param x_start
  * @param x_size
  */
AI_DECLARE_STATIC
void ai_conv2d_kernel_depthwise_f32(ai_node* node,
                                    ai_handle out_data,
                                    const ai_handle in_data,
                                    const ai_handle weights_data,
                                    const ai_handle bias,
                                    const ai_size n_channel_in,
                                    const ai_size n_channel_out,
                                    const ai_size width_in,
                                    const ai_size filt_width,
                                    const ai_size filt_height,
                                    ai_i32 y_start, const ai_i32 y_size,
                                    ai_i32 x_start, const ai_i32 x_size)
{
  /* Pre-calculate offsets */
  const ai_size n_channel_out_for_group = n_channel_out / n_channel_in;
  const ai_offset ch_y_offset =  (width_in - x_size) * n_channel_in;
  const ai_offset ch_weights_offset = (filt_width - x_size);
  const ai_offset ch_weights_offset_2 = filt_width * (filt_height - y_size);
  const ai_float *ch_weights = ((const ai_float*)weights_data) + 
                               (y_start * filt_width + x_start);

  /* Filtering */
  const ai_float bias_factor = (bias==out_data) ? 0.0f : 1.0f;

  const ai_float* in_ptr = ((const ai_float*)in_data) + 
                           y_start*width_in*n_channel_in;
  ai_size out = 0;

  for (const ai_float* in_curr=in_ptr; in_curr<in_ptr+n_channel_in; in_curr++)
  {
    for (ai_size i = 0; i < n_channel_out_for_group; i++) {

      ai_float conv_out = ((const ai_float*)bias)[out] * bias_factor;

      const ai_float *ch_in = in_curr;
      for (ai_i32 y_filt = 0; y_filt < y_size; y_filt++) {
        ai_i32 x_filt = 0;
#if defined(_AI_CONV_2D_LOOP_UNROLLING_OPTIM)
        for ( ; x_filt<(x_size&(~0x3)); x_filt+=4 )
        {
          register ai_vec4_float weights_in_f = AI_VEC4_FLOAT(ch_weights);
          conv_out += weights_in_f.a1 * (*ch_in); ch_in += n_channel_in;
          conv_out += weights_in_f.a2 * (*ch_in); ch_in += n_channel_in;
          conv_out += weights_in_f.a3 * (*ch_in); ch_in += n_channel_in;
          conv_out += weights_in_f.a4 * (*ch_in); ch_in += n_channel_in;
          ch_weights += 4;
        }
#endif
        for( ; x_filt < x_size; x_filt++) {
          conv_out += (*ch_weights++) * (*ch_in);
          ch_in    += n_channel_in;
        }
        ch_in      += ch_y_offset;
        ch_weights += ch_weights_offset;
      }
      ch_weights += ch_weights_offset_2;
      AI_PUSH_CONV_OUT(node, &conv_out, ((ai_float*)out_data+out), ai_float*)
      out++;
      AI_UNUSED(conv_out)
    }
  }
}


/*!
  * @brief ai_conv2d_kernel_group_f32
  * @details conv2d group conv2d kernel
  * @param node
  * @param out_data
  * @param in_data
  * @param weights_data
  * @param bias_data
  * @param beta multiplier of C
  * @param n_channel_in
  * @param n_channel_out
  * @param width_in
  * @param filt_width
  * @param filt_height
  * @param y_start
  * @param y_size
  * @param x_start
  * @param x_size
  */
AI_DECLARE_STATIC
void ai_conv2d_kernel_group_f32(ai_node* node,
                                ai_handle out_data,
                                const ai_handle in_data,
                                const ai_handle weights_data,
                                const ai_handle bias,
                                const ai_size n_channel_in,
                                const ai_size n_channel_out,
                                const ai_size width_in,
                                const ai_i16  dilation_x,
                                const ai_i16  dilation_y,
                                const ai_size filt_width,
                                const ai_size filt_height,
                                const ai_size n_groups,
                                ai_i32 y_start, const ai_i32 y_size,
                                ai_i32 x_start, const ai_i32 x_size)
{
  /* With padding, dilated convolutions may not have the first non-zero
   * coefficient at the image border; adjusting start indices to account for it
   */
  const ai_offset x_shift = (dilation_x-x_start%dilation_x) % dilation_x;
  const ai_offset y_shift = (dilation_y-y_start%dilation_y) % dilation_y;
  /* Kernel start indices and size (in the weight buffer) don't have dilation,
   * so we recover the original (undilated) values. Moreover, the effective
   * kernel size is affected by the start index shift, so we adjust for it */
  const ai_i32 x_start_kernel = (x_start+dilation_x-1)/dilation_x;
  const ai_i32 y_start_kernel = (y_start+dilation_y-1)/dilation_y;
  const ai_i32 x_size_kernel  = (x_size-x_shift+dilation_x-1)/dilation_x;
  const ai_i32 y_size_kernel  = (y_size-y_shift+dilation_y-1)/dilation_y;
  /* Pre-calculate offsets */
  const ai_size n_channel_in_for_group = n_channel_in / n_groups;
  const ai_size n_channel_out_for_group = n_channel_out / n_groups;
  /* Pre-calculate offsets
   * The values depend on the number of steps in the inner loop, which
   * corresponds to the weight elements we access; so we use e.g. x_size_kernel
   * to estimate the offset size
   */
  const ai_offset ch_y_offset = (dilation_y * width_in - dilation_x * x_size_kernel) * n_channel_in;
  const ai_offset ch_in_grp_offset = dilation_x*n_channel_in-n_channel_in_for_group;

  const ai_offset ch_weights_offset =
        n_channel_in_for_group * (filt_width - x_size_kernel);
  const ai_offset ch_weights_offset_2 =
        n_channel_in_for_group * filt_width * (filt_height - y_size_kernel);
  const ai_float *ch_weights = ((const ai_float*)weights_data) +
                      (y_start_kernel * filt_width + x_start_kernel) * n_channel_in_for_group;
  
  /* Filtering */
  const ai_float bias_factor = (bias==out_data) ? 0.0f : 1.0f;
  const ai_float* in_ptr = ((const ai_float*)in_data) +
                           ((y_start+y_shift)*width_in+x_shift)*n_channel_in;
  ai_size out = 0;
  for(ai_size group = 0; group < n_groups; group++)
  {
    for (ai_size i = 0; i < n_channel_out_for_group; i++) {
      ai_float conv_out = ((const ai_float*)bias)[out] * bias_factor;
      const ai_float *ch_in = in_ptr;

      for (ai_i32 y_filt = 0; y_filt < y_size_kernel; y_filt++) {
        for(ai_i32 x_filt = 0; x_filt < x_size_kernel; x_filt++) {
          const ai_float* ch_in_end = ch_in + n_channel_in_for_group;
          for ( ; ch_in<ch_in_end; ch_in++) {
            conv_out += (*ch_weights++) * (*ch_in);
          }
          ch_in += ch_in_grp_offset;
        }
        ch_in += ch_y_offset;
        ch_weights += ch_weights_offset;
      }
      ch_weights += ch_weights_offset_2;
      AI_PUSH_CONV_OUT(node, &conv_out, ((ai_float*)out_data)+out, ai_float*)
      out++;
      AI_UNUSED(conv_out)
    }
    in_ptr += n_channel_in_for_group;
  }
}


/*!
  * @brief ai_conv2d_kernel_f32
  * @details main conv2d kernel routine
  * @param node
  * @param out_data
  * @param in_data
  * @param weights_data
  * @param bias_data
  * @param beta multiplier of C
  * @param n_channel_in
  * @param n_channel_out
  * @param width_in
  * @param filt_width
  * @param filt_height
  * @param y_start
  * @param y_end
  * @param x_start
  * @param x_end
  */
AI_DECLARE_STATIC
void ai_conv2d_kernel_f32(ai_node* node,
                          ai_handle out_data,
                          const ai_handle in_data,
                          const ai_handle weights_data,
                          const ai_handle bias_data,
                          const ai_size n_channel_in,
                          const ai_size n_channel_out,
                          const ai_size width_in,
                          const ai_i16 dilation_x,
                          const ai_i16 dilation_y,
                          const ai_size filt_width,
                          const ai_size filt_height,
                          const ai_size n_groups,
                          ai_i32 y_start, const ai_i32 y_end,
                          ai_i32 x_start, const ai_i32 x_end)
{
  /* avoid null pointer exceptions in called routines */
  const ai_handle bias = (bias_data) ? bias_data : out_data;

  const ai_i32 y_size = (y_end - y_start);
  const ai_i32 x_size = (x_end - x_start);

  AI_ASSERT(in_data && out_data && weights_data && bias)
  AI_ASSERT(x_size>=0 && y_size>=0)

  if (n_groups == 1 && (dilation_x*dilation_y)==1) {
    /* ordinary convolutional layer */
    ai_conv2d_kernel_simple_f32(node, out_data, in_data, weights_data, bias,
                                n_channel_in, n_channel_out, width_in,
                                filt_width, filt_height,
                                y_start, y_size, x_start, x_size);
  } else if (n_groups == n_channel_in && (dilation_x*dilation_y)==1) {
    /* depthwise separable convolutional layer */
    ai_conv2d_kernel_depthwise_f32(node, out_data, in_data, weights_data, bias,
                                   n_channel_in, n_channel_out, width_in,
                                   filt_width, filt_height,
                                   y_start, y_size, x_start, x_size);
  } else {
    /* convolutional layer with groups (general case) */
    ai_conv2d_kernel_group_f32(node, out_data, in_data, weights_data, bias,
                               n_channel_in, n_channel_out, width_in,
                               dilation_x,dilation_y,
                               filt_width, filt_height,n_groups,
                               y_start, y_size, x_start, x_size);
  }
}


/*!
  * @brief ai_conv2d_stripe_f32
  * @details main conv2d stripe routine
  * @param node
  * @param out_data
  * @param in_data
  * @param weights_data
  * @param bias_data
  * @param n_channel_in
  * @param n_channel_out
  * @param width_in
  * @param width_out
  * @param filt_width
  * @param filt_height
  * @param filt_stride_x
  * @param n_groups
  * @param y_start
  * @param y_end
  * @param x_in
  * @param y_in
  */
AI_DECLARE_STATIC
void ai_conv2d_stripe_f32(ai_node* node,
                          ai_handle out_data,
                          const ai_handle in_data,
                          const ai_handle weights_data,
                          const ai_handle bias_data,
                          const ai_size n_channel_in,
                          const ai_size n_channel_out,
                          const ai_size width_in,
                          const ai_size width_out,
                          const ai_size filt_width,
                          const ai_size filt_height,
                          const ai_u16 filt_stride_x, 
                          const ai_size n_groups,
                          ai_i32 y_start, const ai_i32 y_end,
                          ai_i32 x_in, const ai_i32 y_in, 
                          const ai_size filt_width_dilated, 
                          const ai_i16 dilation_x,
                          const ai_i16 dilation_y)
{
#if 1

#if defined(_AI_STM32_CHECK_INTERNAL_FLASH_ADDRESS_OPTIM_0)
  if (((uint32_t)weights_data >= _AI_STM32_INTERNAL_FLASH_BASE_ADDRESS) &&
      ((uint32_t)weights_data < _AI_STM32_INTERNAL_FLASH_END_ADDRESS))
  {

    for (ai_size x_out = 0; x_out < width_out; ++x_out)
    {
      SECTION_PARALLEL(const ai_i32 x_in = -filt_pad_x + x_out * filt_stride_x;)

      /* Handle X padding */
      const ai_i32 x_start = AI_MAX(-x_in, 0);
      const ai_i32 x_end = AI_MIN(width_in-x_in, filt_width_dilated);
      const ai_i32 o_base = (y_in*width_in+x_in+x_start) * n_channel_in;
      const ai_handle in_data_px = (ai_float *)in_data + o_base;

      ai_conv2d_kernel_f32(node, AI_HANDLE_PTR(out_data),
          AI_HANDLE_PTR(in_data_px),
          weights_data, bias_data,
          n_channel_in, n_channel_out, width_in,
          dilation_x, dilation_y,
          filt_width, filt_height,n_groups,
          y_start, y_end, x_start, x_end);

      out_data = (ai_float *)out_data + n_channel_out;
      SECTION_SERIAL(x_in += filt_stride_x;)
    }
   return;
  }
#endif

  if (n_groups == 1 && (dilation_x*dilation_y)==1) 
  {
    ai_float *weights_data_px = (ai_float *)weights_data;
    const ai_float *bias_ptr = (const ai_float *)bias_data;
    ai_float *out_ptr = (ai_float *)out_data;
    ai_i32 o_base_1 = ((y_in + y_start) * width_in) * n_channel_in;
    const ai_float *in_data_px = (const ai_float *)in_data + o_base_1;
    /* ordinary convolutional layer */
    const ai_i32 y_size = (y_end - y_start);
    AI_ASSERT(y_size>=0)

#if defined(_AI_STM32_CHECK_INTERNAL_FLASH_ADDRESS_OPTIM)
    if (((uint32_t)weights_data >= _AI_STM32_INTERNAL_FLASH_BASE_ADDRESS) &&
          ((uint32_t)weights_data < _AI_STM32_INTERNAL_FLASH_END_ADDRESS))
    {
      /* Weights are directly fetched in the internal flash */
      for (ai_size x_out = 0; x_out < n_channel_out; x_out++)
      {
        ai_float bias = (bias_data) ? *bias_ptr++ : 0.0f;
        AI_ASSERT(in_data && out_data && weights_data)
        ai_conv2d_kernel_simple_opt_f32(node, AI_HANDLE_PTR(out_ptr),
            AI_HANDLE_PTR(in_data_px),
            weights_data_px, bias,
            n_channel_in, n_channel_out, width_in, width_out,
            filt_width, filt_height,
            y_start, y_size, x_in, filt_stride_x);

        weights_data_px += (filt_width * filt_height * n_channel_in);
        out_ptr++;
      }
    }
    else
    {
#endif
      /* Weights are fetched in the cache (weights_data_sp) before to call the
       * kernel loop. */
      ai_float weights_data_sp[filt_width * filt_height * n_channel_in];

      for (ai_size x_out = 0; x_out < n_channel_out; x_out++)
      {
        for (ai_u32 i = 0; i < (filt_width * filt_height * n_channel_in); i++)
        {
          weights_data_sp[i] = weights_data_px[i];
        }
        ai_float bias = (bias_data) ? *bias_ptr++ : 0.0f;
        AI_ASSERT(in_data && out_data && weights_data)
        ai_conv2d_kernel_simple_opt_f32(node, AI_HANDLE_PTR(out_ptr),
            AI_HANDLE_PTR(in_data_px),
            weights_data_sp, bias,
            n_channel_in, n_channel_out, width_in, width_out,
            filt_width, filt_height,
            y_start, y_size, x_in, filt_stride_x);

        weights_data_px += (filt_width * filt_height * n_channel_in);
        out_ptr++;
      }
#if defined(_AI_STM32_CHECK_INTERNAL_FLASH_ADDRESS_OPTIM)
    }
#endif
  } 
  else if (n_groups == n_channel_in && (dilation_x*dilation_y)==1) 
  {
    for (ai_size x_out = 0; x_out < width_out; ++x_out) 
    {
      SECTION_PARALLEL(const ai_i32 x_in = -filt_pad_x + x_out * filt_stride_x;)

      /* Handle X padding */
      const ai_i32 x_start = AI_MAX(-x_in, 0);
      const ai_i32 x_end = AI_MIN(width_in-x_in, filt_width_dilated);
      const ai_i32 o_base = (y_in*width_in+x_in+x_start) * n_channel_in;
      const ai_handle in_data_px = (ai_float *)in_data + o_base;
    
      /* avoid null pointer exceptions in called routines */
      const ai_handle bias = (bias_data) ? bias_data : out_data;
      const ai_i32 y_size = (y_end - y_start);
      const ai_i32 x_size = (x_end - x_start);
      AI_ASSERT(in_data && out_data && weights_data && bias)
      AI_ASSERT(x_size>=0 && y_size>=0)
      /* depthwise separable convolutional layer */
      ai_conv2d_kernel_depthwise_f32(node, AI_HANDLE_PTR(out_data),
                                     AI_HANDLE_PTR(in_data_px), weights_data, bias,
                                     n_channel_in, n_channel_out, width_in,
                                     filt_width, filt_height,
                                     y_start, y_size, x_start, x_size);
      
      out_data = (ai_float *)out_data + n_channel_out;
      SECTION_SERIAL(x_in += filt_stride_x;)
    }
  } 
  else 
  {
    for (ai_size x_out = 0; x_out < width_out; ++x_out) 
    {
      SECTION_PARALLEL(const ai_i32 x_in = -filt_pad_x + x_out * filt_stride_x;)

      /* Handle X padding */
      const ai_i32 x_start = AI_MAX(-x_in, 0);
      const ai_i32 x_end = AI_MIN(width_in-x_in, filt_width_dilated);
      const ai_i32 o_base = (y_in*width_in+x_in+x_start) * n_channel_in;
      const ai_handle in_data_px = (ai_float *)in_data + o_base;
    
      /* avoid null pointer exceptions in called routines */
      const ai_handle bias = (bias_data) ? bias_data : out_data;
      const ai_i32 y_size = (y_end - y_start);
      const ai_i32 x_size = (x_end - x_start);
      AI_ASSERT(in_data && out_data && weights_data && bias)
      AI_ASSERT(x_size>=0 && y_size>=0)
      /* convolutional layer with groups (general case) */
      ai_conv2d_kernel_group_f32(node, AI_HANDLE_PTR(out_data),
                                 AI_HANDLE_PTR(in_data_px), weights_data, bias,
                                 n_channel_in, n_channel_out, width_in,
                                 dilation_x,dilation_y,
                                 filt_width, filt_height,n_groups,
                                 y_start, y_size, x_start, x_size);
      out_data = (ai_float *)out_data + n_channel_out;
      SECTION_SERIAL(x_in += filt_stride_x;)
    }
  }

  
#else  /* # if 0 */
  
  for (ai_size x_out = 0; x_out < width_out; ++x_out) 
  {
    SECTION_PARALLEL(const ai_i32 x_in = -filt_pad_x + x_out * filt_stride_x;)

    /* Handle X padding */
    const ai_i32 x_start = AI_MAX(-x_in, 0);
    const ai_i32 x_end = AI_MIN(width_in-x_in, filt_width_dilated);
    const ai_i32 o_base = (y_in*width_in+x_in+x_start) * n_channel_in;
    const ai_handle in_data_px = (ai_float *)in_data + o_base;

    ai_conv2d_kernel_f32(node, AI_HANDLE_PTR(out_data),
                         AI_HANDLE_PTR(in_data_px),
                         weights_data, bias_data,
                         n_channel_in, n_channel_out, width_in,
                         dilation_x, dilation_y,
                         filt_width, filt_height,n_groups,
                         y_start, y_end, x_start, x_end);      
    
    out_data = (ai_float *)out_data + n_channel_out;
    SECTION_SERIAL(x_in += filt_stride_x;)
  }
#endif
}


#undef AI_PUSH_CONV_OUT

#endif    /*__CORE_CONV2D_KERNELS_FLOAT_H_*/
