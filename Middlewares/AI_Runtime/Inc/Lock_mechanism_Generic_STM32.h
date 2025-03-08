/**
  ******************************************************************************
  * @file    Lock_mechanism_Generic_STM32.h
  * @author  AST Embedded Analytics Research Platform + AIS MCD
  * @brief   Locking routines for MCUs
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  
  ******************************************************************************
  */

#ifndef STM32_LOCK_GENERIC_H
#define STM32_LOCK_GENERIC_H

#include <stdint.h>

#include "core_adapter_stm32.h"

/*
 * Replace the while(1) by your own error management mechanism
 * It may be infinite loop (too easy) or corruption of
 */
#define DO_STM32_LOCK   while (1) {}

#define REG_(ptr)       (*((volatile uint32_t*)(ptr)))


/* STM32_CRC_IP_IS_ENABLED_xx
 *
 *  MODE_DEFAULT - simple CRC IP
 *    - write 1 in the CRC_CR register
 *    - wait that the associated bit is cleared (done by hw)
 *    - if ok, check the reset value of the data register
 *  MODE_POLY - CRC IP with POLYSIZE support
 *    - set 7 bit polynomial mode (0x18 -> CR)
 *    - check if the CRC_CR value is correct
 *    - if ok, reset the IP
 *
 * note - MODE_POLY can be also used for L4 series
 *
 */
#define STM32_CRC_IP_IS_ENABLED_MODE_DEFAULT(_crc_addr) \
    int retry_n = 20; \
    REG_(_crc_addr + 8) = ((uint32_t)0x00000001); \
    while (REG_(_crc_addr + 8) && --retry_n) {} \
    if ((REG_(_crc_addr + 8) == 0) && (REG_(_crc_addr) == 0xFFFFFFFF)) \
      res = true;

#define STM32_CRC_IP_IS_ENABLED_MODE_POLY(_crc_addr) \
    REG_(_crc_addr + 8) = ((uint32_t)0x00000018); \
    if (REG_(_crc_addr + 8) == ((uint32_t)0x00000018)) { \
      REG_(_crc_addr + 8) = ((uint32_t)0x00000001);  \
      while(REG_(_crc_addr + 8)); \
      res = true; \
    };


/* STM32_CRC_IP_LOCK
 *
 *  - reset the CRC IP (+ default configuration)
 *  - waiting that the reset bit is cleared
 *  - write specific pattern in CRC data register
 *  - read the result
 *  - if not OK, lock the STM32.
 */
#define STM32_CRC_IP_LOCK(_crc_addr) \
    REG_(_crc_addr + 8) = ((uint32_t)0x00000001); \
    while (REG_(_crc_addr + 8)); \
    REG_(_crc_addr) = ((uint32_t)0xF407A5C2); \
    if (REG_(_crc_addr) != ((uint32_t)0xB5E8B5CD)) { \
      DO_STM32_LOCK \
    };


/**
  * @brief  Request to acquire the CRC IP resource
  * @param  None
  * @retval resource id used by the release call (should be != 0)
  * @NOTE By default, the CRC IP is used exclusively by the code
  *       when the ai_network_XX fct is called. This callback
  *       can be used by the application layer to manage and to guarantee
  *       the access of the IP.
  */
AI_DECLARE_WEAK
uint32_t _ai_platform_acquire_crc(void)
{
  return 0x1;
}

/**
  * @brief  Release the CRC IP resource
  * @param  id - resource id value returned by the acquire fct
  * @NOTE After this call, the CRC IP is no more accessed.
  */
AI_DECLARE_WEAK
void _ai_platform_release_crc(uint32_t id)
{
}

/**
  * @brief  Check if the CRC IP is accessible
  * @param  None
  * @retval true if OK otherwise false
  */
