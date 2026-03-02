 /**
 ******************************************************************************
 * @file    utils.h
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

#ifndef UTILS
#define UTILS

#include "ll_aton_rt_user_api.h"

#define ALIGN_32 __attribute__ ((aligned (32)))
#define IN_PSRAM __attribute__ ((section (".psram_bss")))
#define UNCACHED __attribute__ ((section (".uncached_bss")))
#define WEAK __weak

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

#define ARRAY_NB(a) (sizeof(a)/sizeof(a[0]))

void Run_Inference(NN_Instance_TypeDef *network_instance);

#endif
