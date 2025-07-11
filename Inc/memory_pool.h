/**
 ******************************************************************************
 * @file    memory_pool.h
 * @author  Application Team
 * @brief   Centralized memory pool management for optimized memory access
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2023 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include <stdint.h>
#include <stdbool.h>
#include "app_constants.h"
#include "app_config.h"

/* ========================================================================= */
/* MEMORY POOL CONSTANTS                                                     */
/* ========================================================================= */
#define MEMORY_POOL_MAX_BUFFERS         16  /**< Maximum number of managed buffers */
#define MEMORY_POOL_NAME_LENGTH         32  /**< Maximum buffer name length */

/* ========================================================================= */
/* MEMORY BUFFER TYPES                                                       */
/* ========================================================================= */
typedef enum {
    MEMORY_BUFFER_TYPE_NN_INPUT,        /**< Neural network input buffer */
    MEMORY_BUFFER_TYPE_NN_OUTPUT,       /**< Neural network output buffer */
    MEMORY_BUFFER_TYPE_FRAME_CAPTURE,   /**< Frame capture buffer */
    MEMORY_BUFFER_TYPE_PREPROCESSING,   /**< Preprocessing buffer */
    MEMORY_BUFFER_TYPE_POSTPROCESSING,  /**< Postprocessing buffer */
    MEMORY_BUFFER_TYPE_TRACKING,        /**< Tracking buffer */
    MEMORY_BUFFER_TYPE_PROTOCOL,        /**< Protocol buffer */
    MEMORY_BUFFER_TYPE_TEMPORARY,       /**< Temporary buffer */
    MEMORY_BUFFER_TYPE_COUNT
} memory_buffer_type_t;

/* ========================================================================= */
/* MEMORY BUFFER STRUCTURE                                                   */
/* ========================================================================= */
typedef struct {
    void *ptr;                          /**< Buffer pointer */
    size_t size;                        /**< Buffer size in bytes */
    size_t alignment;                   /**< Buffer alignment requirement */
    memory_buffer_type_t type;          /**< Buffer type */
    char name[MEMORY_POOL_NAME_LENGTH]; /**< Buffer name for debugging */
    bool is_allocated;                  /**< Allocation status */
    bool is_cached;                     /**< Cache coherency required */
    uint32_t access_count;              /**< Access statistics */
    uint32_t last_access_time;          /**< Last access timestamp */
} memory_buffer_t;

/* ========================================================================= */
/* MEMORY POOL STRUCTURE                                                     */
/* ========================================================================= */
typedef struct {
    memory_buffer_t buffers[MEMORY_POOL_MAX_BUFFERS];  /**< Managed buffers */
    uint32_t buffer_count;                             /**< Number of active buffers */
    uint32_t total_allocated;                          /**< Total allocated memory */
    uint32_t peak_allocated;                           /**< Peak memory usage */
    uint32_t allocation_failures;                      /**< Allocation failure count */
    bool is_initialized;                               /**< Initialization status */
} memory_pool_t;

/* ========================================================================= */
/* MEMORY STATISTICS STRUCTURE                                               */
/* ========================================================================= */
typedef struct {
    uint32_t total_memory;              /**< Total managed memory */
    uint32_t used_memory;               /**< Currently used memory */
    uint32_t free_memory;               /**< Available memory */
    uint32_t fragmentation_percent;     /**< Memory fragmentation percentage */
    uint32_t cache_hit_rate;            /**< Cache hit rate percentage */
    uint32_t allocation_count;          /**< Total allocations */
    uint32_t deallocation_count;        /**< Total deallocations */
} memory_statistics_t;

/* ========================================================================= */
/* FUNCTION PROTOTYPES                                                       */
/* ========================================================================= */

/**
 * @brief Initialize memory pool manager
 * @param pool Pointer to memory pool structure
 * @return 0 on success, negative on error
 */
int memory_pool_init(memory_pool_t *pool);

/**
 * @brief Allocate aligned memory buffer
 * @param pool Pointer to memory pool structure
 * @param size Buffer size in bytes
 * @param alignment Memory alignment requirement
 * @param type Buffer type
 * @param name Buffer name for debugging
 * @return Pointer to allocated buffer or NULL on failure
 */
void *memory_pool_alloc(memory_pool_t *pool, size_t size, size_t alignment, 
                        memory_buffer_type_t type, const char *name);

/**
 * @brief Free memory buffer
 * @param pool Pointer to memory pool structure
 * @param ptr Pointer to buffer to free
 * @return 0 on success, negative on error
 */
int memory_pool_free(memory_pool_t *pool, void *ptr);

/**
 * @brief Get buffer information by pointer
 * @param pool Pointer to memory pool structure
 * @param ptr Buffer pointer
 * @return Pointer to buffer structure or NULL if not found
 */
memory_buffer_t *memory_pool_get_buffer(memory_pool_t *pool, void *ptr);

/**
 * @brief Get buffer information by name
 * @param pool Pointer to memory pool structure
 * @param name Buffer name
 * @return Pointer to buffer structure or NULL if not found
 */
memory_buffer_t *memory_pool_get_buffer_by_name(memory_pool_t *pool, const char *name);

/**
 * @brief Invalidate cache for buffer
 * @param pool Pointer to memory pool structure
 * @param ptr Buffer pointer
 * @return 0 on success, negative on error
 */
int memory_pool_invalidate_cache(memory_pool_t *pool, void *ptr);

/**
 * @brief Clean and invalidate cache for buffer
 * @param pool Pointer to memory pool structure
 * @param ptr Buffer pointer
 * @return 0 on success, negative on error
 */
int memory_pool_clean_invalidate_cache(memory_pool_t *pool, void *ptr);

/**
 * @brief Get memory pool statistics
 * @param pool Pointer to memory pool structure
 * @param stats Pointer to statistics structure
 * @return 0 on success, negative on error
 */
int memory_pool_get_statistics(memory_pool_t *pool, memory_statistics_t *stats);

/**
 * @brief Print memory pool information
 * @param pool Pointer to memory pool structure
 */
void memory_pool_print_info(memory_pool_t *pool);

/**
 * @brief Defragment memory pool
 * @param pool Pointer to memory pool structure
 * @return 0 on success, negative on error
 */
int memory_pool_defragment(memory_pool_t *pool);

/**
 * @brief Validate memory pool integrity
 * @param pool Pointer to memory pool structure
 * @return true if valid, false otherwise
 */
bool memory_pool_validate(memory_pool_t *pool);

/**
 * @brief Reset memory pool
 * @param pool Pointer to memory pool structure
 * @return 0 on success, negative on error
 */
int memory_pool_reset(memory_pool_t *pool);

/* ========================================================================= */
/* CONVENIENCE MACROS                                                        */
/* ========================================================================= */

/**
 * @brief Allocate cache-aligned buffer
 */
#define MEMORY_POOL_ALLOC_CACHED(pool, size, type, name) \
    memory_pool_alloc(pool, size, CACHE_LINE_ALIGNMENT, type, name)

/**
 * @brief Allocate 32-byte aligned buffer
 */
#define MEMORY_POOL_ALLOC_32(pool, size, type, name) \
    memory_pool_alloc(pool, size, 32, type, name)

/**
 * @brief Allocate 16-byte aligned buffer
 */
#define MEMORY_POOL_ALLOC_16(pool, size, type, name) \
    memory_pool_alloc(pool, size, 16, type, name)

#endif /* MEMORY_POOL_H */