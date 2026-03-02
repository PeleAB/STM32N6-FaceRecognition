 /**
 ******************************************************************************
 * @file    fast_memcpy.c
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

#include <string.h>
#include <stdint.h>

#define TARGET_ALIGN_VALUE 4
#define IS_ALIGN(_v_,_a_) (!((uint32_t)(_v_) & ((_a_) - 1)))

void __attribute__ ((__optimize__ ("-fno-tree-loop-distribute-patterns"))) *memcpy(void *dest, const void *srcp, size_t n)
{
  char *dst = (char *) dest;
  char *src = (char *) srcp;

  if (IS_ALIGN(dst, TARGET_ALIGN_VALUE) && IS_ALIGN(src, TARGET_ALIGN_VALUE)) {
    uint32_t *dst_aligned = (uint32_t *) dst;
    uint32_t *src_aligned = (uint32_t *) src;

    while (n >= TARGET_ALIGN_VALUE) {
      *dst_aligned++ = *src_aligned++;

      n -= TARGET_ALIGN_VALUE;
    }

    dst = (char *) dst_aligned;
    src = (char *) src_aligned;
  }

  /* copy remining bytes */
  while (n--)
    *dst++ = *src++;

  return dest;
}

