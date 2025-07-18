#ifndef TARGET_EMBEDDING_H
#define TARGET_EMBEDDING_H

#include "arm_math.h"
#include "app_config.h"

#ifdef STUDENT_MODE
#include "../Student/target_embedding_student.h"
#else
/* Instructor implementation functions */

#define EMBEDDING_SIZE 128
#define EMBEDDING_BANK_SIZE 10

extern float target_embedding[EMBEDDING_SIZE];

void embeddings_bank_init(void);
int  embeddings_bank_add(const float *embedding);
void embeddings_bank_reset(void);
int  embeddings_bank_count(void);

#endif /* STUDENT_MODE */

#endif /* TARGET_EMBEDDING_H */
