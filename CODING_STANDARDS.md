# Coding Standards - STM32N6 Face Recognition System

This document defines the coding standards for the STM32N6 Face Recognition System, following embedded C best practices for bare metal systems.

## General Principles

### Deterministic Behavior
- Code must behave predictably with bounded execution time
- Avoid recursive functions and dynamic memory allocation
- Use static analysis to verify worst-case execution times

### Resource Awareness
- Always consider RAM, ROM, and CPU cycle constraints
- Every byte and cycle matters in embedded systems
- Use const for ROM storage, volatile for hardware registers

### Hardware First
- Understand the underlying hardware before writing software
- Read datasheets thoroughly
- Use hardware abstraction layers consistently

## Naming Conventions

### Variables and Functions
- Use descriptive names that indicate purpose and scope
- Use snake_case for variables and functions
- Use ALL_CAPS for macros and constants
- Prefix global variables with `g_`
- Prefix static variables with `s_`

```c
// Good examples
static uint8_t s_uart_tx_buffer[UART_TX_BUFFER_SIZE];
volatile uint32_t g_system_tick_count;
bool is_motor_enabled;
void adc_start_conversion(uint8_t channel);

// Bad examples
static uint8_t buf[64];
volatile uint32_t counter;
bool flag;
void start(uint8_t ch);
```

### Hardware-Specific Naming
- Use peripheral abbreviations consistently
- Include bit positions in register bit field names
- Use descriptive names for pin assignments

```c
// Register definitions
#define UART1_CR1_UE_BIT        (0)
#define UART1_CR1_TE_BIT        (3)
#define UART1_SR_TXE_BIT        (7)

// Pin definitions
#define LED_STATUS_PORT         GPIOA
#define LED_STATUS_PIN          GPIO_PIN_5
```

### Module Prefixes
Use consistent prefixes for related functions:

```c
// UART module
void uart_init(void);
void uart_transmit_byte(uint8_t data);
uint8_t uart_receive_byte(void);
bool uart_is_tx_ready(void);
```

## Data Types and Constants

### Standard Types
- Use stdint.h types for explicit sizing
- Avoid `int` and `char` - be explicit about size
- Use bool from stdbool.h

```c
// Good
uint8_t sensor_count;
int16_t temperature_celsius;
uint32_t timestamp_ms;
bool is_system_ready;

// Bad
int count;
char temp;
unsigned long time;
int ready;
```

### Enumerations
- Use enums for state machines and options
- Specify underlying type when size matters
- Include invalid/unknown values

```c
typedef enum : uint8_t {
    MOTOR_STATE_STOPPED,
    MOTOR_STATE_ACCELERATING,
    MOTOR_STATE_RUNNING,
    MOTOR_STATE_DECELERATING,
    MOTOR_STATE_ERROR,
    MOTOR_STATE_UNKNOWN = 0xFF
} motor_state_t;
```

## Function Design

### Function Signatures
- Keep parameter lists short (≤5 parameters)
- Use const for input parameters
- Return error codes or use output parameters

```c
typedef enum {
    ADC_RESULT_OK,
    ADC_RESULT_TIMEOUT,
    ADC_RESULT_INVALID_CHANNEL
} adc_result_t;

adc_result_t adc_read_channel(uint8_t channel, uint16_t *result);
```

### Function Documentation
Document all public functions with:
- Brief description
- Parameter descriptions
- Return value description
- Timing requirements
- Safety notes

```c
/**
 * @brief Reads temperature from the specified sensor
 * 
 * This function initiates an ADC conversion and waits for completion.
 * The conversion takes approximately 12.5µs on this hardware.
 * 
 * @param[in]  sensor_id    Sensor identifier (0 to MAX_TEMP_SENSORS-1)
 * @param[out] temperature  Pointer to store temperature in 0.1°C units
 * @param[in]  timeout_ms   Maximum time to wait for conversion (1-10000ms)
 * 
 * @return SENSOR_OK on success, error code otherwise
 * 
 * @note This function blocks until conversion completes or timeout
 * @warning Do not call from interrupt context
 */
sensor_result_t sensor_read_temperature(uint8_t sensor_id, 
                                       int16_t *temperature, 
                                       uint32_t timeout_ms);
```

