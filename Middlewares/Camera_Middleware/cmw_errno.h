 /**
 ******************************************************************************
 * @file    cmw_errno.h
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef CMW_ERRNO_H
#define CMW_ERRNO_H

#ifdef __cplusplus
extern "C" {
#endif

/* Common Error codes */
#define CMW_ERROR_NONE                    0
#define CMW_ERROR_NO_INIT                -1
#define CMW_ERROR_WRONG_PARAM            -2
#define CMW_ERROR_BUSY                   -3
#define CMW_ERROR_PERIPH_FAILURE         -4
#define CMW_ERROR_COMPONENT_FAILURE      -5
#define CMW_ERROR_UNKNOWN_FAILURE        -6
#define CMW_ERROR_UNKNOWN_COMPONENT      -7
#define CMW_ERROR_BUS_FAILURE            -8
#define CMW_ERROR_CLOCK_FAILURE          -9
#define CMW_ERROR_MSP_FAILURE            -10
#define CMW_ERROR_FEATURE_NOT_SUPPORTED  -11

/* CMW XSPI error codes */
#define CMW_ERROR_XSPI_SUSPENDED          -20
#define CMW_ERROR_XSPI_ASSIGN_FAILURE     -24
#define CMW_ERROR_XSPI_SETUP_FAILURE      -25
#define CMW_ERROR_XSPI_MMP_LOCK_FAILURE   -26
#define CMW_ERROR_XSPI_MMP_UNLOCK_FAILURE -27

/* CMW TS error code */
#define CMW_ERROR_TS_TOUCH_NOT_DETECTED   -30

/* CMW BUS error codes */
#define CMW_ERROR_BUS_TRANSACTION_FAILURE    -100
#define CMW_ERROR_BUS_ARBITRATION_LOSS       -101
#define CMW_ERROR_BUS_ACKNOWLEDGE_FAILURE    -102
#define CMW_ERROR_BUS_PROTOCOL_FAILURE       -103

#define CMW_ERROR_BUS_MODE_FAULT             -104
#define CMW_ERROR_BUS_FRAME_ERROR            -105
#define CMW_ERROR_BUS_CRC_ERROR              -106
#define CMW_ERROR_BUS_DMA_FAILURE            -107

#ifdef __cplusplus
}
#endif

#endif /* CMW_ERRNO_H */