static inline
bool CRC_Is_Enabled(void)
{
  bool res = false;
  uint32_t id_crc = _ai_platform_acquire_crc();

  if (id_crc == 0)
    return res;

#ifdef CM7  /* H7 and F7 series */

  const uint32_t reg = (REG_(0xE0042000) & 0x7FF);
  if ((reg == ((uint32_t)0x449)) |
      (reg == ((uint32_t)0x451)) |
      (reg == ((uint32_t)0x452)))
  {
    /* all F7 series */
    STM32_CRC_IP_IS_ENABLED_MODE_POLY(0x40023000);
  }
  else if ((REG_(0x5C001000) & 0x7FF) == ((uint32_t)0x450))
  { /* H7 dual core & STM32H742, STM32H743/753, STM32H750 */
    STM32_CRC_IP_IS_ENABLED_MODE_POLY(0x58024C00);
  }
  else if ((REG_(0x5C001000) & 0x7FF) == ((uint32_t)0x483))
  { /* STM32H72x and STM32H73x */
    STM32_CRC_IP_IS_ENABLED_MODE_POLY(0x58024C00);
  }
  else if ((REG_(0x5C001000) & 0x7FF) == ((uint32_t)0x480))
  { /* STM32H7A3/7B3 & STM32H7B0 */
    STM32_CRC_IP_IS_ENABLED_MODE_POLY(0x40023000);
  }
  else if ((REG_(0x5C001000) & 0x7FF) == ((uint32_t)0x0))
  { /* Workaround H7B3 no debug mode */
    STM32_CRC_IP_IS_ENABLED_MODE_POLY(0x40023000);
  }

#elif defined (STM32MP157Cxx)

  STM32_CRC_IP_IS_ENABLED_MODE_POLY(0x4C004000);

#elif defined (CM4) && !defined (STM32MP157Cxx) /* F4/L4 series */

  if ((REG_(0xE0042000) & 0x700) == ((uint32_t)0x400))
  {
    STM32_CRC_IP_IS_ENABLED_MODE_DEFAULT(0x40023000);
  }
  else /* TODO(JMD) which STM32 device? */
  {
    STM32_CRC_IP_IS_ENABLED_MODE_POLY(0x58024C00);
  }

#elif defined (CM33) /* U5/L5 series */
  uint32_t add_crc;

  /* Try to detect the base @ - Secure or un-secure */
  if ((id_crc != 0x40023000) && (id_crc != 0x50023000))
  {
    REG_(0x40023008) = ((uint32_t)0x00000018);
    if (REG_(0x40023008) == ((uint32_t)0x00000018))
      add_crc = 0x40023000;
    else
      add_crc = 0x50023000;
  }
  else
  {
    add_crc = id_crc;
  }

  STM32_CRC_IP_IS_ENABLED_MODE_POLY(add_crc);

#elif defined (CM0) || defined (CM0PLUS) /* F0/U0/G0 series */

  STM32_CRC_IP_IS_ENABLED_MODE_POLY(0x40023000);

#endif

  _ai_platform_release_crc(id_crc);
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
/* NOTE : The CRC need to be activated in the main application */
  uint32_t id_crc = 0;

#ifdef __CORTEX_M_FPB_UNIT_DISABLE__

  /* The Flash Patch and Breakpoint unit need to be disabled
     to prevent the user to debug the lock mechanism */

  /* The FPB unit is not supported by CortexM0 */
#if !defined(CM0) && !defined(CM0PLUS)
   /* Disable the FPB (Flash Patch and Breakpoint) unit of CortexM */
  REG_(0xE0002000) &= ~((uint32_t)0x00000001);
#endif
#endif

  id_crc = _ai_platform_acquire_crc();

#ifdef CM7  /* H7 and F7 series */

  const uint32_t reg = (REG_(0xE0042000) & 0x7FF);
  if ((reg == ((uint32_t)0x449)) |
     (reg == ((uint32_t)0x451)) |
     (reg == ((uint32_t)0x452)))
  {
    /* all F7 series */
    STM32_CRC_IP_LOCK(0x40023000);
  }
  else if ((REG_(0x5C001000) & 0x7FF) == ((uint32_t)0x450))
  { /* H7 dual core & STM32H742, STM32H743/753, STM32H750 */
    STM32_CRC_IP_LOCK(0x58024C00);
  }
  else if ((REG_(0x5C001000) & 0x7FF) == ((uint32_t)0x483))
  { /* STM32H72x and STM32H73x */
    STM32_CRC_IP_LOCK(0x58024C00);
  }
  else if ((REG_(0x5C001000) & 0x7FF) == ((uint32_t)0x480))
  { /* STM32H7A3/7B3 & STM32H7B0 */
    STM32_CRC_IP_LOCK(0x40023000);
  }
  else if ((REG_(0x5C001000) & 0x7FF) == ((uint32_t)0x0))
  { /* Workaround H7B3 no debug mode */
    STM32_CRC_IP_LOCK(0x40023000);
  }

#elif defined (STM32MP157Cxx)

  STM32_CRC_IP_LOCK(0x4C004000);

#elif defined (CM4) && !defined (STM32MP157Cxx) /* F4/L4 series */

  if ((REG_(0xE0042000) & 0x700) == ((uint32_t)0x400))
  {
    STM32_CRC_IP_LOCK(0x40023000);
  }
  else
  {
    STM32_CRC_IP_LOCK(0x58024C00);
  }

#elif defined (CM33) /* U5/L5 series */

  if ((id_crc == 0x40023000) || (id_crc == 0x50023000))
  {
    STM32_CRC_IP_LOCK(id_crc);
  }
  else
  {
    /* First check the IP@ in non secure mode */
    REG_(0x40023008) = ((uint32_t)0x00000018);

    if (REG_(0x40023008) == ((uint32_t)0x00000018))
    {
      STM32_CRC_IP_LOCK(0x40023000);
    }
    else
    {
      STM32_CRC_IP_LOCK(0x50023000);
    }
  }

#elif defined (CM0) || defined (CM0PLUS) /* F0/U0/G0 series */

  STM32_CRC_IP_LOCK(0x40023000);

#else
#warning "Warning: Locking mechanism not set!!. Please define CM0/CM0PLUS or CM4 or CM7 or CM33 according to target"
#endif
  _ai_platform_release_crc(id_crc);
}

#undef DO_STM32_LOCK
#undef REG_

#endif    /*STM32_LOCK_GENERIC_H*/
