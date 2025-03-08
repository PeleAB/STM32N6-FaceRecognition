/**
  ******************************************************************************
  * @file    imx335.c
  * @author  MCD Application Team
  * @brief   This file provides the IMX335 camera driver
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

/* Includes ------------------------------------------------------------------*/
#include "imx335.h"
#include <string.h>

/** @addtogroup BSP
  * @{
  */

/** @addtogroup Components
  * @{
  */

/** @addtogroup IMX335
  * @brief     This file provides a set of functions needed to drive the
  *            IMX335 Camera module.
  * @{
  */

/** @defgroup IMX335_Private_TypesDefinitions Private Types definition
  * @{
  */
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/** @defgroup IMX335_Private_Constants Private Constants
  * @{
  */
struct regval {
  uint16_t addr;
  uint8_t val;
};

static const struct regval res_2592_1944_regs[] = {
  {0x3000, 0x01},
  {0x3002, 0x00},
  {0x300c, 0x3b},
  {0x300d, 0x2a},
  {0x3018, 0x04},
  {0x302c, 0x3c},
  {0x302e, 0x20},
  {0x3056, 0x98},
  {0x3074, 0xc8},
  {0x3076, 0x30},
  {0x304c, 0x00},
  {0x314c, 0xc6},
  {0x315a, 0x02},
  {0x3168, 0xa0},
  {0x316a, 0x7e},
  {0x31a1, 0x00},
  {0x3288, 0x21},
  {0x328a, 0x02},
  {0x3414, 0x05},
  {0x3416, 0x18},
  {0x3648, 0x01},
  {0x364a, 0x04},
  {0x364c, 0x04},
  {0x3678, 0x01},
  {0x367c, 0x31},
  {0x367e, 0x31},
  {0x3706, 0x10},
  {0x3708, 0x03},
  {0x3714, 0x02},
  {0x3715, 0x02},
  {0x3716, 0x01},
  {0x3717, 0x03},
  {0x371c, 0x3d},
  {0x371d, 0x3f},
  {0x372c, 0x00},
  {0x372d, 0x00},
  {0x372e, 0x46},
  {0x372f, 0x00},
  {0x3730, 0x89},
  {0x3731, 0x00},
  {0x3732, 0x08},
  {0x3733, 0x01},
  {0x3734, 0xfe},
  {0x3735, 0x05},
  {0x3740, 0x02},
  {0x375d, 0x00},
  {0x375e, 0x00},
  {0x375f, 0x11},
  {0x3760, 0x01},
  {0x3768, 0x1b},
  {0x3769, 0x1b},
  {0x376a, 0x1b},
  {0x376b, 0x1b},
  {0x376c, 0x1a},
  {0x376d, 0x17},
  {0x376e, 0x0f},
  {0x3776, 0x00},
  {0x3777, 0x00},
  {0x3778, 0x46},
  {0x3779, 0x00},
  {0x377a, 0x89},
  {0x377b, 0x00},
  {0x377c, 0x08},
  {0x377d, 0x01},
  {0x377e, 0x23},
  {0x377f, 0x02},
  {0x3780, 0xd9},
  {0x3781, 0x03},
  {0x3782, 0xf5},
  {0x3783, 0x06},
  {0x3784, 0xa5},
  {0x3788, 0x0f},
  {0x378a, 0xd9},
  {0x378b, 0x03},
  {0x378c, 0xeb},
  {0x378d, 0x05},
  {0x378e, 0x87},
  {0x378f, 0x06},
  {0x3790, 0xf5},
  {0x3792, 0x43},
  {0x3794, 0x7a},
  {0x3796, 0xa1},
  {0x37b0, 0x36},
  {0x3a00, 0x01},
};

static const struct regval mode_2l_10b_regs[] = {
  {0x3050, 0x00},
  {0x319D, 0x00},
  {0x341c, 0xff},
  {0x341d, 0x01},
  {0x3a01, 0x01},
};

static const struct regval inck_74Mhz_regs[] = {
  {0x300c, 0xB6},
  {0x300d, 0x7F},
  {0x314c, 0x80},
  {0x314d, 0x00},
  {0x315a, 0x03},
  {0x3168, 0x68},
  {0x316a, 0x7F},
};

static const struct regval inck_27Mhz_regs[] = {
  {0x300c, 0x42},
  {0x300d, 0x2E},
  {0x314c, 0xB0},
  {0x314d, 0x00},
  {0x315a, 0x02},
  {0x3168, 0x8F},
  {0x316a, 0x7E},
};

