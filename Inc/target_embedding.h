#ifndef TARGET_EMBEDDING_H
#define TARGET_EMBEDDING_H

#include "arm_math.h"
#include "app_config.h"

/* Target embedding constants */
#define EMBEDDING_SIZE 128
#define EMBEDDING_BANK_SIZE 10

/* Global target embedding */
extern float target_embedding[EMBEDDING_SIZE];

/* Target embedding function prototypes */
void embeddings_bank_init(void);
int  embeddings_bank_add(const float *embedding);
void embeddings_bank_reset(void);
int  embeddings_bank_count(void);

#endif /* TARGET_EMBEDDING_H */
