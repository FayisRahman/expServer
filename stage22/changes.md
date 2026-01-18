# Stage 22: Metrics Implementation

## Overview
This stage introduces a comprehensive metrics module to track server performance, resource usage, and traffic statistics.

## Changes by Component

### `xps_metrics`
- **New Module**: Implemented `xps_metrics` for collecting and managing server statistics.

### `xps.h`
- Added `<sys/resource.h>` for resource usage tracking.
- Defined `_GNU_SOURCE` for advanced feature support.
- Added configuration constants:
  - `DEFAULT_HTTP_REQ_TIMEOUT_MSEC` (60000ms)
  - `METRICS_HOST` ("0.0.0.0")
  - `METRICS_PORT` (8004)

### `xps_core`
- **Structure Update**: Added `xps_metrics_t *metrics` and `xps_timer_t *metrics_update_timer` to `xps_core_s`.
- **Initialization**: 
  - Instantiated `xps_metrics` in `xps_core_create` after the event loop creation.
  - Set up `xps_metrics_update_handler` as a callback.
  - Initialized `metrics_update_timer` to periodically refresh metrics.
- **Cleanup**: Ensured `metrics` module is properly destroyed in `xps_core_destroy`.

### `xps_config`
- **Port Reservation**: Updated `parse_listener` to reject configurations attempting to use the reserved `METRICS_PORT`.
- **Request Type**: Set `lookup->type = REQ_METRICS` early in `xps_config_lookup` to identify metrics requests.

### `xps_session`
- **Timing metrics**: Added `req_create_time_msec` and `res_time` to `xps_session_s` (initialized to -1) to track latencies.
- **Metrics Hooks**: 
  - Integrated `xps_metrics_set` calls in `client_source_handler` and `session_timer_handler`.
  - Updated request processing to handle `REQ_METRICS` by serving metrics JSON data.
  - Added metrics tracking for specific request actions:
    - `M_RES_TIME`: Response time calculation.
    - `M_REQ_FILE_SERVE`: Static file serving.
    - `M_REQ_REDIRECT`: Redirections.
    - `M_REQ_REVERSE_PROXY`: Reverse proxy requests.
    - `M_CONN_TIMEOUT`: Session timeouts.
	- also added some request handling if unknown paths gets requested for (file_serve and metrics and rest of paths it will return appropriate 4xx status code if path is not found)

### `xps_http_req`
- Integrated `xps_metrics_set` to track request lifecycle:
  - `M_REQ_CREATE`: Request creation in `xps_http_req_create`.
  - `M_REQ_DESTROY`: Request destruction in `xps_http_req_destroy`.

### `xps_http_res`
- Integrated `xps_metrics_set` in `xps_http_req_create` to track response status codes:
  - `M_RES_2XX`: Success responses.
  - `M_RES_3XX`: Redirection responses.
  - `M_RES_4XX`: Client error responses.
  - `M_RES_5XX`: Server error responses.

### `xps_connection`
- **Lifecycle Tracking**: 
  - `M_CONN_ACCEPT`: New connection accepted in `xps_connection_create`.
  - `M_CONN_CLOSE`: Connection closed in `xps_connection_destroy`.
- **Traffic Tracking**:
  - `M_TRAFFIC_RECV_BYTES`: Bytes received in `connection_source_handler`.
  - `M_TRAFFIC_SEND_BYTES`: Bytes sent in `connection_sink_handler`.
- **Error Tracking**:
  - `M_CONN_ERROR`: Connection errors during read/write operations.

### `xps_listener`
- **Error Tracking**: Added metrics updates for failures:
  - `M_CONN_ACCEPT_ERROR`: Failures in `accept()`, socket configuration, or connection object creation.

### `main.c`
- Initialized the specific metrics listener configuration.