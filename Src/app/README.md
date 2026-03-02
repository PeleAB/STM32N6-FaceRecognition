# APP layer

Purpose: application orchestration that wires services together into user-facing behavior.

Responsibilities
- Entry points (`main_thread`, `app_run`), pipeline orchestration, task priority policy, mode selection, UI/debug toggles.
- Configure services with profiles (sensor/venc/fps), react to events, and own overall lifecycle.

Non-goals
- Direct HAL/BSP access or hardware policy; rely on services and FAL.
- Service-level feature logic that should live below.