static const struct regval inck_24Mhz_regs[] = {
  {0x300c, 0x3B},
  {0x300d, 0x2A},
  {0x314c, 0xC6},
  {0x314d, 0x00},
  {0x315a, 0x02},
  {0x3168, 0xA0},
  {0x316a, 0x7E},
};

static const struct regval inck_18Mhz_regs[] = {
  {0x300c, 0x2D},
  {0x300d, 0x1F},
  {0x314c, 0x84},
  {0x314d, 0x00},
  {0x315a, 0x01},
  {0x3168, 0x6B},
  {0x316a, 0x7D},
};

static const struct regval inck_6Mhz_regs[] = {
  {0x300c, 0x0F},
  {0x300d, 0x0B},
  {0x314c, 0xC6},
  {0x314d, 0x00},
  {0x315a, 0x00},
  {0x3168, 0xA0},
  {0x316a, 0x7C},
};

static const struct regval framerate_10fps_regs[] = {
  {0x3030, 0xC0},
  {0x3031, 0x34},
};

static const struct regval framerate_15fps_regs[] = {
  {0x3030, 0x2A},
  {0x3031, 0x23},
};

static const struct regval framerate_20fps_regs[] = {
  {0x3030, 0x60},
  {0x3031, 0x1A},
};

static const struct regval framerate_25fps_regs[] = {
  {0x3030, 0x1A},
  {0x3031, 0x15},
};

static const struct regval framerate_30fps_regs[] = {
  {0x3030, 0x94},
  {0x3031, 0x11},
};

static const struct regval mirrorflip_mode_regs[][10] = {
  {
    {AREA3_ST_ADR_1_LSB, 0xc8}, //AREA3_ST_ADR_1 LSB
    {AREA3_ST_ADR_1_MSB, 0x00}, //AREA3_ST_ADR_1 MSB
    {IMX335_REG_HREVERSE, 0x00}, //HREVERSE 0
    {IMX335_REG_VREVERSE, 0x00}, //VREVERSE 0
    {0x3081, 0x02}, //RESERVED
    {0x3083, 0x02}, //RESERVED
    {0x30b6, 0x00}, //RESERVED
    {0x30b7, 0x00}, //RESERVED
    {0x3116, 0x08}, //RESERVED
    {0x3117, 0x00}, //RESERVED
  },
  {
    {AREA3_ST_ADR_1_LSB, 0xf8}, //AREA3_ST_ADR_1 LSB
    {AREA3_ST_ADR_1_MSB, 0x0f}, //AREA3_ST_ADR_1 MSB
    {IMX335_REG_HREVERSE, 0x00}, //HREVERSE 0
    {IMX335_REG_VREVERSE, 0x01}, //VREVERSE 1
    {0x3081, 0xfe}, //RESERVED
    {0x3083, 0xfe}, //RESERVED
    {0x30b6, 0xfa}, //RESERVED
    {0x30b7, 0x01}, //RESERVED
    {0x3116, 0x02}, //RESERVED
    {0x3117, 0x00}, //RESERVED
  },
  {
    {AREA3_ST_ADR_1_LSB, 0xc8}, //AREA3_ST_ADR_1 LSB
    {AREA3_ST_ADR_1_MSB, 0x00}, //AREA3_ST_ADR_1 MSB
    {IMX335_REG_HREVERSE, 0x01}, //HREVERSE 1
    {IMX335_REG_VREVERSE, 0x00}, //VREVERSE 0
    {0x3081, 0x02}, //RESERVED
    {0x3083, 0x02}, //RESERVED
    {0x30b6, 0x00}, //RESERVED
    {0x30b7, 0x00}, //RESERVED
    {0x3116, 0x08}, //RESERVED
    {0x3117, 0x00}, //RESERVED
  },
  {
    {AREA3_ST_ADR_1_LSB, 0xf8}, //AREA3_ST_ADR_1 LSB
    {AREA3_ST_ADR_1_MSB, 0x0f}, //AREA3_ST_ADR_1 MSB
    {IMX335_REG_HREVERSE, 0x01}, //HREVERSE 1
    {IMX335_REG_VREVERSE, 0x01}, //VREVERSE 1
    {0x3081, 0xfe}, //RESERVED
    {0x3083, 0xfe}, //RESERVED
    {0x30b6, 0xfa}, //RESERVED
    {0x30b7, 0x01}, //RESERVED
    {0x3116, 0x02}, //RESERVED
    {0x3117, 0x00}, //RESERVED
  },
};

