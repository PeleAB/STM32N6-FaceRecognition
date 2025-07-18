/**
 * @file trump2_buffer.h
 * @brief Trump2 image buffer data for dummy input testing
 * 
 * This file contains the actual trump2.jpg image converted to 128x128 RGB format
 * for use as a consistent test input in the dummy input buffer system.
 */

#ifndef TRUMP2_BUFFER_H
#define TRUMP2_BUFFER_H

#include <stdint.h>
#include "app_config.h"

/* External declaration of the trump2 image buffer */
extern const uint8_t trump2_rgb_buffer[NN_WIDTH * NN_HEIGHT * NN_BPP];

#endif /* TRUMP2_BUFFER_H */