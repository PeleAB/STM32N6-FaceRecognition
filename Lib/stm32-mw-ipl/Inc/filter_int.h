/**
  ******************************************************************************
  * @file    filter_int.h
  * @author  AIS Team
  * @brief   Image processing library filter functions 

  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

#ifndef __FILTER_INT_H__
#define __FILTER_INT_H__

#include "stm32ipl_imlib.h"
#include "stm32ipl_imlib_int.h"

#ifdef IMLIB_ENABLE_MEDIAN
static inline uint8_t hist_median(uint8_t *data, int len, const int cutoff)
{
  int i = 0;
#if defined(ARM_MATH_CM7) || defined(ARM_MATH_CM4) || defined(ARM_MATH_MVEI)
  uint32_t oldsum=0, sum32 = 0;

#ifdef IPL_FILTER_HAS_MVE
  for (i = 0; i < len; i += 16) { /* work 16 at time with MVE */
    sum32 += vaddvq_u8(vldrbq_u8(&data[i]));
    if ((int32_t)sum32 >= cutoff) { /* within this group */
      break;
    } /* if we're at the last 16 values */
    oldsum = sum32;
  } /* for each group of 16 elements */
  sum32 = oldsum;
#endif /* IPL_FILTER_HAS_MVE */

  for (; i < len; i += 4) { /* work 4 at time with SIMD */
    sum32 = __USADA8(*(uint32_t *)&data[i], 0, sum32);
    if ((int32_t)sum32 >= cutoff) { /* within this group */
      while ((int32_t)oldsum < cutoff && i < len)
        oldsum += data[i++];
      break;
    } /* if we're at the last 4 values */
    oldsum = sum32;
  } /* for each group of 4 elements */
#else /* generic C version */
  int sum = 0;
  for (i = 0; i < len && sum < cutoff; i++) {
    sum += data[i];
  }
#endif
  return i-1;
} /* hist_median() */
#endif /* IMLIB_ENABLE_MEDIAN */

#endif /* __FILTER_INT_H__ */
