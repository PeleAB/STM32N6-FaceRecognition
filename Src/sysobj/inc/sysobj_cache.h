/**
 ******************************************************************************
 * @file    fal_cache.h
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

#ifndef FAL_CACHE_H
#define FAL_CACHE_H

#include <stddef.h>

int FAL_CacheIsEnabled(void);
void FAL_CacheInvalidate(void *addr, size_t len);
void FAL_CacheClean(void *addr, size_t len);
void FAL_CacheCleanInvalidate(void *addr, size_t len);

#endif /* FAL_CACHE_H */
