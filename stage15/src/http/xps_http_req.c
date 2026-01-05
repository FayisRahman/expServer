#include "xps_http_req.h"

int http_process_request_line(xps_http_req_t *http_req, xps_buffer_t *buff);
xps_keyval_t *http_header_create(xps_http_req_t *http_req);

xps_keyval_t *http_header_create(xps_http_req_t *http_req) {
  char *key =
      str_from_ptrs(http_req->header_key_start, http_req->header_key_end);
  char *value =
      str_from_ptrs(http_req->header_val_start, http_req->header_val_end);

  if (key == NULL || value == NULL) {
    logger(LOG_ERROR, "http_header_create()",
           "str_from_ptrs() failed for %s%s%s", !key ? "key" : "",
           !key && !value ? " and " : "", !value ? "value" : "");
    if(key)
      free(key);
    if(value)
      free(value);
    return NULL;
  }

  xps_keyval_t *header = malloc(sizeof(xps_keyval_t));

  if (header == NULL) {
    logger(LOG_ERROR, "http_header_create()", "malloc() failed");
    return NULL;
  }

  header->key = key;
  header->val = value;

  return header;
}

int http_process_request_line(xps_http_req_t *http_req, xps_buffer_t *buff) {

  assert(http_req != NULL);
  assert(buff != NULL);

  int error = xps_http_parse_request_line(http_req, buff);
  if (error != OK)
    return error;
  http_req->request_line =
      str_from_ptrs(http_req->request_line_start, http_req->request_line_end);
  /*similarly assign method, uri, schema, host, path, pathname*/
  http_req->method =
      str_from_ptrs(http_req->method_start, http_req->method_end);
  http_req->uri = str_from_ptrs(http_req->uri_start, http_req->uri_end);
  http_req->schema =
      str_from_ptrs(http_req->schema_start, http_req->schema_end);
  http_req->host = str_from_ptrs(http_req->host_start, http_req->host_end);
  http_req->path = str_from_ptrs(http_req->path_start, http_req->path_end);
  http_req->pathname =
      str_from_ptrs(http_req->pathname_start, http_req->pathname_end);
  http_req->http_version =
      str_from_ptrs(http_req->http_major, http_req->http_minor);

  http_req->port = -1;
  /*find port_str from port start and end pointers*/
  char *port_str = str_from_ptrs(http_req->port_start, http_req->port_end);
  /*if port_str is null assign default port number 80 for http and 443 for https
  if not null assign atoi(port_str)*/
  if (port_str == NULL) {
    if (http_req->schema != NULL) {
      if (strcmp(http_req->schema, "http") == 0)
        http_req->port = 80; // Default http port
      else if (strcmp(http_req->schema, "https") == 0)
        http_req->port = 443; // Default https port
    }
  } else {
    http_req->port = atoi(port_str);
    free(port_str);
  }

  printf("Request Line: %s\n", http_req->request_line);
  printf("Method: %s\n", http_req->method);
  printf("URI: %s\n", http_req->uri);
  printf("Schema: %s\n", http_req->schema);
  printf("Host: %s\n", http_req->host);
  printf("Path: %s\n", http_req->path);
  printf("Pathname: %s\n", http_req->pathname);
  printf("Port: %d\n", http_req->port);
  printf("Version: %s\n", http_req->http_version);

  return OK;
}

int http_process_headers(xps_http_req_t *http_req, xps_buffer_t *buff) {

  assert(http_req != NULL);
  assert(buff != NULL);
  /*initialize headers list of http_req*/
  vec_init(&(http_req->headers));
  int error;
  while (1) {
    error = xps_http_parse_header_line(http_req, buff);
    if (error == E_FAIL || error == E_AGAIN)
      break;
    if (error == OK || error == E_NEXT) {
      /* Alloc memory for new header*/
      /*assign key,val from their corresponding start and end pointers*/
      xps_keyval_t *header = http_header_create(http_req);
      if (header == NULL) {
        logger(LOG_ERROR, "http_process_headers()",
               "http_header_create() failed");
        error = E_FAIL;
        break;
      }
      /*push this header into headers list of http_req*/
      vec_push(&(http_req->headers), header);

      if(error == E_NEXT) continue;
    }

    printf("HEADERS\n");
    for (int i = 0; i < http_req->headers.length; i++) {
      xps_keyval_t *header = http_req->headers.data[i];
      printf("%s: %s\n", header->key, header->val);
    }

    return OK;
  }
  /*error occurs, thus iterate through header list, free each header*/
  /*deinitialize headers list*/
  for (int i = 0; i < http_req->headers.length; i++) {
    xps_keyval_t *header = http_req->headers.data[i];
    free(header->key);
    free(header->val);
    free(header);
  }
  vec_deinit(&(http_req->headers));

  return error;
}

