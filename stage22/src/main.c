#include "network/xps_listener.h"
#include "xps.h"

xps_cliargs_t *cliargs;
int n_listeners;
xps_core_t *core;
// int n_cores = 0;

void sigint_handler(int signum);
int core_create(xps_config_t *config);
void core_destroy();

int main(int argc, char *argv[]) {
  signal(SIGINT, sigint_handler);           // for handling ctrl+c
  cliargs = xps_cliargs_create(argc, argv); // get commandline arguments
  if (cliargs == NULL) {
    logger(LOG_ERROR, "main()", "Failed to read from command line");
    exit(EXIT_FAILURE);
  }
  /*create config, create cores, start cores*/
  xps_config_t *config = xps_config_create(cliargs->config_path);
  if (config == NULL) {
    logger(LOG_ERROR, "main()", "failed to parse config file");
    exit(EXIT_FAILURE);
  }
  if (core_create(config) != OK) {
    logger(LOG_ERROR, "main()", "cores_create() failed");
    exit(EXIT_FAILURE);
  }

  xps_core_start(core);

  return EXIT_SUCCESS;
}

void sigint_handler(int signum) {
  logger(LOG_WARNING, "sigint_handler()", "SIGINT received");

  core_destroy();

  exit(EXIT_SUCCESS);
}

int core_create(xps_config_t *config) {
  assert(config != NULL);

  // Create cores
  core = xps_core_create(config);
  if (core == NULL) {
    logger(LOG_ERROR, "cores_create()", "xps_core_create() failed");
    core_destroy();
    return E_FAIL;
  }

  /* Create listeners*/
  xps_listener_t *listeners[config->_all_listeners.length + 1];
  n_listeners = 0;

  // creating metrics listener TODO: Stage22
  xps_listener_t *metrics_listener = xps_listener_create(METRICS_HOST, METRICS_PORT);
  if (metrics_listener) {
    logger(LOG_INFO, "cores_create()", "Metrics server listening on http://%s:%d", METRICS_HOST,
           METRICS_PORT);
    listeners[n_listeners] = metrics_listener;
    n_listeners += 1;
  }

  for (int i = 0; i < config->_all_listeners.length; i++) {
    xps_config_listener_t *conf = config->_all_listeners.data[i];
    xps_listener_t *listener = xps_listener_create(conf->host, conf->port);
    if (listener) {
      logger(LOG_INFO, "cores_create()", "Server listening on http://%s:%d", conf->host,
             conf->port);
      listeners[n_listeners] = listener;
      n_listeners++;
    } else {
      logger(LOG_ERROR, "cores_create()", "listener creation failed");
    }
  }

  /*attach listeners to core*/
  for (int j = 0; j < n_listeners; j++) {
    xps_listener_t *listener = listeners[j];
    /*Attach listener to loop*/
    if (xps_loop_attach(core->loop, listener->sock_fd, EPOLLIN | EPOLLET, listener,
                        xps_listener_connection_handler, NULL, NULL) != OK) {
      logger(LOG_ERROR, "cores_create()", "xps_loop_attach() failed");
      free(listener);
      continue;
    }
    /*Add listener to 'listeners' list of core*/
    listener->core = core;
    vec_push(&(core->listeners), listener);
  }

  logger(LOG_DEBUG, "core_create()", "created core");

  return OK;
}

void core_destroy() {
  xps_core_destroy(core);
  logger(LOG_DEBUG, "cores_destroy()", "destroyed cores");
}
