#ifndef DISPLAY_UTILS_H
#define DISPLAY_UTILS_H

#include "app_postprocess.h"
#include "app_config.h"
#include "stm32_lcd.h"

typedef struct
{
  uint32_t X0;
  uint32_t Y0;
  uint32_t XSize;
  uint32_t YSize;
} Rectangle_TypeDef;

extern Rectangle_TypeDef lcd_bg_area;
extern Rectangle_TypeDef lcd_fg_area;
#if USE_LCD
extern uint8_t lcd_fg_buffer[2][LCD_FG_WIDTH * LCD_FG_HEIGHT * 2];
#endif

void LCD_init(void);
void Display_WelcomeScreen(void);
void Display_NetworkOutput(od_pp_out_t *p_postprocess, uint32_t inference_ms, uint32_t boottime_ms);

#endif /* DISPLAY_UTILS_H */
