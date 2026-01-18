# Stage 22 Metrics

## xps_metrics
- added metrics module

## xps.h
- added <sys/resource.h> library
- added #define _GNU_SOURCE
- added #define DEFAULT_HTTP_REQ_TIMEOUT_MSEC 60000 // 60sec
- added #define METRICS_HOST "0.0.0.0"
- added #define METRICS_PORT 8004

## xps_core
- added xps_metrics_t *metrics in the xps_core_s
- added xps_timer_t *metrics_update_timer in the xps_core_s
- in xps_core_create we instantiate xps_metrics_create after allocating `loop` with callback as xps_metrics_update_handler
- create a metrics_update_timer after creating metrics and updating the fields in core(ie, core->metric, core->metris_update_timer)
- `metrics` is destroyed in xps_core_destroy appropriately

## xps_config
- in xps_config_lookup setting lookup->type = REQ_METRICS in teh begining and returning it
- in parse_listener seeing if teh listener is METRICS_PORT if yes then raise error saying cant use metrics port as it is already occupied for the metrics usage

## xps_session
- added u_long req_create_time_msec;
        long res_time; in xps_Session_s and initialised its values to -1 in xps_session_create()
- added xps_metrics set in client_source_handler(), session_timer_handler()
- added lookup->type == REQ_METRICS in session process request to sent the metrics data to the client
- xps_Metrics_set for REQ_FILE_SERVE,REQ_REDIRECT,REQ_REVERSE_PROXY

## xps_http_req
- xps_metrics_set in xps_http_req_create() and xps_http_req_destroy()

## xps_http_res
- xps_metrics_set in xps_http_res_create for status_code

## xps_connection
- xps_metrics_set in xps_connection_create(),xps_connection_destroy(),[connection_source_handler() for read_n > 0, on read_n < 0, on xps_pipe_write() fail], [connection_sink_handler() for write_n > 0 , write_n < 0]

## xps_listener
- xps_metrics_set in listener_conection_handler() on accept() failed, on make_socket_non_blocking() fail, on xps_connection_create() failed
 
## main.c
- created a metrics_listener