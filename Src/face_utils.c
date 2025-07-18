/**
 ******************************************************************************
 * @file    face_utils.c
 * @brief   Face recognition utility functions
 ******************************************************************************
 */

#include "face_utils.h"
#include <math.h>
#include <stddef.h>

#ifdef STUDENT_MODE
/* ========================================================================= */
/* STUDENT IMPLEMENTATION FUNCTIONS                                          */
/* ========================================================================= */
/* TODO: Implement these functions according to the specifications          */
/* ========================================================================= */

/**
 * @brief Calculate cosine similarity between two embedding vectors
 * TODO: Implement cosine similarity calculation
 * HINT: cosine_similarity = dot_product / (norm1 * norm2)
 */
float embedding_cosine_similarity(const float *emb1, const float *emb2, uint32_t len)
{
    /* TODO: Implement cosine similarity calculation
     * Steps:
     * 1. Check input parameters (return 0.0f if invalid)
     * 2. Initialize variables for dot_product, norm1_squared, norm2_squared
     * 3. In a single loop through all elements:
     *    - Calculate dot_product += emb1[i] * emb2[i]
     *    - Calculate norm1_squared += emb1[i] * emb1[i]
     *    - Calculate norm2_squared += emb2[i] * emb2[i]
     * 4. Check for zero norms (return 0.0f if either norm is zero)
     * 5. Return dot_product / sqrtf(norm1_squared * norm2_squared)
     * 
     * Mathematical background:
     * - Cosine similarity measures the cosine of the angle between two vectors
     * - It's independent of vector magnitude, only depends on direction
     * - Values range from -1.0 (opposite) to +1.0 (identical direction)
     * - Used in face recognition to compare face embeddings
     */
    
    // Input validation
    if (!emb1 || !emb2 || len == 0) {
        return 0.0f;
    }
    
    // Initialize accumulator variables
    float dot_product = 0.0f;
    float norm1_squared = 0.0f;
    float norm2_squared = 0.0f;
    
    // TODO: STUDENT IMPLEMENTATION GOES HERE
    // Calculate dot product and squared norms in single pass
    
    // TODO: Check for zero norms to avoid division by zero
    
    // TODO: Calculate and return cosine similarity
    
    (void)emb1;       // Remove unused parameter warnings
    (void)emb2;       // Remove these lines when implementing
    (void)len;
    (void)dot_product;
    (void)norm1_squared;
    (void)norm2_squared;
    
    return 0.0f;  // Replace with actual calculation
}

#else
/* ========================================================================= */
/* INSTRUCTOR IMPLEMENTATION FUNCTIONS                                       */
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

#endif /* STUDENT_MODE */