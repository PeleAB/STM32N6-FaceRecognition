/**
 ******************************************************************************
 * @file    face_utils.c
 * @brief   Face recognition utility functions
 ******************************************************************************
 */

#include "face_utils.h"
#include <math.h>
#include <stddef.h>

/* ========================================================================= */
/* IMPLEMENTATION FUNCTIONS                                                  */
/* ========================================================================= */

/**
 * @brief Calculate cosine similarity between two embedding vectors
 * @param emb1 First embedding vector
 * @param emb2 Second embedding vector
 * @param len Length of embedding vectors
 * @return Cosine similarity value between -1.0 and 1.0
 * @note Returns 0.0 if either vector has zero norm
 */
float embedding_cosine_similarity(const float *emb1, const float *emb2, uint32_t len)
{
    if (!emb1 || !emb2 || len == 0) {
        return 0.0f;
    }
    
    float dot_product = 0.0f;
    float norm1_squared = 0.0f;
    float norm2_squared = 0.0f;
    
    /* Calculate dot product and squared norms in single pass */
    for (uint32_t i = 0; i < len; i++) {
        const float val1 = emb1[i];
        const float val2 = emb2[i];
        
        dot_product += val1 * val2;
        norm1_squared += val1 * val1;
        norm2_squared += val2 * val2;
    }
    
    /* Check for zero norms to avoid division by zero */
    if (norm1_squared == 0.0f || norm2_squared == 0.0f) {
        return 0.0f;
    }
    
    /* Calculate cosine similarity */
    return dot_product / sqrtf(norm1_squared * norm2_squared);
}

