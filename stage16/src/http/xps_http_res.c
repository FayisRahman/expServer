#include "xps_http_res.h"

xps_http_res_t *xps_http_res_create(xps_core_t *core, u_int code) {

  assert(core != NULL);

  xps_http_res_t *http_res = malloc(sizeof(xps_http_res_t));
  if (http_res == NULL) {
    logger(LOG_ERROR, "xps_http_res_create()",
           "failed to alloc memory for http_res. malloc() returned NULL");
    return NULL;
  }

  const char *reason_phrase = "Unknown";
  switch (code) {
  case HTTP_OK:
    reason_phrase = "OK";
    break;
  case HTTP_CREATED:
    reason_phrase = "Created";
    break;
  case HTTP_MOVED_PERMANENTLY:
    reason_phrase = "Moved Permanently";
    break;
  case HTTP_MOVED_TEMPORARILY:
    reason_phrase = "Moved Temporarily";
    break;
  case HTTP_NOT_MODIFIED:
    reason_phrase = "Not Modified";
    break;
  case HTTP_TEMPORARY_REDIRECT:
    reason_phrase = "Temporary Redirect";
    break;
  case HTTP_PERMANENT_REDIRECT:
    reason_phrase = "Permanent Redirect";
    break;
  case HTTP_BAD_REQUEST:
    reason_phrase = "Bad Request";
    break;
  case HTTP_UNAUTHORIZED:
    reason_phrase = "Unauthorized";
    break;
  case HTTP_FORBIDDEN:
    reason_phrase = "Forbidden";
    break;
  case HTTP_NOT_FOUND:
    reason_phrase = "Not Found";
    break;
  case HTTP_REQUEST_TIME_OUT:
    reason_phrase = "Request Time-out";
    break;
  case HTTP_TOO_MANY_REQUESTS:
    reason_phrase = "Too Many Requests";
    break;
  case HTTP_INTERNAL_SERVER_ERROR:
    reason_phrase = "Internal Server Error";
    break;
  case HTTP_NOT_IMPLEMENTED:
    reason_phrase = "Not Implemented";
    break;
  case HTTP_BAD_GATEWAY:
    reason_phrase = "Bad Gateway";
    break;
  case HTTP_SERVICE_UNAVAILABLE:
    reason_phrase = "Service Unavailable";
    break;
  case HTTP_GATEWAY_TIMEOUT:
    reason_phrase = "Gateway Time-out";
    break;
  case HTTP_HTTP_VERSION_NOT_SUPPORTED:
    reason_phrase = "HTTP Version not supported";
    break;
  }

  snprintf(http_res->response_line, sizeof(http_res->response_line),
           "HTTP/1.1 %d %s", code, reason_phrase);

  vec_init(&http_res->headers);

  // Get current time
  time_t now = time(0);
  struct tm tm = *gmtime(&now);
  char time_buf[128];
  strftime(time_buf, sizeof time_buf, "%a, %d %b %Y %H:%M:%S GMT", &tm);

  xps_http_set_header(&(http_res->headers), "Date", time_buf);
  xps_http_set_header(&(http_res->headers), "Server", SERVER_NAME);
  xps_http_set_header(&(http_res->headers), "Access-Control-Allow-Origin", "*");

  http_res->body = NULL;

  return http_res;
}

void xps_http_res_destroy(xps_http_res_t *res) {
  assert(res != NULL);

  for (int i = 0; i < res->headers.length; i++) {
    xps_keyval_t *header = res->headers.data[i];
    free(header->key);
    free(header->val);
    free(header);
  }

  vec_deinit(&(res->headers));

  if (res->body)
    xps_buffer_destroy(res->body);

  free(res);
}

xps_buffer_t *xps_http_res_serialize(xps_http_res_t *http_res) {
  /* valid params */
  assert(http_res != NULL);

  // Serialize headers
  xps_buffer_t *headers_str = xps_http_serialize_headers(&(http_res->headers));
  if (headers_str == NULL) {
    logger(LOG_ERROR, "xps_http_res_serialize()",
           "failed to serialize headers");
    return NULL;
  }

  // Calculate length for final buffer
  size_t final_len = strlen(http_res->response_line) + 1 + headers_str->len +
                     1 + (http_res->body ? http_res->body->len : 0);

  // Create instance for final buffer
  xps_buffer_t *buff = xps_buffer_create(final_len, 0, NULL);
  if (buff == NULL) {
    logger(LOG_ERROR, "xps_http_res_serialize()",
           "failed to create buffer instance");
    xps_buffer_destroy(headers_str);
    return NULL;
  }

  // Copy everything
  /* copy response line */
  memcpy(buff->pos, http_res->response_line, strlen(http_res->response_line));
  buff->pos += strlen(http_res->response_line);
  memcpy(buff->pos, "\n", 1);
  buff->pos += 1;

  /* copy headers */
  memcpy(buff->pos, headers_str->data, headers_str->len);
  buff->pos += headers_str->len;
  memcpy(buff->pos, "\n", 1);
  buff->pos += 1;

  if (http_res->body != NULL) {
    /* copy response body*/
    memcpy(buff->pos, http_res->body->data, http_res->body->len);
    buff->pos += http_res->body->len;
  }

  buff->len = final_len;

  xps_buffer_destroy(headers_str);

  logger(LOG_DEBUG, "xps_http_res_serialize()",
         "http response serialized succefully");

  return buff;
}