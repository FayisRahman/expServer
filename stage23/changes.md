# Stage 23: Multiprocess

## Overview
Refactored the server from a single-core to a **multi-core** architecture to maximize performance. This involved parallelising the event loop using threads and aggregating metrics across all cores.

## Multi-core Implementation
### `main.c`
- **Single to Multi-core Conversion**:
    - Renamed `core_create` to `cores_create`.
    - Introduced a global `cores` array and `n_cores` variable to manage multiple core instances.
    - `cores_create` now iterates based on the configured number of workers to create multiple `xps_core_t` instances.
- **Thread Management**:
    - Added `threads_create()` to spawn a `pthread` for each worker core.
    - Added `thread_start()` as the entry point for each thread, running `xps_core_start()`.
    - Added `threads_destroy()` to cancel and join threads on shutdown.

### `xps.h`
- Added `<pthread.h>` include.
- Added global thread management variables: `thread_ids`, `n_threads`.
- Added global core management variables: `cores`, `n_cores`.

## Metrics Aggregation
### `xps_metrics.c`
- **Cumulative Metrics**: Updated `xps_metrics_get_json` to iterate through all `cores` and calculate the cumulative resource usage (RAM, CPU) and request stats (requests, connections, traffic) across all worker threads.
- **Per-Core CPU Tracking**: Added logic to track and display CPU usage percentage for each individual worker core (`workers_cpu_usage_percent` array).