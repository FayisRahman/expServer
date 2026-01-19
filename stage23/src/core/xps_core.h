#ifndef XPS_CORE_H
#define XPS_CORE_H

#include "../xps.h"

struct xps_core_s {
  xps_loop_t *loop;
  xps_config_t *config;

  vec_void_t listeners;
  vec_void_t connections;
  vec_void_t pipes;
  vec_void_t sessions;
  vec_void_t timers;
  u_int n_null_listeners;
  u_int n_null_connections;
  u_int n_null_pipes;
  u_int n_null_sessions;
  u_int n_null_timers;

  xps_metrics_t *metrics;

  u_long curr_time_msec;
  u_long init_time_msec;

  xps_timer_t *metrics_update_timer;
};

xps_core_t *xps_core_create(xps_config_t *config);
void xps_core_destroy(xps_core_t *core);
void xps_core_start(xps_core_t *core);
void xps_core_update_time(xps_core_t *core);

#endif