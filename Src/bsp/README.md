# BSP layer

Purpose: board-specific bring-up and low-level enablement that sits directly on the STM32 HAL and Cube BSP drivers.

Responsibilities
- Clocking, power, caches, security (RIF/IAC/MPU), and reset configuration.
- Pin mux, memory-mapped peripherals, external memories, board LEDs/buttons/console UART.
- ISR defaults and common fault handlers.
- Provide small, testable init functions (e.g., `bsp_clock_init`, `bsp_console_init`) that higher layers can call in sequence.

Non-goals
- No application logic, buffering strategies, or FreeRTOS task ownership beyond what is required for basic startup.

