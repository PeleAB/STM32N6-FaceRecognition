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

/* ========================================================================= */
/* IMPLEMENTATION FUNCTIONS                                                  */
/* ========================================================================= */

/**
 * @brief Compute the target embedding as average of all embeddings in bank
 * @note This function is called automatically when embeddings are added
 */

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

