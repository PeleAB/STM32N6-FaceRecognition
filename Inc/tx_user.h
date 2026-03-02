 /**
 ******************************************************************************
 * @file    tx_user.h
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

#ifndef TX_USER
#define TX_USER

#define TX_MAX_PRIORITIES 64

#define TX_TIMER_TICKS_PER_SECOND (1000UL)

#ifndef SYSTEM_CLOCK
#define SYSTEM_CLOCK 800000000
#endif

#ifndef TICK_FREQ
#define TICK_FREQ TX_TIMER_TICKS_PER_SECOND
#endif

#endif

