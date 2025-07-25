 /**
 ******************************************************************************
 * @file    mx66uw1g45g_conf.h
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef MX66UW1G45G_CONF_H
#define MX66UW1G45G_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32n6xx_hal.h"

/** @addtogroup BSP
  * @{
  */
#define CONF_OSPI_ODS                MX66UW1G45G_CR_ODS_24   /* MX66UW1G45G Output Driver Strength */

#define DUMMY_CYCLES_READ            8U
#define DUMMY_CYCLES_READ_OCTAL      10U
#define DUMMY_CYCLES_READ_OCTAL_DTR  10U
#define DUMMY_CYCLES_REG_OCTAL       4U
#define DUMMY_CYCLES_REG_OCTAL_DTR   5U

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* MX66UW1G45G_CONF_H */
