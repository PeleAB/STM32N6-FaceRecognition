/**
 ******************************************************************************
 * @file    sysobj_cache.h
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

#ifndef SYSOBJ_CACHE_H
#define SYSOBJ_CACHE_H

#include <stddef.h>

int SYSOBJ_CacheIsEnabled(void);
void SYSOBJ_CacheInvalidate(void *addr, size_t len);
void SYSOBJ_CacheClean(void *addr, size_t len);
void SYSOBJ_CacheCleanInvalidate(void *addr, size_t len);

#endif /* SYSOBJ_CACHE_H */



