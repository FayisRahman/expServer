#include "../xps.h"

xps_timer_t *xps_timer_create(xps_core_t *core, u_long duration_msec, void *ptr, xps_handler_t cb) {
  assert(core != NULL);
  assert(ptr != NULL);
  assert(cb != NULL);

  //  Allocate memory for timer using malloc()
  xps_timer_t *timer = malloc(sizeof(xps_timer_t));

  //  Check if malloc failed, log error and return NULL
  if(timer == NULL){
    logger(LOG_ERROR, "xps_timer_create()", "malloc() failed");
    return NULL;
  }

  //  initialize the values
  timer->core = core;
  timer->expiry_time_msec = core->curr_time_msec + duration_msec;
  timer->ptr = ptr;
  timer->cb = cb;
  vec_push(&(core->timers), timer);
  

  logger(LOG_DEBUG, "xps_timer_create()", "created timer");
  return timer;
}

void xps_timer_destroy(xps_timer_t *timer) {
  assert(timer != NULL);

  //  Find timer in core->timers and set to NULL
  // HINT: Loop through timer->core->timers.data[i]
  //       If match found, set data[i] = NULL
  //       Increment timer->core->n_null_timers
  xps_core_t *core = timer->core;
  for(int i=0; i<core->timers.length;i++){
    if(core->timers.data[i] == timer){
      core->timers.data[i] = NULL;
      core->n_null_timers++;
      break;
    }
  }

  free(timer);
  

  logger(LOG_DEBUG, "xps_timer_destroy()", "destroyed timer");
}

void xps_timer_update(xps_timer_t *timer, u_long duration_msec) {
  assert(timer != NULL);
  assert(duration_msec >= 0);
  
  //  Reset expiry time to current time + duration
  // HINT: timer->expiry_time_msec = timer->core->curr_time_msec + duration_msec
  timer->expiry_time_msec = timer->core->curr_time_msec + duration_msec;
  
}