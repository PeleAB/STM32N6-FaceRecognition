#ifndef FACE_DEBUG_IMAGE_H
#define FACE_DEBUG_IMAGE_H

#include <stdint.h>

#define FACE_DEBUG_IMAGE_WIDTH  128
#define FACE_DEBUG_IMAGE_HEIGHT 128

/* RGB565 fixture generated from skimage.data.astronaut (NASA image). */
extern const uint16_t g_face_debug_image_rgb565[FACE_DEBUG_IMAGE_WIDTH *
                                                FACE_DEBUG_IMAGE_HEIGHT];

#endif /* FACE_DEBUG_IMAGE_H */
