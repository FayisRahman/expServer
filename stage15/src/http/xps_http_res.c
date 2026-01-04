#include "xps_http_res.h"

xps_http_res_t *xps_http_res_create(xps_core_t *core, u_int code) {

  xps_http_res_t *http_res = malloc(sizeof(xps_http_res_t));
  if (http_res == NULL) {
    logger(LOG_ERROR, "xps_http_res_create()",
           "failed to alloc memory for http_res. malloc() returned NULL");
    return NULL;
  }

  // Get current time
  time_t now = time(0);
  struct tm tm = *gmtime(&now);
  char time_buf[128];
  strftime(time_buf, sizeof time_buf, "%a, %d %b %Y %H:%M:%S GMT", &tm);

  const char *reason_phrase = "Unknown";
  switch (code) {
  case 200:
    reason_phrase = "OK";
    break;
  case 400:
    reason_phrase = "Bad Request";
    break;
  case 403:
    reason_phrase = "Forbidden";
    break;
  case 404:
    reason_phrase = "Not Found";
    break;
  case 500:
    reason_phrase = "Internal Server Error";
    break;
  }

  snprintf(http_res->response_line, sizeof(http_res->response_line),
           "HTTP/1.1 %d %s\r\n", code, reason_phrase);

  vec_init(&http_res->headers);

  xps_http_set_header(&(http_res->headers), "Date", time_buf);
  xps_http_set_header(&(http_res->headers), "Server", SERVER_NAME);
  xps_http_set_header(&(http_res->headers), "Access-Control-Allow-Origin", "*");

  http_res->body = NULL;

  return http_res;
}

void xps_http_res_destroy(xps_http_res_t *res){
  
}