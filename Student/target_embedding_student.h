/**
******************************************************************************
* @file    target_embedding_student.h
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

#ifndef TARGET_EMBEDDING_STUDENT_H
#define TARGET_EMBEDDING_STUDENT_H

#include "arm_math.h"

/* ========================================================================= */
/* EMBEDDING CONFIGURATION                                                   */
/* ========================================================================= */

#define EMBEDDING_SIZE 128          /**< Size of each face embedding vector */
#define EMBEDDING_BANK_SIZE 10      /**< Maximum number of embeddings in bank */

/* ========================================================================= */
/* GLOBAL VARIABLES                                                          */
/* ========================================================================= */

extern float target_embedding[EMBEDDING_SIZE];  /**< Current target embedding (averaged) */

/* ========================================================================= */
/* STUDENT IMPLEMENTATION FUNCTIONS                                          */
/* ========================================================================= */
/* These functions are for student implementation.                          */
/* Students should implement these functions according to specifications.   */
/* ========================================================================= */

/**
 * @brief Initialize the embeddings bank to empty state
 * @note Resets all embeddings to zero and sets count to 0
 * 
 * @details This function should:
 *          - Reset the bank count to 0
 *          - Zero out the embedding bank array
 *          - Zero out the target embedding array
 */
void embeddings_bank_init(void);

/**
 * @brief Add a new embedding to the bank
 * @param embedding Pointer to embedding vector to add
 * @return Number of embeddings in bank after addition, or -1 on error
 * @note The embedding is normalized before storage
 * @note Returns -1 if bank is full or embedding has zero norm
 * 
 * @details This function should:
 *          1. Check if bank is full (return -1 if so)
 *          2. Calculate the norm of the input embedding
 *          3. Return -1 if norm is zero
 *          4. Normalize the embedding and store it in the bank
 *          5. Increment bank count
 *          6. Recompute the target embedding
 *          7. Return the new bank count
 */
int embeddings_bank_add(const float *embedding);

/**
 * @brief Reset the embeddings bank to empty state
 * @note This is equivalent to calling embeddings_bank_init()
 */
void embeddings_bank_reset(void);

/**
 * @brief Get the current number of embeddings in the bank
 * @return Current number of embeddings stored in the bank
 */
int embeddings_bank_count(void);

#endif /* TARGET_EMBEDDING_STUDENT_H */