#include "xps_session.h"

void client_source_handler(void *ptr);
void client_source_close_handler(void *ptr);
void client_sink_handler(void *ptr);
void client_sink_close_handler(void *ptr);
void upstream_source_handler(void *ptr);
void upstream_source_close_handler(void *ptr);
void upstream_sink_handler(void *ptr);
void upstream_sink_close_handler(void *ptr);
void file_sink_handler(void *ptr);
void file_sink_close_handler(void *ptr);
void upstream_error_res(xps_session_t *session);
void set_to_client_buff(xps_session_t *session, xps_buffer_t *buff);
void set_from_client_buff(xps_session_t *session, xps_buffer_t *buff);
void session_check_destroy(xps_session_t *session);

// custom function
void session_destroy_pipes(xps_session_t *session);

void session_destroy_pipes(xps_session_t *session) {

  assert(session != NULL);

  if (session->client_source)
    xps_pipe_source_destroy(session->client_source);
  if (session->client_sink)
    xps_pipe_sink_destroy(session->client_sink);
  if (session->upstream_source)
    xps_pipe_source_destroy(session->upstream_source);
  if (session->upstream_sink)
    xps_pipe_sink_destroy(session->upstream_sink);
  if (session->file_sink)
    xps_pipe_sink_destroy(session->file_sink);
}

xps_session_t *xps_session_create(xps_core_t *core, xps_connection_t *client) {
  /* validate parameters */
  assert(core != NULL);
  assert(client != NULL);

  // Alloc memory for session instance
  xps_session_t *session = malloc(sizeof(xps_session_t));
  if (session == NULL) {
    logger(LOG_ERROR, "xps_session_create()", "malloc() failed for 'session'");
    return NULL;
  }

  session->client_source = xps_pipe_source_create(
      session, client_source_handler, client_source_close_handler);
  session->client_sink = xps_pipe_sink_create(session, client_sink_handler,
                                              client_sink_close_handler);
  session->upstream_source = xps_pipe_source_create(
      session, upstream_source_handler, upstream_source_close_handler);
  session->upstream_sink = xps_pipe_sink_create(session, upstream_sink_handler,
                                                upstream_sink_close_handler);
  session->file_sink =
      xps_pipe_sink_create(session, file_sink_handler, file_sink_close_handler);

  if (!(session->client_source && session->client_sink &&
        session->upstream_source && session->upstream_sink &&
        session->file_sink)) {
    logger(LOG_ERROR, "xps_session_create()",
           "failed to create some sources/sinks");

    session_destroy_pipes(session);

    free(session);
    return NULL;
  }

  // Init values
  session->core = core;
  session->client = client;
  session->upstream = NULL;
  session->upstream_connected = false;
  session->upstream_error_res_set = false;
  session->upstream_write_bytes = 0;
  session->file = NULL;
  session->to_client_buff = NULL;
  session->from_client_buff = NULL;
  session->client_sink->ready = true;
  session->upstream_sink->ready = true;
  session->file_sink->ready = true;

  // Add to 'sessions' list of core
  vec_push(&(core->sessions), session);

  // Attach client
  if (xps_pipe_create(core, DEFAULT_PIPE_BUFF_THRESH, client->source,
                      session->client_sink) == NULL ||
      xps_pipe_create(core, DEFAULT_PIPE_BUFF_THRESH, session->client_source,
                      client->sink) == NULL) {
    logger(LOG_ERROR, "xps_session_create()", "failed to create client pipes");

    xps_session_destroy(session);

    return NULL;
  }

  logger(LOG_DEBUG, "xps_session_create()", "created session");

  if (client->listener->port == 8001) {
    xps_connection_t *upstream =
        xps_upstream_create(core, client->listener->host, 3000);
    if (upstream == NULL) {
      logger(LOG_ERROR, "xps_session_create()", "xps_upstream_create() failed");
      perror("Error message");
      xps_session_destroy(session);
      return NULL;
    }
    session->upstream = upstream;
    xps_pipe_create(core, DEFAULT_PIPE_BUFF_THRESH, session->upstream_source,
                    upstream->sink);
    xps_pipe_create(core, DEFAULT_PIPE_BUFF_THRESH, upstream->source,
                    session->upstream_sink);
  } else if (client->listener->port == 8002) {
    int error;
    xps_file_t *file = xps_file_create(core, "../public/sample.txt", &error);
    if (file == NULL) {
      logger(LOG_ERROR, "xps_session_create()", "xps_file_create() failed");
      perror("Error message");
      xps_session_destroy(session);
      return NULL;
    }
    session->file = file;
    xps_pipe_create(core, DEFAULT_PIPE_BUFF_THRESH, file->source,
                    session->file_sink);
  }

  return session;
}

void client_source_handler(void *ptr) {
  /* validate parameters */
  assert(ptr != NULL);

  xps_pipe_source_t *source = ptr;
  xps_session_t *session = source->ptr;

  // write to session->to_client_buff
  if (xps_pipe_source_write(source, session->to_client_buff) != OK) {
    logger(LOG_ERROR, "client_source_handler()",
           "xps_pipe_source_write() failed");
    return;
  }
  xps_buffer_destroy(session->to_client_buff);

  set_to_client_buff(session, NULL);
  session_check_destroy(session);
}

void client_source_close_handler(void *ptr) {
  assert(ptr != NULL);

  xps_pipe_source_t *source = ptr;
  xps_session_t *session = source->ptr;

  session_check_destroy(session);
}