xps_buffer_t *xps_http_req_serialize(xps_http_req_t *http_req) {
  assert(http_req != NULL);

  /* Serialize headers into a buffer headers_str*/
  xps_buffer_t *headers_str = xps_http_serialize_headers(&(http_req->headers));

  if (headers_str == NULL) {
    logger(LOG_ERROR, "xps_http_req_serialize()",
           "xps_http_serialize_header() failed");
    return NULL;
  }

  size_t final_len = strlen(http_req->request_line) + 1 + headers_str->len +
                     1; // Calculate length for final buffer
  /*Create instance for final buffer*/
  xps_buffer_t *buff = xps_buffer_create(final_len, 0, NULL);

  if (buff == NULL) {
    logger(LOG_ERROR, "xps_http_req_serialize()", "xps_buffer_create() failed");
    return NULL;
  }

  // Copy everything to final buffer
  memcpy(buff->pos, http_req->request_line, strlen(http_req->request_line));
  buff->pos += strlen(http_req->request_line);
  /*similarly copy "\n", headers_str, "\n"*/
  memcpy(buff->pos, "\n", 1);
  buff->pos += 1;
  memcpy(buff->pos, headers_str->data, headers_str->len);
  buff->pos += headers_str->len;
  memcpy(buff->pos, "\n", 1);
  buff->pos += 1;

  /*destroy headers_str buffer*/
  xps_buffer_destroy(headers_str);
  return buff;
}

xps_http_req_t *xps_http_req_create(xps_core_t *core, xps_buffer_t *buff,
                                    int *error) {
  /*assert*/
  assert(core != NULL);
  assert(buff != NULL);
  *error = E_FAIL;
  /* Alloc memory for http_req instance*/
  xps_http_req_t *http_req = malloc(sizeof(xps_http_req_t));
  if (http_req == NULL) {
    logger(LOG_ERROR, "xps_http_req_create()", "malloc() failed for http_req");
    return NULL;
  }
  memset(http_req, 0, sizeof(xps_http_req_t));
  /*Set initial parser state*/
  http_req->parser_state = RL_START;
  /*Process request line and handle possible errors*/
  int ret = http_process_request_line(http_req, buff);
  if (ret == E_FAIL || ret == E_AGAIN) {
    logger(LOG_ERROR, "xps_http_req_create()",
           "http_process_request_line() return E_FAIL or E_AGAIN");
    free(http_req);
    *error = ret;
    return NULL;
  }
  /*Process headers and handle possible errors*/
  ret = http_process_headers(http_req, buff);
  if (ret == E_FAIL || ret == E_AGAIN) {
    logger(LOG_ERROR, "xps_http_req_create()",
           "http_process_headers() return E_FAIL or E_AGAIN");
    free(http_req->request_line);
    free(http_req->method);
    free(http_req->uri);
    free(http_req->schema);
    free(http_req->host);
    free(http_req->path);
    free(http_req->pathname);
    free(http_req->http_version);
    free(http_req);
    *error = ret;
    return NULL;
  }
  // Header length
  http_req->header_len = (size_t)(buff->pos - buff->data);
  // Body length is retrieved from header Content-Length
  http_req->body_len = 0;
  const char *body_len_str = xps_http_get_header(
      &(http_req->headers),
      "Content-Length"); /*get header value for key Content-Length*/
  /*assign body_len*/
  if (body_len_str != NULL)
    http_req->body_len = atoi(body_len_str);
  *error = OK;

  logger(LOG_DEBUG, "xps_http_req_create()", "http_req created succesffully");

  return http_req;
}

void xps_http_req_destroy(xps_core_t *core, xps_http_req_t *http_req) {
  assert(http_req != NULL);
  /*Frees memory allocated for various components of the HTTP request
   * line(request line, method,etc)*/
  free(http_req->request_line);
  free(http_req->method);
  free(http_req->uri);
  free(http_req->schema);
  free(http_req->host);
  free(http_req->path);
  free(http_req->pathname);
  free(http_req->http_version);

  /* iterate through the headers list of http_req and free the memory*/
  for (int i = 0; i < http_req->headers.length; i++) {
    xps_keyval_t *header = http_req->headers.data[i];
    free(header->key);
    free(header->val);
    free(header);
  }
  /*de-intialize the headers list*/
  vec_deinit(&(http_req->headers));
  /*free http_req*/
  free(http_req);

  logger(LOG_DEBUG, "xps_http_req_destroy()", "destroyed http_req");
}