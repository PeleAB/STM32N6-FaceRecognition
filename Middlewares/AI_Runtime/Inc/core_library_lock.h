/**
  ******************************************************************************
  * @file    core_library_lock.h
  * @author  AST Embedded Analytics Research Platform
  * @brief   Definitions of AI platform MCUs locking routines
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

#ifndef CORE_LIBRARY_LOCK_H
#define CORE_LIBRARY_LOCK_H

/* bz169244 - remove lock mechanism CRC-based for STM32 series */

#if (defined(CM0) || defined(CM0PLUS) || defined(CM4) || defined(CM7) || defined(CM33) || defined(CM55)) && \
    defined(ENABLE_LOCK_FEATURE)

#if defined(STELLAR_E)
#include "Lock_mechanism_Generic_STELLAR.h"
#pragma message("[Warning] Enabled STELLAR-E Lock Code Feature..")
#else
#include "Lock_mechanism_Generic_STM32.h"
#pragma message("[Warning] Enabled STM32 Lock Code Feature..")
#endif

#define CORE_LIBRARY_LOCK              do { CRC_Lock(); } while (0);
#define CORE_LIBRARY_LOCK_ACCESS()     CRC_Is_Enabled()

#else

/* #pragma message("[Warning] Disabled Lock Code Feature..") */

#define CORE_LIBRARY_LOCK              do { } while (0);
#define CORE_LIBRARY_LOCK_ACCESS()     true
#endif

#endif      /* CORE_LIBRARY_LOCK_H */