void client_sink_handler(void *ptr) {
  assert(ptr != NULL);

  xps_pipe_sink_t *sink = ptr;
  xps_session_t *session = sink->ptr;

  xps_buffer_t *buff = xps_pipe_sink_read(sink, sink->pipe->buff_list->len);
  if (buff == NULL) {
    logger(LOG_ERROR, "client_sink_handler()", "xps_pipe_sink_read() failed");
    return;
  }

  set_from_client_buff(session, buff);
  xps_pipe_sink_clear(sink, buff->len);
}

void client_sink_close_handler(void *ptr) {

  assert(ptr != NULL);
  xps_pipe_sink_t *sink = ptr;
  xps_session_t *session = sink->ptr;

  session_check_destroy(session);
}

void upstream_source_handler(void *ptr) {

  assert(ptr != NULL);

  xps_pipe_source_t *source = ptr;
  xps_session_t *session = source->ptr;

  if (xps_pipe_source_write(source, session->from_client_buff) != OK) {
    logger(LOG_ERROR, "upstream_source_handler()",
           "xps_pipe_source_write() failed");
    return;
  }

  // Checking if upstream is connected
  if (session->upstream_connected == false) {
    session->upstream_write_bytes += session->from_client_buff->len;
    if (session->upstream_write_bytes >
        session->upstream_source->pipe->buff_list->len)
      session->upstream_connected = true;
  }

  xps_buffer_destroy(session->from_client_buff);

  set_from_client_buff(session, NULL);
  session_check_destroy(session);
}

void upstream_source_close_handler(void *ptr) {

  assert(ptr != NULL);

  xps_pipe_source_t *source = ptr;
  xps_session_t *session = source->ptr;

  if (!session->upstream_connected && !session->upstream_error_res_set) {
    upstream_error_res(session);
  }

  session_check_destroy(session);
}

void upstream_sink_handler(void *ptr) {

  assert(ptr != NULL);

  xps_pipe_sink_t *sink = ptr;
  xps_session_t *session = sink->ptr;

  session->upstream_connected = true;

  xps_buffer_t *buff = xps_pipe_sink_read(sink, sink->pipe->buff_list->len);
  if (buff == NULL) {
    logger(LOG_ERROR, "upstream_sink_handler()", "xps_pipe_sink_read() failed");
    return;
  }

  set_to_client_buff(session, buff);
  xps_pipe_sink_clear(sink, buff->len);
}

void upstream_sink_close_handler(void *ptr) {

  assert(ptr != NULL);
  xps_pipe_sink_t *sink = ptr;
  xps_session_t *session = sink->ptr;

  if (!session->upstream_connected && !session->upstream_error_res_set) {
    upstream_error_res(session);
  }

  session_check_destroy(session);
}

void upstream_error_res(xps_session_t *session) {
  assert(session != NULL);

  session->upstream_error_res_set = true;
}

void file_sink_handler(void *ptr) {

  assert(ptr != NULL);
  xps_pipe_sink_t *sink = ptr;
  xps_session_t *session = sink->ptr;

  xps_buffer_t *buff = xps_pipe_sink_read(sink, sink->pipe->buff_list->len);
  if (buff == NULL) {
    logger(LOG_ERROR, "file_sink_handler()", "xps_pipe_sink_read() failed");
    return;
  }

  set_to_client_buff(session, buff);
  xps_pipe_sink_clear(sink, buff->len);
}

void file_sink_close_handler(void *ptr) {

  assert(ptr != NULL);

  xps_pipe_sink_t *sink = ptr;
  xps_session_t *session = sink->ptr;

  session_check_destroy(session);
}

void set_to_client_buff(xps_session_t *session, xps_buffer_t *buff) {

  assert(session != NULL);

  session->to_client_buff = buff;

  if (buff == NULL) {
    session->client_source->ready = false;
    session->upstream_sink->ready = true;
    session->file_sink->ready = true;
  } else {
    session->client_source->ready = true;
    session->upstream_sink->ready = false;
    session->file_sink->ready = false;
  }
}

void set_from_client_buff(xps_session_t *session, xps_buffer_t *buff) {

  assert(session != NULL);

  session->from_client_buff = buff;

  if (buff == NULL) {
    session->client_sink->ready = true;
    session->upstream_source->ready = false;
  } else {
    session->client_sink->ready = false;
    session->upstream_source->ready = true;
  }
}

void session_check_destroy(xps_session_t *session) {

  assert(session != NULL);

  bool c2u_flow = session->upstream_source->active &&
                  (session->client_sink->active || session->from_client_buff);

  bool u2c_flow = session->client_source->active &&
                  (session->upstream_sink->active || session->to_client_buff);

  bool f2c_flow = session->client_source->active &&
                  (session->file_sink->active || session->to_client_buff);

  bool flowing = c2u_flow || u2c_flow || f2c_flow;

  if (!flowing)
    xps_session_destroy(session);
}

void xps_session_destroy(xps_session_t *session) {

  assert(session != NULL);

  /* destroy client_source, client_sink, upstream_source, upstream_sink and
   * file_sink attached to session */
  session_destroy_pipes(session);

  if (session->to_client_buff != NULL)
    xps_buffer_destroy(session->to_client_buff);
  if (session->from_client_buff != NULL)
    xps_buffer_destroy(session->from_client_buff);

  // Set NULL in core's list of sessions
  for (int i = 0; i < session->core->sessions.length; i++) {
    xps_session_t *temp = session->core->sessions.data[i];
    if (temp == session) {
      session->core->sessions.data[i] = NULL;
      session->core->n_null_sessions++;
    }
  }

  free(session);

  logger(LOG_DEBUG, "xps_session_destroy()", "destroyed session");
}