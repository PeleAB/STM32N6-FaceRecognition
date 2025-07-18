/**
 ******************************************************************************
 * @file    target_embedding.c
 * @brief   Target embedding management functions
 ******************************************************************************
 */

#include "target_embedding.h"
#include <string.h>
#include <math.h>

/* ========================================================================= */
/* GLOBAL VARIABLES                                                          */
/* ========================================================================= */

float target_embedding[EMBEDDING_SIZE];                                /**< Current target embedding (averaged) */
static float embedding_bank[EMBEDDING_BANK_SIZE][EMBEDDING_SIZE];     /**< Bank of stored embeddings */
static int bank_count = 0;                                            /**< Current number of embeddings in bank */

#ifdef STUDENT_MODE
/* ========================================================================= */
/* STUDENT IMPLEMENTATION FUNCTIONS                                          */
/* ========================================================================= */
/* TODO: Implement these functions according to the specifications          */
/* ========================================================================= */

/**
 * @brief Compute the target embedding as average of all embeddings in bank
 * @note This function is called automatically when embeddings are added
 * TODO: Students should implement this function
 */
static void compute_target(void)
{
    /* TODO: Compute average normalized embedding
     * Steps:
     * 1. If bank_count == 0:
     *    - Zero out target_embedding array
     *    - Return
     * 2. Create sum array and zero it out
     * 3. For each embedding in bank:
     *    - Add it to the sum array (element-wise)
     * 4. Divide each sum element by bank_count to get average
     * 5. Calculate norm of the average embedding
     * 6. If norm > 0:
     *    - Normalize target_embedding by dividing by norm
     * 
     * This creates a representative embedding from multiple samples.
     * Normalization ensures the embedding has unit length for cosine similarity.
     */
    
    (void)bank_count;  // Remove unused parameter warnings
    
    // STUDENT IMPLEMENTATION GOES HERE
}

/**
 * @brief Initialize the embeddings bank to empty state
 * TODO: Initialize all arrays and counters to zero
 */
void embeddings_bank_init(void)
{
    /* TODO: Initialize embeddings bank
     * Steps:
     * 1. Set bank_count to 0
     * 2. Zero out embedding_bank array: memset(embedding_bank, 0, sizeof(embedding_bank))
     * 3. Zero out target_embedding array: memset(target_embedding, 0, sizeof(target_embedding))
     */
    
    // STUDENT IMPLEMENTATION GOES HERE
}

/**
 * @brief Add a new embedding to the bank
 * TODO: Normalize and store embedding, then recompute target
 */
int embeddings_bank_add(const float *embedding)
{
    /* TODO: Add normalized embedding to bank
     * Steps:
     * 1. Check if bank is full (bank_count >= EMBEDDING_BANK_SIZE):
     *    - Return -1 if full
     * 2. Calculate norm of input embedding:
     *    - norm = sqrt(sum of squares of all elements)
     * 3. If norm == 0:
     *    - Return -1 (invalid embedding)
     * 4. Normalize and store embedding in bank:
     *    - embedding_bank[bank_count][i] = embedding[i] / norm
     * 5. Increment bank_count
     * 6. Call compute_target() to update target embedding
     * 7. Return bank_count
     * 
     * Normalization ensures all embeddings have unit length.
     * This is important for cosine similarity calculations.
     */
    
    (void)embedding;  // Remove unused parameter warning
    
    // STUDENT IMPLEMENTATION GOES HERE
    
    return -1;  // Replace with actual implementation
}

/**
 * @brief Reset the embeddings bank to empty state
 * TODO: Call initialization function
 */
void embeddings_bank_reset(void)
{
    /* TODO: Reset embeddings bank
     * This is simply a call to embeddings_bank_init()
     */
    
    // STUDENT IMPLEMENTATION GOES HERE
}

/**
 * @brief Get the current number of embeddings in the bank
 * TODO: Return the current bank count
 */
int embeddings_bank_count(void)
{
    /* TODO: Return current bank count
     * Simply return the bank_count variable
     */
    
    // STUDENT IMPLEMENTATION GOES HERE
    
    return 0;  // Replace with actual bank_count
}

#else
/* ========================================================================= */
/* INSTRUCTOR IMPLEMENTATION FUNCTIONS                                       */
/* ========================================================================= */

static void compute_target(void)
{
    if (bank_count == 0)
    {
        memset(target_embedding, 0, sizeof(target_embedding));
        return;
    }
    float sum[EMBEDDING_SIZE];
    memset(sum, 0, sizeof(sum));
    for (int n = 0; n < bank_count; n++)
    {
        for (int i = 0; i < EMBEDDING_SIZE; i++)
        {
            sum[i] += embedding_bank[n][i];
        }
    }
    for (int i = 0; i < EMBEDDING_SIZE; i++)
    {
        target_embedding[i] = sum[i] / (float)bank_count;
    }
    float norm = 0.f;
    for (int i = 0; i < EMBEDDING_SIZE; i++)
    {
        norm += target_embedding[i] * target_embedding[i];
    }
    norm = sqrtf(norm);
    if (norm > 0.f)
    {
        for (int i = 0; i < EMBEDDING_SIZE; i++)
        {
            target_embedding[i] /= norm;
        }
    }
}

void embeddings_bank_init(void)
{
    bank_count = 0;
    memset(embedding_bank, 0, sizeof(embedding_bank));
    memset(target_embedding, 0, sizeof(target_embedding));
}

int embeddings_bank_add(const float *embedding)
{
    if (bank_count >= EMBEDDING_BANK_SIZE)
        return -1;
    float norm = 0.f;
    for (int i = 0; i < EMBEDDING_SIZE; i++)
    {
        norm += embedding[i] * embedding[i];
    }
    norm = sqrtf(norm);
    if (norm == 0.f)
        return -1;
    for (int i = 0; i < EMBEDDING_SIZE; i++)
    {
        embedding_bank[bank_count][i] = embedding[i] / norm;
    }
    bank_count++;
    compute_target();
    return bank_count;
}

void embeddings_bank_reset(void)
{
    embeddings_bank_init();
}

int embeddings_bank_count(void)
{
    return bank_count;
}

#endif /* STUDENT_MODE */