static const struct regval test_pattern_enable_regs[] = {
  {0x3148, 0x10},
  {0x3280, 0x00},
  {0x329c, 0x01},
  {0x32a0, 0x11},
  {0x3302, 0x00},
  {0x3303, 0x00},
  {0x336c, 0x00},
};

static const struct regval test_pattern_disable_regs[] = {
  {0x3148, 0x00},
  {0x3280, 0x01},
  {0x329c, 0x00},
  {0x32a0, 0x10},
  {0x3302, 0x32},
  {0x3303, 0x00},
  {0x336c, 0x01},
};

#define IMX335_1H_PERIOD_USEC (1000000.0F / 4500 / 30)

/**
  * @}
  */

/** @defgroup IMX335_Private_Functions_Prototypes Private Functions Prototypes
  * @{
  */
static int32_t IMX335_WriteTable(IMX335_Object_t *pObj, const struct regval *regs, uint32_t size);
static int32_t IMX335_ReadRegWrap(void *handle, uint16_t Reg, uint8_t* Data, uint16_t Length);
static int32_t IMX335_WriteRegWrap(void *handle, uint16_t Reg, uint8_t* Data, uint16_t Length);
static int32_t IMX335_Delay(IMX335_Object_t *pObj, uint32_t Delay);

/**
  * @}
  */

/** @defgroup IMX335_Private_Functions Private Functions
  * @{
  */
static int32_t IMX335_WriteTable(IMX335_Object_t *pObj, const struct regval *regs, uint32_t size)
{
  uint32_t index;
  int32_t ret = IMX335_OK;

  /* Set registers */
  for(index=0; index<size ; index++)
  {
    if(ret != IMX335_ERROR)
    {
      if(imx335_write_reg(&pObj->Ctx, regs[index].addr, (uint8_t *)&(regs[index].val), 1) != IMX335_OK)
      {
        ret = IMX335_ERROR;
      }
    }
  }
  return ret;
}

/**
  * @brief This function provides accurate delay (in milliseconds)
  * @param pObj   pointer to component object
  * @param Delay  specifies the delay time length, in milliseconds
  * @retval IMX335_OK
  */
static int32_t IMX335_Delay(IMX335_Object_t *pObj, uint32_t Delay)
{
  uint32_t tickstart;
  tickstart = pObj->IO.GetTick();
  while((pObj->IO.GetTick() - tickstart) < Delay)
  {
  }
  return IMX335_OK;
}

/**
  * @brief  Wrap component ReadReg to Bus Read function
  * @param  handle  Component object handle
  * @param  Reg  The target register address to write
  * @param  pData  The target register value to be written
  * @param  Length  buffer size to be written
  * @retval error status
  */
static int32_t IMX335_ReadRegWrap(void *handle, uint16_t Reg, uint8_t* pData, uint16_t Length)
{
  IMX335_Object_t *pObj = (IMX335_Object_t *)handle;

  return pObj->IO.ReadReg(pObj->IO.Address, Reg, pData, Length);
}

/**
  * @brief  Wrap component WriteReg to Bus Write function
  * @param  handle  Component object handle
  * @param  Reg  The target register address to write
  * @param  pData  The target register value to be written
  * @param  Length  buffer size to be written
  * @retval error status
  */
static int32_t IMX335_WriteRegWrap(void *handle, uint16_t Reg, uint8_t* pData, uint16_t Length)
{
  IMX335_Object_t *pObj = (IMX335_Object_t *)handle;

  return pObj->IO.WriteReg(pObj->IO.Address, Reg, pData, Length);
}

/**
  * @}
  */

/** @defgroup IMX335_Exported_Functions IMX335 Exported Functions
  * @{
  */
/**
  * @brief  Register component IO bus
  * @param  Component object pointer
  * @retval Component status
  */
int32_t IMX335_RegisterBusIO(IMX335_Object_t *pObj, IMX335_IO_t *pIO)
{
  int32_t ret;

  if (pObj == NULL)
  {
    ret = IMX335_ERROR;
  }
  else
  {
    pObj->IO.Init      = pIO->Init;
    pObj->IO.DeInit    = pIO->DeInit;
    pObj->IO.Address   = pIO->Address;
    pObj->IO.WriteReg  = pIO->WriteReg;
    pObj->IO.ReadReg   = pIO->ReadReg;
    pObj->IO.GetTick   = pIO->GetTick;

    pObj->Ctx.ReadReg  = IMX335_ReadRegWrap;
    pObj->Ctx.WriteReg = IMX335_WriteRegWrap;
    pObj->Ctx.handle   = pObj;

    if(pObj->IO.Init != NULL)
    {
      ret = pObj->IO.Init();
    }
    else
    {
      ret = IMX335_ERROR;
    }
  }

  return ret;
}

