# Stage 21 Changes: Request Timeouts

## 1. New Timer Module
**Files:** `src/core/xps_timer.h`, `src/core/xps_timer.c`
- Created `xps_timer_t` struct and associated functions:
    - `xps_timer_create()`: Allocates and initializes a timer.
    - `xps_timer_destroy()`: Frees timer resources and handles removal from core.
    - `xps_timer_update()`: Resets the timer expiry based on current time.

## 2. Core System Updates
**Files:** `src/core/xps_core.h`, `src/core/xps_core.c`
- **Struct Updates:** Added fields to `xps_core_t` to manage timers and time tracking:
    - `vec_void_t timers`: List of active timers.
    - `curr_time_msec`: Current time in milliseconds (cached per loop iteration).
    - `init_time_msec`: Server start time.
- **New Functions:**
    - `xps_core_update_time()`: Updates `curr_time_msec` using `gettimeofday`.
- **Lifecycle:** Updated `create` and `destroy` to handle the new timer list.

## 3. Event Loop Integration
**Files:** `src/core/xps_loop.c`
- **Time Management:**
    - Calling `xps_core_update_time()` at the start of `xps_loop_run` and after `epoll_wait`.
    - Implemented `handle_timers()` to check for expired timers and invoke callbacks.
    - Calculated `timeout` for `epoll_wait` based on the nearest timer expiration.

## 4. Session Timeout Implementation
**Files:** `src/core/xps_session.h`, `src/core/xps_session.c`
- **Struct Updates:** Added `xps_timer_t *timer` to `xps_session_t`.
- **Logic:**
    - Initialized a 60-second timer (`DEFAULT_HTTP_REQ_TIMEOUT_MSEC`) on session creation.
    - Created `session_timer_handler()` to destroy the session if it times out.
    - **Keep-Alive:** Called `xps_timer_update()` in `client_source_handler` (on read) and `client_sink_handler` (on write) to reset the timeout when activity occurs.

## 5. Utilities & Configuration
**Files:** `src/xps.h`, `src/utils/xps_utils.c`
- **New Headers:** Added `<sys/time.h>` and `<time.h>`.
- **Constants:** Added `DEFAULT_HTTP_REQ_TIMEOUT_MSEC` (60000ms).
- **Helpers:** Added `timeval_to_msec()` to convert `struct timeval` to milliseconds.