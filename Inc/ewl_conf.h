/**
  ******************************************************************************
  * @file    EWL_conf_template.h
  * @author  MCD Application Team
  * @brief   EWL configuration template file.
  *          This file should be copied to the application folder and renamed
  *          to EWL_conf.h.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef EWL_CONF_H
#define EWL_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/


#define EWL_USE_MALLOC_MM 0
#define EWL_USE_FREERTOS_MM 1
#define EWL_USE_THREADX_MM 2
#define EWL_USE_STM32MPM_MM 3
#define EWL_USER_MM 4


/* change the value of this macro to select memory manager */
#define EWL_ALLOC_API EWL_USER_MM

#define EWL_USE_POLLING_SYNC 0
#define EWL_USE_FREERTOS_SYNC 1
#define EWL_USE_THREADX_SYNC 2
#define EWL_USER_SYNC 4


/* change the value of this macro to select memory manager */
#define EWL_SYNC_API EWL_USE_FREERTOS_SYNC

//#define TRACE_EWL

#define ALIGNMENT_INCR 8UL

#define MEM_CHUNKS      32

/* Includes ------------------------------------------------------------------*/
/**
  * @brief Include module's header file
  */

#if (EWL_ALLOC_API == MALLOC_MM)
#include <stdlib.h>
#endif /* (EWL_ALLOC_API == MALLOC_MM) */

#if (EWL_ALLOC_API == EWL_USE_FREERTOS_MM) || (EWL_SYNC_API == EWL_USE_FREERTOS_SYNC)
#include "cmsis_os.h"
#endif /*(EWL_ALLOC_API == EWL_USE_FREERTOS_MM) || (EWL_SYNC_API == EWL_USE_FREERTOS_SYNC) */

#if (EWL_ALLOC_API == EWL_USE_THREADX_MM) || (EWL_SYNC_API == EWL_USE_THREADX_SYNC)
#include "tx_api.h"
#endif /* (EWL_ALLOC_API == EWL_USE_THREADX_MM) || (EWL_SYNC_API == EWL_USE_THREADX_SYNC) */

#if (EWL_ALLOC_API == EWL_USE_STM32MPM_MM)
#error "add proper include here"
#include ""
#endif /* EWL_ALLOC_API == EWL_USE_STM32MPM_MM */

/* Exported macro ------------------------------------------------------------*/
#ifdef  USE_FULL_ASSERT
/**
  * @brief  The assert_param macro is used for function's parameters check.
  * @param  expr: If expr is false, it calls assert_failed function
  *         which reports the name of the source file and the source
  *         line number of the call that failed.
  *         If expr is true, it returns no value.
  * @retval None
  */
#define assert_param(expr) ((expr) ? (void)0U : assert_failed((uint8_t *)__FILE__, __LINE__))
/* Exported functions ------------------------------------------------------- */
void assert_failed(uint8_t *file, uint32_t line);
#else
#define assert_param(expr) ((void)0U)
#endif /* USE_FULL_ASSERT */

#ifdef __cplusplus
}
#endif

#endif /* STM32N6xx_HAL_CONF_H */

