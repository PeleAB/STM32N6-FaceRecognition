/**
******************************************************************************
* @file    face_utils_student.h
* @author  GPM Application Team (Student Implementation)
*
******************************************************************************
* @attention
*
* Copyright (c) 2024 STMicroelectronics.
* All rights reserved.
*
* This software is licensed under terms that can be found in the LICENSE file
* in the root directory of this software component.
* If no LICENSE file comes with this software, it is provided AS-IS.
*
******************************************************************************
*/

#ifndef FACE_UTILS_STUDENT_H
#define FACE_UTILS_STUDENT_H

#include "arm_math.h"

/* ========================================================================= */
/* STUDENT IMPLEMENTATION FUNCTIONS                                          */
/* ========================================================================= */
/* These functions are for student implementation.                          */
/* Students should implement these functions according to specifications.   */
/* ========================================================================= */

/**
 * @brief Calculate cosine similarity between two embedding vectors
 * @param emb1 First embedding vector
 * @param emb2 Second embedding vector
 * @param len Length of embedding vectors
 * @return Cosine similarity value between -1.0 and 1.0
 * @note Returns 0.0 if either vector has zero norm
 * 
 * @details Cosine similarity is calculated as:
 *          similarity = dot_product(emb1, emb2) / (norm(emb1) * norm(emb2))
 *          
 *          Where:
 *          - dot_product = sum(emb1[i] * emb2[i]) for i = 0 to len-1
 *          - norm(emb) = sqrt(sum(emb[i]^2)) for i = 0 to len-1
 *          
 *          This measures the cosine of the angle between two vectors.
 *          Values close to 1.0 indicate similar vectors.
 *          Values close to -1.0 indicate opposite vectors.
 *          Values close to 0.0 indicate orthogonal vectors.
 */
float embedding_cosine_similarity(const float *emb1, const float *emb2, uint32_t len);

#endif /* FACE_UTILS_STUDENT_H */