/**
  * @brief  Initializes the IMX335 CAMERA component.
  * @param  pObj  pointer to component object
  * @param  Resolution  Camera resolution
  * @param  PixelFormat pixel format to be configured
  * @retval Component status
  */
int32_t IMX335_Init(IMX335_Object_t *pObj, uint32_t Resolution, uint32_t PixelFormat)
{
  int32_t ret = IMX335_OK;

  if(pObj->IsInitialized == 0U)
  {
    switch (Resolution)
    {
      case IMX335_R2592_1944:
        if(IMX335_WriteTable(pObj, res_2592_1944_regs, ARRAY_SIZE(res_2592_1944_regs)) != IMX335_OK)
        {
          ret = IMX335_ERROR;
        }
        break;
      /* Add new resolution here */
      default:
        /* Resolution not supported */
        ret = IMX335_ERROR;
    }

    if(!ret)
    {
      if(IMX335_WriteTable(pObj, mode_2l_10b_regs, ARRAY_SIZE(mode_2l_10b_regs)) != IMX335_OK)
      {
        ret = IMX335_ERROR;
      }
      else
      {
        pObj->IsInitialized = 1U;
      }
    }
  }

  return ret;
}

/**
  * @brief  Initializes the IMX335 CAMERA component.
  * @param  pObj  pointer to component object
  * @param  Resolution  Camera resolution
  * @param  PixelFormat pixel format to be configured
  * @retval Component status
  */
int32_t IMX335_Start(IMX335_Object_t *pObj)
{
  uint8_t tmp;
  int32_t ret = IMX335_OK;
  /* Start streaming */
  tmp = IMX335_MODE_STREAMING;
  ret = imx335_write_reg(&pObj->Ctx, IMX335_REG_MODE_SELECT, &tmp, 1);
  if (ret != IMX335_OK)
  {
    return IMX335_ERROR;
  }
  IMX335_Delay(pObj, 20);
  return ret;
}

/**
  * @brief  De-initializes the camera sensor.
  * @param  pObj  pointer to component object
  * @retval Component status
  */
int32_t IMX335_DeInit(IMX335_Object_t *pObj)
{
  if(pObj->IsInitialized == 1U)
  {
    /* De-initialize camera sensor interface */
    pObj->IsInitialized = 0U;
  }

  return IMX335_OK;
}

/**
  * @brief  Read the IMX335 Camera identity.
  * @param  pObj  pointer to component object
  * @param  Id    pointer to component ID
  * @retval Component status
  */
int32_t IMX335_ReadID(IMX335_Object_t *pObj, uint32_t *Id)
{
  int32_t ret;
  uint8_t tmp;

  /* Initialize I2C */
  pObj->IO.Init();

  if(imx335_read_reg(&pObj->Ctx, IMX335_REG_ID, &tmp, 1)!= IMX335_OK)
  {
    ret = IMX335_ERROR;
  }
  else
  {
    *Id = tmp;
    ret = IMX335_OK;
  }

  /* Component status */
  return ret;
}

/**
  * @brief  Read the IMX335 Camera Capabilities.
  * @param  pObj          pointer to component object
  * @param  Capabilities  pointer to component Capabilities
  * @retval Component status
  */
int32_t IMX335_GetCapabilities(IMX335_Object_t *pObj, IMX335_Capabilities_t *Capabilities)
{
  int32_t ret;

  if(pObj == NULL)
  {
    ret = IMX335_ERROR;
  }
  else
  {
    Capabilities->Config_Brightness    = 0;
    Capabilities->Config_Contrast      = 0;
    Capabilities->Config_HueDegree     = 0;
    Capabilities->Config_Gain          = 1;
    Capabilities->Config_Exposure      = 1;
    Capabilities->Config_ExposureMode  = 0;
    Capabilities->Config_LightMode     = 0;
    Capabilities->Config_MirrorFlip    = 1;
    Capabilities->Config_NightMode     = 0;
    Capabilities->Config_Resolution    = 0;
    Capabilities->Config_Saturation    = 0;
    Capabilities->Config_SpecialEffect = 0;
    Capabilities->Config_Zoom          = 0;
    Capabilities->Config_SensorInfo    = 1;
    Capabilities->Config_TestPattern   = 1;

    ret = IMX335_OK;
  }

  return ret;
}

