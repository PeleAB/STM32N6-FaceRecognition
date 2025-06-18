#ifndef IMG_BUFFER_H
#define IMG_BUFFER_H

#include <stdint.h>
#include "app_config.h"

#define IMG_BUFFER_SIZE (LCD_FG_WIDTH * LCD_FG_HEIGHT * 2)

extern uint8_t img_buffer[IMG_BUFFER_SIZE];

#endif /* IMG_BUFFER_H */
