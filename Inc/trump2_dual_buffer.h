#ifndef TRUMP2_DUAL_BUFFER_H
#define TRUMP2_DUAL_BUFFER_H

#include <stdint.h>
#include "app_config.h"

/* Dual dummy buffers derived from trump2.jpg */

/* img_buffer: 480x480 centered in 800x480 RGB565 with black padding (original camera frame) */
extern const uint16_t trump2_img_buffer[800 * 480];

/* nn_rgb: 128x128 RGB888 (neural network input) */
extern const uint8_t trump2_nn_rgb[128 * 128 * 3];

extern const uint8_t dummy_cropped_face_rgb [112 * 112 * 3];

/* Buffer sizes */
#define TRUMP2_IMG_BUFFER_SIZE (800 * 480 * 2)
#define TRUMP2_NN_RGB_SIZE (128 * 128 * 3)

#endif /* TRUMP2_DUAL_BUFFER_H */