/**
  * @brief  Set the gain
  * @param  pObj  pointer to component object
  * @param  Gain Gain in mdB
  * @retval Component status
  */
int32_t IMX335_SetGain(IMX335_Object_t *pObj, int32_t gain)
{
  int32_t ret = IMX335_OK;
  uint8_t hold;

  if ((gain > IMX335_GAIN_MAX) || (gain < IMX335_GAIN_MIN))
  {
    ret = IMX335_ERROR;
  }
  else
  {
    /* Convert to IMX335 gain unit (0.3 dB = 300 mdB) */
    gain /= IMX335_GAIN_UNIT_MDB;

    hold = 1;
    if(imx335_write_reg(&pObj->Ctx, IMX335_REG_HOLD, &hold, 1) != IMX335_OK)
    {
      ret = IMX335_ERROR;
    }
    else
    {
      if(imx335_write_reg(&pObj->Ctx, IMX335_REG_GAIN, (uint8_t *)&gain, 2) != IMX335_OK)
      {
        ret = IMX335_ERROR;
      }
      else
      {
        hold = 0;
        if(imx335_write_reg(&pObj->Ctx, IMX335_REG_HOLD, &hold, 1) != IMX335_OK)
        {
          ret = IMX335_ERROR;
        }
      }
    }
  }

return ret;
}

/**
  * @brief  Set the exposure
  * @param  pObj  pointer to component object
  * @param  Exposure Exposure in micro seconds
  * @retval Component status
  */
int32_t IMX335_SetExposure(IMX335_Object_t *pObj, int32_t exposure)
{
  int32_t ret = IMX335_OK;
  uint32_t vmax, shutter;
  uint8_t hold;


  if (imx335_read_reg(&pObj->Ctx, IMX335_REG_VMAX, (uint8_t *)&vmax, 4) != IMX335_OK)
  {
    ret = IMX335_ERROR;
  }
  else
  {
    shutter = (uint32_t) (vmax - (exposure / IMX335_1H_PERIOD_USEC));

    if (shutter < IMX335_SHUTTER_MIN)
    {
      ret = IMX335_ERROR;
    }
    else
    {
      hold = 1;
      if(imx335_write_reg(&pObj->Ctx, IMX335_REG_HOLD, &hold, 1) != IMX335_OK)
      {
        ret = IMX335_ERROR;
      }
      else
      {
        if(imx335_write_reg(&pObj->Ctx, IMX335_REG_SHUTTER, (uint8_t *)&shutter, 3) != IMX335_OK)
        {
          ret = IMX335_ERROR;
        }
        else
        {
          hold = 0;
          if(imx335_write_reg(&pObj->Ctx, IMX335_REG_HOLD, &hold, 1) != IMX335_OK)
          {
            ret = IMX335_ERROR;
          }
        }
      }
    }
  }

  return ret;
}

/**
  * @brief  Set the Frequency
  * @param  pObj  pointer to component object
  * @param  frequency in Mhz
  * @retval Component status
  */
int32_t IMX335_SetFrequency(IMX335_Object_t *pObj, int32_t frequency)
{
  uint32_t ret = IMX335_OK;

  switch (frequency)
  {
    case IMX335_INCK_74MHZ:
      if(IMX335_WriteTable(pObj, inck_74Mhz_regs, ARRAY_SIZE(inck_74Mhz_regs)) != IMX335_OK)
      {
        ret = IMX335_ERROR;
      }
      break;
    case IMX335_INCK_27MHZ:
      if(IMX335_WriteTable(pObj, inck_27Mhz_regs, ARRAY_SIZE(inck_27Mhz_regs)) != IMX335_OK)
      {
        ret = IMX335_ERROR;
      }
      break;
    case IMX335_INCK_24MHZ:
      if(IMX335_WriteTable(pObj, inck_24Mhz_regs, ARRAY_SIZE(inck_24Mhz_regs)) != IMX335_OK)
      {
        ret = IMX335_ERROR;
      }
      break;
    case IMX335_INCK_18MHZ:
      if(IMX335_WriteTable(pObj, inck_18Mhz_regs, ARRAY_SIZE(inck_18Mhz_regs)) != IMX335_OK)
      {
        ret = IMX335_ERROR;
      }
      break;
    default:
      /* IMX335_INCK_6MHZ */
      if(IMX335_WriteTable(pObj, inck_6Mhz_regs, ARRAY_SIZE(inck_6Mhz_regs)) != IMX335_OK)
      {
        ret = IMX335_ERROR;
      }
      break;
  };

  return ret;
}

