#include "face_utils.h"
#include <math.h>

float embedding_cosine_similarity(const float *emb1, const float *emb2, uint32_t len)
{
    float dot = 0.f;
    float norm1 = 0.f;
    float norm2 = 0.f;
    for (uint32_t i = 0; i < len; i++)
    {
        dot += emb1[i] * emb2[i];
        norm1 += emb1[i] * emb1[i];
        norm2 += emb2[i] * emb2[i];
    }
    if (norm1 == 0.f || norm2 == 0.f)
    {
        return 0.f;
    }
    return dot / sqrtf(norm1 * norm2);
}
