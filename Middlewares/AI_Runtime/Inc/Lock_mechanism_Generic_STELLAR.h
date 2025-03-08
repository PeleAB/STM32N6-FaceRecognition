/**
  ******************************************************************************
  * @file    Lock_mechanism_Generic_STELLAR.h
  * @author  AST Embedded Analytics Research Platform + AIS MCD
  * @brief   Locking routines for MCUs
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

#ifndef STELLAR_LOCK_GENERIC_H
#define STELLAR_LOCK_GENERIC_H

#include <stdint.h>

// #include <cmsis_compiler.h>

#if !defined(STELLAR_E)
#error STELLAR_E C-define required
#endif


/*
 * Replace the while(1) by your own error management mechanism
 * It may be infinite loop (too easy) or corruption of
 */
#define DO_STM32_LOCK   while (1) {}

#define REG_(ptr)       (*((volatile uint32_t*)(ptr)))

#define SYSCFG_MIDR1    0x42000000UL
#define SYSCFG_MIDR2    0x42000004UL
#define STELLAR_E_ID    0x2511U
#define STELLAR_E_SERIE 0x45U

/**
  * @brief  Check if the CRC IP is accessible
  * @param  None
  * @retval true if OK otherwise false
  */
static inline
bool CRC_Is_Enabled(void)
{
  bool res = false;

	// check SR5E1 device part number
  if (((uint16_t)(REG_(SYSCFG_MIDR1)>>16U)) == STELLAR_E_ID)
    res = true;

  return res;
}


/**
  * @brief  CRC Lock mechanism used to lock a generated binary files
  *         to STM32 product.
  * @param  None
  * @retval None
  * @NOTE
  */
static inline
void CRC_Lock( void )
{
	// check SR5E1 device part number
	if (((uint16_t)(REG_(SYSCFG_MIDR1)>>16U)) != STELLAR_E_ID) {
		DO_STM32_LOCK;
	}
}

#undef DO_STM32_LOCK
#undef REG_

#endif    /* STELLAR_LOCK_GENERIC_H */