/**
  * @brief  Set the Framerate
  * @param  pObj  pointer to component object
  * @param  framerate 10, 15, 20, 25 or 30fps
  * @retval Component status
  */
int32_t IMX335_SetFramerate(IMX335_Object_t *pObj, int32_t framerate)
{
  uint32_t ret = IMX335_OK;
  switch (framerate)
  {
    case 10:
      if(IMX335_WriteTable(pObj, framerate_10fps_regs, ARRAY_SIZE(framerate_10fps_regs)) != IMX335_OK)
      {
        ret = IMX335_ERROR;
      }
      break;
    case 15:
      if(IMX335_WriteTable(pObj, framerate_15fps_regs, ARRAY_SIZE(framerate_15fps_regs)) != IMX335_OK)
      {
        ret = IMX335_ERROR;
      }
      break;
    case 20:
      if(IMX335_WriteTable(pObj, framerate_20fps_regs, ARRAY_SIZE(framerate_20fps_regs)) != IMX335_OK)
      {
       ret = IMX335_ERROR;
      }
      break;
    case 25:
      if(IMX335_WriteTable(pObj, framerate_25fps_regs, ARRAY_SIZE(framerate_25fps_regs)) != IMX335_OK)
      {
        ret = IMX335_ERROR;
      }
      break;
    default:
      /* 30fps */
      if(IMX335_WriteTable(pObj, framerate_30fps_regs, ARRAY_SIZE(framerate_30fps_regs)) != IMX335_OK)
      {
        ret = IMX335_ERROR;
      }
      break;
  };

  return ret;
}

/**
  * @brief  Control imx335 camera mirror/vflip.
  * @param  pObj  pointer to component object
  * @param  Config To configure mirror, flip, both or none
  * @retval Component status
  */
int32_t IMX335_MirrorFlipConfig(IMX335_Object_t *pObj, uint32_t Config)
{
  int32_t ret = IMX335_OK;

  switch (Config)
  {
    case IMX335_FLIP:
      ret = IMX335_WriteTable(pObj, mirrorflip_mode_regs[1], ARRAY_SIZE(mirrorflip_mode_regs[1]));
      break;
    case IMX335_MIRROR:
      ret = IMX335_WriteTable(pObj, mirrorflip_mode_regs[2], ARRAY_SIZE(mirrorflip_mode_regs[2]));
      break;
    case IMX335_MIRROR_FLIP:
      ret = IMX335_WriteTable(pObj, mirrorflip_mode_regs[3], ARRAY_SIZE(mirrorflip_mode_regs[3]));
      break;
    case IMX335_MIRROR_FLIP_NONE:
    default:
      ret = IMX335_WriteTable(pObj, mirrorflip_mode_regs[0], ARRAY_SIZE(mirrorflip_mode_regs[0]));
      break;
  }
  return ret;
}

/**
  * @brief  Set the Test Pattern Generator
  * @param  pObj  pointer to component object
  * @param  mode Pattern mode:
  *              -1 : Disable
  *               0 : All 000h
  *               1 : All FFFh
  *               2 : All 555h
  *               3 : All AAAh
  *               4 : Toggle 555/AAAh
  *               5 : Toggle AAA/555h
  *               6 : Toggle 000/555h
  *               7 : Toggle 555/000h
  *               8 : Toggle 000/FFFh
  *               9 : Toggle FFF/000h
  *               10: Horizontal color bars
  *               11: Vertical color bars
  * @retval Component status
  */
int32_t IMX335_SetTestPattern(IMX335_Object_t *pObj, int32_t mode)
{
  int32_t ret = IMX335_OK;
  uint8_t val;

  if (mode >= 0)
  {
    /* Enable Test Pattern #mode */
    val = mode;
    if (imx335_write_reg(&pObj->Ctx, IMX335_REG_TPG, &val, 1) != IMX335_OK)
    {
      return IMX335_ERROR;
    }
    if (IMX335_WriteTable(pObj, test_pattern_enable_regs, ARRAY_SIZE(test_pattern_enable_regs)) != IMX335_OK)
    {
      return IMX335_ERROR;
    }
  }
  else
  {
    /* Disable Test Pattern */
    if (IMX335_WriteTable(pObj, test_pattern_disable_regs, ARRAY_SIZE(test_pattern_disable_regs)) != IMX335_OK)
    {
      return IMX335_ERROR;
    }
  }

  return ret;
}

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */