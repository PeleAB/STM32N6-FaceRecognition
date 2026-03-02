# FAL layer (Firmware Abstraction Layer)

Purpose: wrap HAL/BSP primitives into cohesive, safe building blocks with clear lifecycles and data models.

Responsibilities
- Typed handles and init/start/stop APIs for camera, encoder, display DMA, cache ops, timers, GPIO/UART helpers.
- Resource ownership and locking (DMA2D, XSPI, cache clean/invalidate) exposed via simple primitives.
- Normalize error codes and parameter checking; hide HAL structs from upper layers.

Non-goals
- No application policy (e.g., when to encode or draw), no threading policy; those live in Services or App.
