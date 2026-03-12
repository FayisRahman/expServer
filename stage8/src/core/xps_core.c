#include "xps_core.h"


xps_core_t *xps_core_create() {

  xps_core_t *core = malloc(sizeof(xps_core_t));/* allocate memory using malloc() */

  if(core == NULL){
    logger(LOG_ERROR, "xps_core_create():", "core malloc failed");
    return NULL;
  }

  xps_loop_t *loop = xps_loop_create(core);/* create xps_loop instance */

  if(loop == NULL){
    logger(LOG_ERROR, "xps_core_create():", "xps_loop_create() failed");
    return NULL;
  }

  // Init values
  core->loop = loop;
  vec_init(&(core->listeners));
  vec_init(&(core->connections));
  core->n_null_listeners = 0;
  core->n_null_connections = 0;

  logger(LOG_DEBUG, "xps_core_create()", "created core");

  return core;
}

void xps_core_destroy(xps_core_t *core) {
  assert(core != NULL);

  // Destroy connections
  for (int i = 0; i < core->connections.length; i++) {
    xps_connection_t *connection = core->connections.data[i];
    if (connection != NULL)
      xps_connection_destroy(connection); // modification of xps_connection_destroy() will be look at later
  }
  vec_deinit(&(core->connections));

 for (int i = 0; i < core->listeners.length; i++) {
    xps_listener_t *listener = core->listeners.data[i];
    if (listener != NULL)
      xps_listener_destroy(listener); // modification of xps_listener_destroy() will be look at later
  }
  vec_deinit(&(core->listeners));

  xps_loop_destroy(core->loop);

  /* free core instance */
  free(core);

  logger(LOG_DEBUG, "xps_core_destroy()", "destroyed core");
}

void xps_core_start(xps_core_t *core) {

  assert(core != NULL);

  logger(LOG_DEBUG, "xps_core_start()", "starting core");

  /* create listeners from port 8001 to 8004 */
  for (int port = 8001; port <= 8004; port++) {
    /* create listener instance using xps_listener_create() */
    xps_listener_create(core,"127.0.0.1",port);
    logger(LOG_INFO, "xps_core_start()", "Server listening on port %u", port);
	}

  /* run loop instance using xps_loop_run() */

	xps_loop_run(core->loop);

}