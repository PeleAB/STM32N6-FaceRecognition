 /**
 ******************************************************************************
 * @file    stm32_lcd_ex.c
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

/* Includes ------------------------------------------------------------------*/
#include "stm32_lcd_ex.h"
#include <stdio.h>
#include <stdarg.h>

/* Private define ------------------------------------------------------------*/
#define N_PRINTABLE_CHARS    47 /*!< 800px wide screen / 17px wide font */

/* Functions Definition ------------------------------------------------------*/
void UTIL_LCDEx_PrintfAtLine(uint16_t line, const char * format, ...)
{
  static char buffer[N_PRINTABLE_CHARS + 1];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, N_PRINTABLE_CHARS + 1, format, args);
  UTIL_LCD_DisplayStringAtLine(line, (uint8_t *) buffer);
  va_end(args);
}

void UTIL_LCDEx_PrintfAt(uint32_t x_pos, uint32_t y_pos, Text_AlignModeTypdef mode, const char * format, ...)
{
  static char buffer[N_PRINTABLE_CHARS + 1];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, N_PRINTABLE_CHARS + 1, format, args);
  UTIL_LCD_DisplayStringAt(x_pos, y_pos, (uint8_t *) buffer, mode);
  va_end(args);
}
