/**
  ******************************************************************************
  * @file    ewl_impl.h
  * @author  GPM Application Team
  * @brief   header for ewl_impl.c
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
#ifndef __EWL_IMPL_H__
#define __EWL_IMPL_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "ewl_conf.h"

/* Exported types ------------------------------------------------------------*/

typedef struct
{
  u32 clientType;
  u8 *pool;
  u32 pool_size;
#if (EWL_ALLOC_API == EWL_USE_MALLOC_MM)
  /* tables required for buffer alignment when using malloc */
  u32 *chunks[MEM_CHUNKS];
  u32 *alignedChunks[MEM_CHUNKS];
  u32 totalChunks;
#elif (EWL_ALLOC_API == EWL_USE_FREERTOS_MM)
  /* no instance definitions required for freertos MM */
#elif (EWL_ALLOC_API == EWL_USE_THREADX_MM)
  /* byte pool structure for threadx memory management */
  TX_BYTE_POOL byte_pool;
    /* tables required for buffer alignment when using threadx */
  u32 *chunks[MEM_CHUNKS];
  u32 *alignedChunks[MEM_CHUNKS];
  u32 totalChunks;
#elif (EWL_ALLOC_API == EWL_USE_STM32MPM_MM)
#error "no STM32MPM yet"
#elif (EWL_ALLOC_API == EWL_USER_MM)
#else
#error "incorrect EWL memory management configuration"
#endif /* EWL_ALLOC_API */

#if (EWL_SYNC_API == EWL_USE_POLLING_SYNC)
#elif (EWL_SYNC_API == EWL_USE_FREERTOS_SYNC)
  EventGroupHandle_t ewl_event_group;
#elif (EWL_SYNC_API == EWL_USE_THREADX_SYNC)
  /* event flags structure for Threadx synchronization */
  TX_EVENT_FLAGS_GROUP flag_events;
#endif /* EWL_SYNC_API */
} VENC_EWL_TypeDef;

/* Exported constants --------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported macros -----------------------------------------------------------*/

#define ALIGNED_SIZE(x) ((((x) + ALIGNMENT_INCR - 1UL) / ALIGNMENT_INCR) * ALIGNMENT_INCR)

/* Macro for debug printing */
#undef PTRACE
#ifdef TRACE_EWL
#include <stdio.h>
#define PTRACE(...)                           \
  do                                        \
  {                                         \
    printf("%s:%d:", __FILE__, __LINE__); \
    printf(__VA_ARGS__);                  \
  } while (0)
#else
#define PTRACE(...) /* no trace */
#endif /* TRACE_EWL */



/* Exported functions ------------------------------------------------------- */

void EWLPoolChoiceCb(uint8_t **pool_ptr, size_t *size);
void EWLPoolReleaseCb(uint8_t **pool_ptr);
void VENC_IRQHandler(void);

#ifdef __cplusplus
}
#endif
#endif /* __EWL_IMPL_H__ */

