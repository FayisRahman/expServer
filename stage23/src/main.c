#include "network/xps_listener.h"
#include "xps.h"
#include <assert.h>

xps_cliargs_t *cliargs;
int n_listeners;
xps_core_t **cores;
int n_cores = 0;
pthread_t *thread_ids;
int n_threads;
xps_config_t *config;

void sigint_handler(int signum);
int cores_create(xps_config_t *config);
void cores_destroy();
int threads_create(xps_core_t **cores, int n_cores);
void threads_destroy();
void *thread_start(void *arg);

int main(int argc, char *argv[]) {
  signal(SIGINT, sigint_handler);           // for handling ctrl+c
  cliargs = xps_cliargs_create(argc, argv); // get commandline arguments
  if (cliargs == NULL) {
    logger(LOG_ERROR, "main()", "Failed to read from command line");
    exit(EXIT_FAILURE);
  }
  /*create config, create cores, start cores*/
  config = xps_config_create(cliargs->config_path);
  if (config == NULL) {
    logger(LOG_ERROR, "main()", "failed to parse config file");
    exit(EXIT_FAILURE);
  }
  if (cores_create(config) != OK) {
    logger(LOG_ERROR, "main()", "cores_create() failed");
    exit(EXIT_FAILURE);
  }

  if (threads_create(cores, n_cores) != OK) {
    logger(LOG_ERROR, "main()", "threads_create() failed");
    exit(EXIT_FAILURE);
  }

  return EXIT_SUCCESS;
}

void sigint_handler(int signum) {
  logger(LOG_WARNING, "sigint_handler()", "SIGINT received");

  threads_destroy();
  cores_destroy();
  xps_config_destroy(config);
  xps_cliargs_destroy(cliargs);

  exit(EXIT_SUCCESS);
}

int cores_create(xps_config_t *config) {
  assert(config != NULL);

  cores = malloc(sizeof(xps_core_t *) * config->workers);
  if (cores == NULL) {
    logger(LOG_ERROR, "cores_create()", "malloc() failed for cores");
    return E_FAIL;
  }
  n_cores = 0;
  // Create cores
  for (int i = 0; i < config->workers; i++) {
    xps_core_t *core = xps_core_create(config);
    if (core) {
      cores[n_cores] = core;
      n_cores += 1;
    } else {
      logger(LOG_ERROR, "cores_create()", "xps_core_create() failed");
      cores_destroy();
      return E_FAIL;
    }
  }
  /* Create listeners*/
  xps_listener_t *listeners[config->_all_listeners.length + 1];
  n_listeners = 0;

  //creating metrics listener TODO: Stage22
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
      logger(LOG_INFO, "cores_create()", "Server listening on http://%s:%d",
             conf->host, conf->port);
      listeners[n_listeners] = listener;
      n_listeners++;
    } else {
      logger(LOG_ERROR, "cores_create()", "listener creation failed");
    }
  }
  /*Duplicate and add listeners to cores*/
  for (int i = 0; i < n_cores; i++) {
    xps_core_t *curr_core = cores[i];
    for (int j = 0; j < n_listeners; j++) {
      xps_listener_t *listener = listeners[j];
      xps_listener_t *dup_listener = malloc(sizeof(xps_listener_t));
      if (dup_listener == NULL) {
        logger(LOG_ERROR, "cores_create()", "malloc() faield on dup_listener");
        continue;
      }
      /* Initialize dup_listener values*/
      dup_listener->core = curr_core;
      dup_listener->host = listener->host;
      dup_listener->port = listener->port;
      int new_sock_fd = dup(listener->sock_fd);
      if (new_sock_fd == -1) {
        logger(LOG_ERROR, "cores_create()", "dup() failed");
        perror("Error message");
        free(dup_listener);
        continue;
      }
      dup_listener->sock_fd = new_sock_fd;
      /*Attach listener to loop*/
      if (xps_loop_attach(curr_core->loop, dup_listener->sock_fd,
                          EPOLLIN | EPOLLET, dup_listener,
                          xps_listener_connection_handler, NULL, NULL) != OK) {
        logger(LOG_ERROR, "cores_create()", "xps_loop_attach() failed");
        free(dup_listener);
        continue;
      }
      /*Add listener to 'listeners' list of core*/
      vec_push(&(curr_core->listeners), dup_listener);
    }
  }
  for (int i = 0; i < n_listeners; i++) {
    xps_listener_destroy(listeners[i]);
  }

  logger(LOG_DEBUG, "cores_create()", "created cores");

  return OK;
}

void cores_destroy() {
  for (int i = 0; i < n_cores; i++) {
    xps_core_t *curr_core = cores[i];
    xps_core_destroy(curr_core);
    
  }
  free(cores);
  logger(LOG_DEBUG, "cores_destroy()", "destroyed cores");
}

int threads_create(xps_core_t **cores, int n_cores){
  assert(cores != NULL);

  thread_ids = malloc(sizeof(pthread_t) * n_cores);
  if(thread_ids == NULL){
    logger(LOG_ERROR, "theads_create()", "malloc() failed for 'thread_ids'");
    return E_FAIL;
  }

  for(int i = 0; i < n_cores; i++){
    xps_core_t *curr_core = cores[i];
    int err = pthread_create(&(thread_ids[i]),NULL, thread_start, (void*) curr_core);
    if (err != 0) {
      logger(LOG_ERROR, "threads_create()", "pthread_create() failed");
      continue;
    }
    n_threads += 1;
    logger(LOG_DEBUG, "threads_create()", "created thread %d", i);
  }

  for(int i=0;i < n_cores; i++ ){
    if(pthread_join(thread_ids[i],NULL) != 0){
      logger(LOG_ERROR, "threads_create()", "pthread_join() failed");
      exit(EXIT_FAILURE);
    }
  }

  logger(LOG_DEBUG, "threads_create()", "created threads");

  return OK;

}

void threads_destroy() {
  for (int i = 0; i < n_threads; i++) {
    int err = pthread_cancel(thread_ids[i]);
    if (err != 0) {
      logger(LOG_ERROR, "threads_destroy()", "pthread_cancel() failed");
      perror("Error message");
      exit(EXIT_FAILURE);
    }
  }
  free(thread_ids);

  logger(LOG_DEBUG, "threads_destroy()", "destroyed threads");
}

void *thread_start(void *arg) {
  xps_core_t *core = arg;
  xps_core_start(core);
}