/**
 * @file sysobj_params.h
 * @brief Persistent parameter store — triple-bank NOR flash with CRC32
 *        protection and majority-vote recovery.
 */

#ifndef SYSOBJ_PARAMS_H
#define SYSOBJ_PARAMS_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Hard platform limit: max parameters per page (15 entries × 16 bytes = 240 B). */
#define PARAMS_MAX_ENTRIES 15U

typedef enum {
  PARAM_TYPE_U32 = 0,
  PARAM_TYPE_U64 = 1,
} param_type_t;

typedef struct {
  uint16_t     id;
  param_type_t type;
  uint64_t     default_val;
  uint64_t     min_val;
  uint64_t     max_val;
} param_descriptor_t;

/** Optional hook called immediately before erase+write (e.g. pause inference). */
typedef void (*params_hook_t)(void);

typedef struct {
  uint32_t                  flash_base_addr; /*!< NOR flash byte address of bank 0 (sector-aligned). */
  uint32_t                  xspi_instance;   /*!< BSP XSPI instance index (usually 0). */
  uint32_t                  mmp_base_addr;   /*!< AXI-mapped window base (e.g. XSPI2_BASE); 0 to use BSP read. */
  params_hook_t             pre_write_hook;  /*!< Called before erase+write. May be NULL. */
  params_hook_t             post_write_hook; /*!< Called after write completes. May be NULL. */
  const param_descriptor_t *table;           /*!< Application parameter descriptor table. */
  uint16_t                  table_count;     /*!< Number of entries in table (≤ PARAMS_MAX_ENTRIES). */
} params_cfg_t;

typedef enum {
  PARAMS_OK = 0,
  PARAMS_ERR_NOT_INIT,
  PARAMS_ERR_NOT_FOUND,
  PARAMS_ERR_OUT_OF_RANGE,
  PARAMS_ERR_FLASH,
  PARAMS_ERR_ALL_CORRUPT, /*!< All 3 banks invalid — returned default value. */
} params_status_t;

typedef struct {
  bool valid[3]; /*!< valid[n] = true if bank n passed magic+CRC checks. */
} params_bank_health_t;

/**
 * @brief Initialize the parameter store.
 * Must be called after BSP_PlatformInit (XSPI must be ready).
 * Creates the internal FreeRTOS mutex.
 * @param cfg Configuration pointer (must remain valid for the lifetime of use).
 * @return PARAMS_OK or PARAMS_ERR_ALL_CORRUPT (recoverable — defaults used).
 */
params_status_t sysobj_params_init(const params_cfg_t *cfg);

/**
 * @brief Read a parameter by ID.
 * @param id              Parameter ID.
 * @param out_value       Receives the value (defaults to descriptor default on error/corrupt).
 * @param out_was_default Set to true if the default value was used.
 * @return PARAMS_OK, PARAMS_ERR_NOT_INIT, PARAMS_ERR_NOT_FOUND, PARAMS_ERR_ALL_CORRUPT.
 */
params_status_t sysobj_params_read(uint16_t id, uint64_t *out_value,
                                   bool *out_was_default);

/**
 * @brief Write a parameter by ID.
 * Validates bounds, then writes to all 3 banks.
 * @param id    Parameter ID.
 * @param value New value (must be within [min_val, max_val]).
 * @return PARAMS_OK, PARAMS_ERR_NOT_INIT, PARAMS_ERR_NOT_FOUND,
 *         PARAMS_ERR_OUT_OF_RANGE, PARAMS_ERR_FLASH.
 */
params_status_t sysobj_params_write(uint16_t id, uint64_t value);

/**
 * @brief Query the health (validity) of each bank for a given parameter.
 * @param id  Parameter ID (pass any valid ID to check page-level health).
 * @param out Receives per-bank validity flags.
 * @return PARAMS_OK or PARAMS_ERR_NOT_INIT.
 */
params_status_t sysobj_params_bank_health(uint16_t id,
                                          params_bank_health_t *out);

#ifdef __cplusplus
}
#endif

#endif /* SYSOBJ_PARAMS_H */
