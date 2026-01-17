#include "xps_core.h"

xps_core_t *xps_core_create(xps_config_t *config) {

  xps_core_t *core =
      malloc(sizeof(xps_core_t)); /* allocate memory using malloc() */
  /* handle error where core == NULL */
  if (core == NULL) {
    logger(LOG_ERROR, "xps_core_create()", "malloc failed for 'core'");
    return NULL;
  }

  xps_loop_t *loop = xps_loop_create(core); /* create xps_loop instance */
  /* handle error where loop == NULL */
  if (loop == NULL) {
    logger(LOG_ERROR, "xps_core_create()",
           "xps_loop_create() failed to create loop");
    free(core);
    return NULL;
  }

  // Init values
  core->loop = loop;
  core->config = config;
  vec_init(&(core->listeners));
  vec_init(&(core->connections));
  vec_init(&(core->pipes));
  vec_init(&(core->sessions));
  vec_init(&(core->timers));
  core->n_null_listeners = 0;
  core->n_null_connections = 0;
  core->n_null_pipes = 0;
  core->n_null_sessions = 0;
  core->n_null_timers = 0;
  core->init_time_msec = 0;
  core->curr_time_msec = 0;
  logger(LOG_DEBUG, "xps_core_create()", "created core");

  return core;
}

void xps_core_destroy(xps_core_t *core) {
  assert(core != NULL);

  // Destroy connections
  for (int i = 0; i < core->connections.length; i++) {
    xps_connection_t *connection = core->connections.data[i];
    if (connection != NULL)
      xps_connection_destroy(connection);
  }
  vec_deinit(&(core->connections));

  /* destory all the listeners and de-initialize core->listeners */
  for (int i = 0; i < core->listeners.length; i++) {
    xps_listener_t *listener = core->listeners.data[i];
    if (listener != NULL)
      xps_listener_destroy(listener);
  }
  vec_deinit(&(core->listeners));

  /* destory all the listeners and de-initialize core->listeners */
  for (int i = 0; i < core->pipes.length; i++) {
    xps_pipe_t *pipe = core->pipes.data[i];
    if (pipe != NULL) {
      if (pipe->source) {
        xps_pipe_source_destroy(pipe->source);
      }
      if (pipe->sink) {
        xps_pipe_sink_destroy(pipe->sink);
      }
      xps_pipe_destroy(pipe);
    }
  }
  vec_deinit(&(core->pipes));

  /* destory all the sessions and de-initialize core->sessions */
  for (int i = 0; i < core->sessions.length; i++) {
    xps_session_t *session = core->sessions.data[i];
    if (session != NULL) {
      xps_session_destroy(session);
    }
  }
  vec_deinit(&(core->sessions));

  /* destory all the timers and de-initialize core->timers */
  for (int i = 0; i < core->timers.length; i++) {
    xps_timer_t *timer = core->timers.data[i];
    if (timer != NULL)
      xps_timer_destroy(timer);
  }
  vec_deinit(&(core->timers));

  /* destory loop attached to core */
  xps_loop_destroy(core->loop);

  /* free core instance */
  free(core);

  logger(LOG_DEBUG, "xps_core_destroy()", "destroyed core");
}

void xps_core_start(xps_core_t *core) {

  /* validate params */
  assert(core != NULL);

  logger(LOG_DEBUG, "xps_start()", "starting core");

  // /* create listeners from port 8001 to 8004 */
  // for (int port = 8001; port <= 8004; port++) {
  //   xps_listener_create(core, LOCALHOST, port);
  //   logger(LOG_INFO, "xps_core_start()", "server listening on http://%s:%d",
  //          LOCALHOST, port);
  // }

  /* run loop instance using xps_loop_run() */
  xps_loop_run(core->loop);
}

//TODO: stage21
void xps_core_update_time(xps_core_t *core){
  assert(core != NULL);

  struct timeval time;
  if (gettimeofday(&time, NULL) < 0) {
    logger(LOG_ERROR, "xps_core_update_time()", "gettimeofday() failed");
    perror("Error message");
    return;
  }

  // Set curr_time_msec
  core->curr_time_msec = timeval_to_msec(time);

  // Set 'init_time_msec'
  if (core->init_time_msec == 0)
    core->init_time_msec = core->curr_time_msec;

}