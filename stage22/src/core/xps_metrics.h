#ifndef XPS_METRICS_H
#define XPS_METRICS_H

#include "../xps.h"

struct xps_metrics_s {
  xps_core_t *core;
  u_long _res_n;
  u_long _res_time_sum;
  u_long _last_worker_cpu_update_uptime_msec;
  u_long _last_worker_cpu_time_msec;

  const char *server_name;
  u_int pid;
  u_int workers;
  u_long uptime_msec;

  float sys_cpu_usage_percent;
  u_long sys_ram_usage_bytes;
  u_long sys_ram_total_bytes;

  float worker_cpu_usage_percent;
  u_long worker_ram_usage_bytes;

  u_long conn_current;
  u_long conn_accepted;
  u_long conn_error;
  u_long conn_timeout;
  u_long conn_accept_error;

  u_long req_current;
  u_long req_total;
  u_long req_file_serve;
  u_long req_reverse_proxy;
  u_long req_redirect;

  u_long res_avg_res_time_msec;
  u_long res_peak_res_time_msec;
  u_long res_code_2xx;
  u_long res_code_3xx;
  u_long res_code_4xx;
  u_long res_code_5xx;

  size_t traffic_total_send_bytes;
  size_t traffic_total_recv_bytes;
};

typedef enum xps_metric_type_e {
  M_CONN_ACCEPT,
  M_CONN_CLOSE,
  M_CONN_ACCEPT_ERROR,
  M_CONN_ERROR,
  M_CONN_TIMEOUT,
  M_REQ_CREATE,
  M_REQ_DESTROY,
  M_REQ_FILE_SERVE,
  M_REQ_REVERSE_PROXY,
  M_REQ_REDIRECT,
  M_RES_TIME,
  M_RES_2XX,
  M_RES_3XX,
  M_RES_4XX,
  M_RES_5XX,
  M_TRAFFIC_SEND_BYTES,
  M_TRAFFIC_RECV_BYTES
} xps_metric_type_t;

xps_metrics_t *xps_metrics_create(xps_core_t *core, xps_config_t *config);
void xps_metrics_destroy(xps_metrics_t *metrics);

xps_buffer_t *xps_metrics_get_json(xps_metrics_t *metrics);
void xps_metrics_set(xps_core_t *core, xps_metric_type_t type, long val);
void xps_metrics_update_handler(void *ptr);

#endif