#ifndef XPS_TIMER_H
#define XPS_TIMER_H

#include "../xps.h"

struct xps_timer_s {
  xps_core_t *core;
  u_long expiry_time_msec;
  void *ptr;
  xps_handler_t cb;
   u_long curr_time_msec;
  u_long init_time_msec;
};

xps_timer_t *xps_timer_create(xps_core_t *core, u_long duration_msec, void *ptr, xps_handler_t cb);
void xps_timer_destroy(xps_timer_t *timer);
void xps_timer_update(xps_timer_t *timer, u_long duration_msec);

#endif