## Memory Management

### Stack Usage
- Minimize stack usage in deeply nested calls
- Avoid large local arrays
- Use static allocation where possible

```c
// Good - static allocation
static uint8_t s_sensor_readings[MAX_SENSORS];

void process_sensors(void)
{
    read_all_sensors(s_sensor_readings, MAX_SENSORS);
}

// Bad - large stack allocation
void process_sensors(void)
{
    uint8_t sensor_readings[MAX_SENSORS]; // Too much stack usage
    read_all_sensors(sensor_readings, MAX_SENSORS);
}
```

### Const and Volatile Usage
- Use `const` for read-only data (stored in ROM)
- Use `volatile` for hardware registers and interrupt-shared data
- Combine appropriately for hardware registers

```c
// Hardware register (read-only)
volatile const uint32_t * const STATUS_REG = (uint32_t *)0x40000000;

// Hardware register (read-write)
volatile uint32_t * const CONTROL_REG = (uint32_t *)0x40000004;

// Configuration data in ROM
const motor_config_t MOTOR_CONFIGS[NUM_MOTORS] = {
    {.max_speed = 1000, .acceleration = 50},
    {.max_speed = 800,  .acceleration = 30}
};
```

## Hardware Abstraction

### Register Access
- Create hardware abstraction layers (HAL)
- Use bit manipulation macros consistently
- Avoid magic numbers

```c
// Register bit manipulation macros
#define SET_BIT(reg, bit)       ((reg) |= (1U << (bit)))
#define CLEAR_BIT(reg, bit)     ((reg) &= ~(1U << (bit)))
#define READ_BIT(reg, bit)      (((reg) >> (bit)) & 1U)
#define TOGGLE_BIT(reg, bit)    ((reg) ^= (1U << (bit)))

// Hardware abstraction
typedef struct {
    volatile uint32_t CR1;      // Control register 1
    volatile uint32_t CR2;      // Control register 2
    volatile uint32_t SR;       // Status register
    volatile uint32_t DR;       // Data register
} uart_registers_t;

#define UART1 ((uart_registers_t *)0x40013800)

void uart_enable_transmitter(void)
{
    SET_BIT(UART1->CR1, UART1_CR1_TE_BIT);
}
```

## File Organization

### Header File Structure
```c
#ifndef UART_DRIVER_H
#define UART_DRIVER_H

/* Includes */
#include <stdint.h>
#include <stdbool.h>
#include "hardware_config.h"

/* Macros and Constants */
#define UART_MAX_INSTANCES      3
#define UART_DEFAULT_TIMEOUT    1000

/* Type Definitions */
typedef enum {
    UART_RESULT_OK,
    UART_RESULT_ERROR,
    UART_RESULT_TIMEOUT
} uart_result_t;

/* Public Function Prototypes */
bool uart_init(const uart_config_t *config);
uart_result_t uart_transmit(const uint8_t *data, uint16_t length);
uart_result_t uart_receive(uint8_t *data, uint16_t length);

#endif /* UART_DRIVER_H */
```

### Source File Structure
```c
/**
 ******************************************************************************
 * @file    uart_driver.c
 * @brief   UART driver implementation
 ******************************************************************************
 */

/* Includes */
#include "uart_driver.h"
#include "system_config.h"

/* Private Constants */
#define UART_TIMEOUT_DEFAULT    1000

/* Private Variables */
static uart_state_t s_uart_states[UART_MAX_INSTANCES];

/* Private Function Prototypes */
static bool uart_configure_pins(uint8_t instance);
static void uart_calculate_baudrate(uint32_t baudrate, uint32_t *brr);

/* Public Functions */
bool uart_init(const uart_config_t *config)
{
    // Implementation
}

/* Private Functions */
static bool uart_configure_pins(uint8_t instance)
{
    // Implementation
}
```

## Error Handling

### Error Codes
- Define comprehensive error enums
- Use consistent error handling patterns
- Log errors when possible

