#include "img_buffer.h"

__attribute__ ((section (".psram_bss")))
__attribute__ ((aligned (32)))
uint8_t img_buffer[IMG_BUFFER_SIZE];
