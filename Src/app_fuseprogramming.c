 /**
 ******************************************************************************
 * @file    app_fuseprogramming.c
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

#include "stm32n6xx_hal_bsec.h"
static void ErrorHandler(void);

#define BSEC_HW_CONFIG_ID        124U
#define BSEC_HWS_HSLV_VDDIO3     15U
#define BSEC_HWS_HSLV_VDDIO2     16U

#define BSEC_FUSE_ADDRESS        BSEC_HW_CONFIG_ID
#define BSEC_FUSE_MASK           ((uint32_t)((1U << BSEC_HWS_HSLV_VDDIO3) | (1U << BSEC_HWS_HSLV_VDDIO2)));




/**
  * @brief  Check specific fuse configuration and update it if needed.
  * @retval None
  */
void Fuse_Programming(void)
{
  uint32_t fuse_id, bit_mask, data;

  BSEC_HandleTypeDef sBsecHandler;

  sBsecHandler.Instance = BSEC;

  /* Read current value of fuse */
  fuse_id = BSEC_FUSE_ADDRESS;
  if (HAL_BSEC_OTP_Read(&sBsecHandler, fuse_id, &data) == HAL_OK)
  {
    /* Check if bit has already been set */
    bit_mask = BSEC_FUSE_MASK;
    if ((data & bit_mask) != bit_mask)
    {
      data |= bit_mask;
      /* Bitwise programming of lower bits */
      if (HAL_BSEC_OTP_Program(&sBsecHandler, fuse_id, data, HAL_BSEC_NORMAL_PROG) == HAL_OK)
      {
        /* Read lower bits to verify the correct programming */
        if (HAL_BSEC_OTP_Read(&sBsecHandler, fuse_id, &data) == HAL_OK)
        {
          if ((data & bit_mask) != bit_mask)
          {
            /* Error : Fuse programming not taken in account */
            ErrorHandler();
          }
        }
        else
        {
          /* Error : Fuse read unsuccessful */
          ErrorHandler();
        }
      }
      else
      {
        /* Error : Fuse programming unsuccessful */
        ErrorHandler();
      }
    }
    else
    {

    }
  }
  else
  {
    /* Error  : Fuse read unsuccessful */
    ErrorHandler();
  }
}

/**
  * @brief  Handle error which occurs when calling HAL API
  * @retval None
  */
static void ErrorHandler(void)
{
  while(1);
}