```c
typedef enum {
    SYSTEM_OK = 0,
    SYSTEM_ERROR_INVALID_PARAM,
    SYSTEM_ERROR_TIMEOUT,
    SYSTEM_ERROR_HARDWARE_FAULT,
    SYSTEM_ERROR_OUT_OF_MEMORY,
    SYSTEM_ERROR_UNKNOWN = 0xFF
} system_error_t;
```

### Assert Macros
- Use asserts for development debugging
- Implement custom assert for embedded systems
- Remove asserts in production if needed

```c
#ifdef DEBUG
    #define ASSERT(condition) \
        do { \
            if (!(condition)) { \
                debug_printf("ASSERT FAILED: %s:%d\n", __FILE__, __LINE__); \
                while(1); /* Halt in debug */ \
            } \
        } while(0)
#else
    #define ASSERT(condition) ((void)0)
#endif
```

## Interrupt Handling

### ISR Design
- Keep ISRs short and simple
- Use flags to communicate with main code
- Avoid complex calculations in ISRs

```c
// Global flags for ISR communication
volatile bool g_adc_conversion_complete = false;
volatile bool g_uart_tx_complete = false;

// ADC conversion complete ISR
void ADC1_IRQHandler(void)
{
    if (ADC1->SR & ADC_SR_EOC) {
        ADC1->SR &= ~ADC_SR_EOC;  // Clear flag
        g_adc_conversion_complete = true;
    }
}
```

### Critical Sections
- Minimize time with interrupts disabled
- Use nested interrupt disable/enable carefully

```c
void increment_counter_safe(counter_t *counter)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    
    if (counter->count == UINT32_MAX) {
        counter->overflow = true;
        counter->count = 0;
    } else {
        counter->count++;
    }
    
    __set_PRIMASK(primask);
}
```

## Tools and Automation

### Code Formatting
Use clang-format with the provided `.clang-format` configuration:
```bash
# Format single file
clang-format -i Src/main.c

# Format all source files
find Src Inc -name "*.c" -o -name "*.h" | xargs clang-format -i
```

### Static Analysis
Use clang-tidy with the provided `.clang-tidy` configuration:
```bash
# Analyze single file
clang-tidy Src/main.c -- -I Inc/

# Analyze all source files
find Src -name "*.c" | xargs clang-tidy -- -I Inc/
```

### Pre-commit Hooks
Set up git pre-commit hooks to enforce standards:
```bash
#!/bin/sh
# Check code formatting
if ! clang-format --dry-run --Werror Src/*.c Inc/*.h; then
    echo "Code formatting check failed. Run: make format"
    exit 1
fi

# Run static analysis
if ! clang-tidy Src/*.c -- -I Inc/; then
    echo "Static analysis failed. Fix reported issues."
    exit 1
fi
```

## Build Integration

### Makefile Targets
Add these targets to your Makefile:
```makefile
# Format code
format:
	find Src Inc -name "*.c" -o -name "*.h" | xargs clang-format -i

# Check formatting
format-check:
	find Src Inc -name "*.c" -o -name "*.h" | xargs clang-format --dry-run --Werror

# Static analysis
analyze:
	find Src -name "*.c" | xargs clang-tidy -- -I Inc/

# All checks
check: format-check analyze
	@echo "All coding standard checks passed"
```

## Compliance Checklist

Before committing code, ensure:

- [ ] Code follows naming conventions
- [ ] Functions are properly documented
- [ ] Error handling is implemented
- [ ] Memory usage is optimized
- [ ] Hardware access follows patterns
- [ ] Code passes clang-format check
- [ ] Code passes clang-tidy analysis
- [ ] No warnings in compilation
- [ ] Critical sections are minimal
- [ ] ISRs are kept simple

## Project-Specific Guidelines

### Face Recognition System
- Use descriptive variable names for embeddings and similarity scores
- Document neural network input/output formats
- Use const for model weights and configuration data
- Implement proper error handling for hardware failures
- Use volatile for camera and display buffers
- Document timing requirements for real-time processing

### Performance Considerations
- Profile critical paths regularly
- Use ARM NEON instructions where beneficial
- Minimize cache misses with proper data alignment
- Use DMA for bulk data transfers
- Monitor stack usage in recursive algorithms