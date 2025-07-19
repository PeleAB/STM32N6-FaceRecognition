#ifndef DUMMY_DUAL_BUFFER_H
#define DUMMY_DUAL_BUFFER_H

#include <stdint.h>
#include "app_config.h"

/* Dual dummy buffers derived from test image */

/* img_buffer: 480x480 centered in 800x480 RGB565 with black padding (original camera frame) */
extern const uint16_t dummy_test_img_buffer[800 * 480];

/* nn_rgb: 128x128 RGB888 (neural network input) */
extern const uint8_t dummy_test_nn_rgb[128 * 128 * 3];

extern const uint8_t dummy_cropped_face_rgb [112 * 112 * 3];

/* Buffer sizes */
#define DUMMY_TEST_IMG_BUFFER_SIZE (800 * 480 * 2)
#define DUMMY_TEST_NN_RGB_SIZE (128 * 128 * 3)

#endif /* DUMMY_DUAL_BUFFER_H */
