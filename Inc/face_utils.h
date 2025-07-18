#ifndef FACE_UTILS_H
#define FACE_UTILS_H

#include "arm_math.h"
#include "app_config.h"

#ifdef STUDENT_MODE
#include "../Student/face_utils_student.h"
#else
/* Instructor implementation functions */

float embedding_cosine_similarity(const float *emb1, const float *emb2, uint32_t len);

#endif /* STUDENT_MODE */

#endif /* FACE_UTILS_H */
