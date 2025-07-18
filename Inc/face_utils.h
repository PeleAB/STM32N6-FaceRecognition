#ifndef FACE_UTILS_H
#define FACE_UTILS_H

#include "arm_math.h"
#include "app_config.h"

/* Face recognition utility function prototypes */
float embedding_cosine_similarity(const float *emb1, const float *emb2, uint32_t len);

#endif /* FACE_UTILS_H */
