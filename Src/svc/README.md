# SVC layer (Services)

Purpose: reusable services built on FAL to deliver higher-level features for the application.

Responsibilities
- Camera service (sensor detect, profile selection, pipe routing), NN runtime service, postprocess/overlay service, encoder service, UVC sink service.
- Shared utilities like buffer queues, event dispatch, statistics/CPU-load tracking.
- Own FreeRTOS tasks/queues/timers when needed and expose narrow, message-oriented APIs to the App.
- NN service (`nn_service`) registers/selects multiple AToN networks and exposes their user I/O sizes so the app can swap models without changing low-level calls.

Non-goals
- Board bring-up and raw HAL interaction (BSP/FAL handle those).
- Application-specific orchestration (that stays in App).

