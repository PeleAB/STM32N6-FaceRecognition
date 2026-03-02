# Layered architecture plan

Goal: separate hardware bring-up from reusable firmware building blocks, services, and application orchestration.

Stack
- APP (`Src/app`, `Inc/app`): orchestrates pipeline, owns tasks/priorities, modes, and UI/debug toggles; calls services only.
- SVC (`Src/svc`, `Inc/svc`): reusable services (camera, NN runtime, postprocess/overlay, encoder, UVC sink, stats, buffer queue/event bus); may own FreeRTOS objects.
- FAL (`Src/fal`, `Inc/fal`): firmware abstraction over HAL/BSP (typed handles, cache/DMA helpers, DCMIPP/VENC wrappers) with error normalization and locking.
- HAL + BSP (`STM32Cube_FW_N6`, `Lib`, `Src/bsp`, `Inc/bsp`): vendor HAL/LL and board bring-up (clock/power/cache/security/pinmux, console, LEDs/buttons, external memories).

Directory layout (created, content to be migrated incrementally)
- `Src/bsp` / `Inc/bsp`: board init modules, RTOS hooks, board-level fault handlers.
- `Src/fal` / `Inc/fal`: HAL wrappers for camera/DCMIPP, encoder (H264/JPEG), DMA2D/cache, XSPI, console/UART helpers.
- `Src/svc` / `Inc/svc`: camera_service, nn_service, postprocess/overlay_service, encoder_service, uvc_sink, stats_service, buffer queue/event dispatch.
- `Src/app` / `Inc/app`: `app_run`, pipeline orchestrator, mode selection, debug UI toggles.

Current-to-target mapping (proposed)
- `Src/main.c`: move clock/security/cache/console setup into `bsp/platform_*.c`; keep RTOS entry/orchestration in `app/main_thread.c`.
- `Src/fal/sysobj_camera.c`: hardware config wrapper over HAL (formerly `Src/app_cam.c`), sensor profile handling will move to `svc/camera_service.c`.
- `Src/app_pipeline.c`: move buffer queue to `svc/bqueue.c`, pipeline coordination/tasks to `app/pipeline_orchestrator.c`, and ISR callbacks to an event path that returns buffers via the service.
- `Src/app_display.c`: split into `svc/overlay_renderer.c`, `svc/encoder_service.c`, `svc/uvc_sink.c`; DMA2D locking moves to `fal/dma2d_hal.c`.
- `Src/fal/sysobj_encoder.c`: HAL wrapper for encoder (was `Src/app_enc.c`); a service should wrap it with policy (GOP, bitrate) and keyframe control.
- `Src/app_stats.c`: `svc/stats_service.c` with optional sinks (console/overlay).
- `Src/svc/buffer_queue.c`: shared utility (formerly `Src/app_bqueue.c`).
- `Src/utils.c`: NN runtime helper → `svc/nn_service.c` or `fal/nn_hal.c` (depending on HAL coupling).
- `Src/app_fuseprogramming.c`, `Src/freertos_bsp.c`: board-specific init/hooks → `bsp`.

Migration steps
1) Keep existing files/builds working; introduce new modules alongside current ones and update includes incrementally.
2) Move low-level init from `main.c` into `bsp` and expose a `platform_init()` used by the app entry.
3) Extract FAL wrappers (camera/encoder/DMA2D/cache) without changing behavior; then layer services on top.
4) Shift pipeline orchestration into `app` once services exist; remove direct HAL calls from app/service code paths.



