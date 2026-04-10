/**
 * @file sysobj_timer.h
 * @brief Hardware timer sysobj — wraps STM32 HAL TIM with a simple API for
 *        one-shot, fixed-count, or infinite repeating callbacks.
 *
 * Usage:
 *   1. Configure a TIM peripheral via STM32CubeMX (TIM Base, enable global IRQ).
 *   2. Call sysobj_timer_init() with the htim handle and desired period/units.
 *   3. Call sysobj_timer_start() to start the timer.
 *   4. Route HAL_TIM_PeriodElapsedCallback → sysobj_timer_irq_handler().
 *   5. Call sysobj_timer_stop() or let it auto-stop after N repetitions.
 *
 * Thread safety: sysobj_timer_stop() is safe from any context (IRQ or task).
 * Callbacks execute in the TIM interrupt context.
 */

#ifndef SYSOBJ_TIMER_H
#define SYSOBJ_TIMER_H

#include "stm32n6xx_hal.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum number of concurrently registered timer instances. */
#define SYSOBJ_TIMER_MAX_INSTANCES 8U

/** Pass as repeat to fire exactly once. */
#define SYSOBJ_TIMER_REPEAT_ONCE     1

/** Pass as repeat to repeat indefinitely until sysobj_timer_stop() is called. */
#define SYSOBJ_TIMER_REPEAT_INFINITE (-1)

typedef enum {
  SYSOBJ_TIMER_UNIT_NSEC = 0, /*!< Nanoseconds */
  SYSOBJ_TIMER_UNIT_USEC,     /*!< Microseconds */
  SYSOBJ_TIMER_UNIT_MSEC,     /*!< Milliseconds */
  SYSOBJ_TIMER_UNIT_SEC,      /*!< Seconds */
} sysobj_timer_unit_t;

typedef enum {
  SYSOBJ_TIMER_OK = 0,
  SYSOBJ_TIMER_ERR_NULL,      /*!< NULL pointer argument. */
  SYSOBJ_TIMER_ERR_PERIOD,    /*!< Period is zero or negative. */
  SYSOBJ_TIMER_ERR_OVERFLOW,  /*!< Period too large for the given clock / counter width. */
  SYSOBJ_TIMER_ERR_TOO_MANY,  /*!< SYSOBJ_TIMER_MAX_INSTANCES exceeded. */
  SYSOBJ_TIMER_ERR_NOT_INIT,  /*!< sysobj_timer_init() not called yet. */
  SYSOBJ_TIMER_ERR_HAL,       /*!< HAL_TIM_Base_Start_IT() returned error. */
} sysobj_timer_error_t;

/** Callback invoked (from TIM IRQ context) each time the period elapses. */
typedef void (*sysobj_timer_cb_t)(void *user_data);

typedef struct {
  TIM_HandleTypeDef  *htim;        /*!< HAL TIM handle (must be HAL-initialised). */
  uint32_t            tim_clock_hz;/*!< Timer input clock frequency in Hz. */
  bool                is_32bit;    /*!< true if counter is 32-bit (e.g. TIM2/5), false for 16-bit. */
  uint32_t            period;      /*!< Desired period value (interpreted in 'unit'). */
  sysobj_timer_unit_t unit;        /*!< Unit of 'period'. */
  int32_t             repeat;      /*!< SYSOBJ_TIMER_REPEAT_ONCE, SYSOBJ_TIMER_REPEAT_INFINITE, or N > 0. */
  sysobj_timer_cb_t   callback;    /*!< Function called on each elapsed event. May be NULL. */
  void               *user_data;   /*!< Passed verbatim to callback. */
} sysobj_timer_cfg_t;

typedef struct {
  sysobj_timer_cfg_t cfg;          /*!< Configuration copy (set during init). */
  volatile int32_t   remaining;    /*!< Repetitions remaining; -1 = infinite. */
  volatile bool      running;      /*!< true while the timer is active. */
  uint32_t           _psc;         /*!< Computed prescaler value (PSC). */
  uint32_t           _arr;         /*!< Computed auto-reload value (ARR). */
  bool               _initialized; /*!< Set to true by sysobj_timer_init(). */
} sysobj_timer_t;

/**
 * @brief Initialise a timer instance and register it for IRQ dispatch.
 *
 * Computes PSC and ARR from the desired period/unit/clock.
 * Does NOT start the hardware timer.
 *
 * @param timer  Pointer to a caller-allocated sysobj_timer_t.
 * @param cfg    Configuration (copied internally).
 * @return SYSOBJ_TIMER_OK or an error code.
 */
sysobj_timer_error_t sysobj_timer_init(sysobj_timer_t *timer,
                                       const sysobj_timer_cfg_t *cfg);

/**
 * @brief Start (or restart) the timer.
 *
 * Programs PSC/ARR into the hardware, generates an update event to load the
 * shadow registers, clears the update flag, then calls HAL_TIM_Base_Start_IT.
 *
 * @param timer Initialised timer instance.
 * @return SYSOBJ_TIMER_OK or an error code.
 */
sysobj_timer_error_t sysobj_timer_start(sysobj_timer_t *timer);

/**
 * @brief Stop the timer immediately.
 *
 * Safe to call from any context (task or ISR).
 *
 * @param timer Running timer instance.
 */
void sysobj_timer_stop(sysobj_timer_t *timer);

/**
 * @brief De-initialise the timer and unregister it from IRQ dispatch.
 * @param timer Initialised timer instance.
 */
void sysobj_timer_deinit(sysobj_timer_t *timer);

/**
 * @brief Query whether the timer is currently running.
 * @param timer Timer instance.
 * @return true if running.
 */
bool sysobj_timer_is_running(const sysobj_timer_t *timer);

/**
 * @brief Return the number of repetitions remaining (snapshot, not atomic).
 * Returns -1 for infinite timers, 0 when stopped.
 * @param timer Timer instance.
 */
int32_t sysobj_timer_get_remaining(const sysobj_timer_t *timer);

/**
 * @brief Must be called from HAL_TIM_PeriodElapsedCallback.
 *
 * Routes the interrupt to the correct sysobj_timer_t instance, fires the
 * callback, and decrements/stops the repetition counter.
 *
 * @param htim The TIM handle passed by HAL.
 */
void sysobj_timer_irq_handler(TIM_HandleTypeDef *htim);

#ifdef __cplusplus
}
#endif

#endif /* SYSOBJ_TIMER_H */
