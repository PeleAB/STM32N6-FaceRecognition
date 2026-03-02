/**
 ******************************************************************************
 * @file    fal_cache.c
 * @author  GPM Application Team
 *
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

#include "fal_cache.h"

#include <stdint.h>

#include "stm32n6xx_hal.h"

#define CACHE_LINE_SIZE 32U

static void cache_align_region(uintptr_t *addr, size_t *len)
{
  uintptr_t aligned_addr = *addr & ~(CACHE_LINE_SIZE - 1U);
  size_t aligned_len = *len + (*addr - aligned_addr);

  aligned_len = (aligned_len + CACHE_LINE_SIZE - 1U) & ~(CACHE_LINE_SIZE - 1U);

  *addr = aligned_addr;
  *len = aligned_len;
}

int FAL_CacheIsEnabled(void)
{
#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
  return (SCB->CCR & SCB_CCR_DC_Msk) != 0U;
#else
  return 0;
#endif
}

void FAL_CacheInvalidate(void *addr, size_t len)
{
  uintptr_t target = (uintptr_t) addr;

  if (!FAL_CacheIsEnabled() || len == 0U)
    return;

  cache_align_region(&target, &len);
  SCB_InvalidateDCache_by_Addr((void *) target, (int32_t) len);
}

void FAL_CacheClean(void *addr, size_t len)
{
  uintptr_t target = (uintptr_t) addr;

  if (!FAL_CacheIsEnabled() || len == 0U)
    return;

  cache_align_region(&target, &len);
  SCB_CleanDCache_by_Addr((void *) target, (int32_t) len);
}

void FAL_CacheCleanInvalidate(void *addr, size_t len)
{
  uintptr_t target = (uintptr_t) addr;

  if (!FAL_CacheIsEnabled() || len == 0U)
    return;

  cache_align_region(&target, &len);
  SCB_CleanInvalidateDCache_by_Addr((void *) target, (int32_t) len);
}

