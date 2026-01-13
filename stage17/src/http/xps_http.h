#ifndef XPS_HTTP_H
#define XPS_HTTP_H

#include "../xps.h"

#define CR '\r'
#define LF '\n'

typedef enum {
  HTTP_GET,
  HTTP_HEAD,
  HTTP_POST,
  HTTP_PUT,
  HTTP_DELETE,
  HTTP_OPTIONS,
  HTTP_TRACE,
  HTTP_CONNECT,
} xps_http_method_t;

typedef enum {
  HTTP_OK = 200,
  HTTP_CREATED = 201,

  HTTP_MOVED_PERMANENTLY = 301,
  HTTP_MOVED_TEMPORARILY = 302,
  HTTP_NOT_MODIFIED = 304,
  HTTP_TEMPORARY_REDIRECT = 307,
  HTTP_PERMANENT_REDIRECT = 308,

  HTTP_BAD_REQUEST = 400,
  HTTP_UNAUTHORIZED = 401,
  HTTP_FORBIDDEN = 403,
  HTTP_NOT_FOUND = 404,
  HTTP_REQUEST_TIME_OUT = 408,
  HTTP_TOO_MANY_REQUESTS = 429,

  HTTP_INTERNAL_SERVER_ERROR = 500,
  HTTP_NOT_IMPLEMENTED = 501,
  HTTP_BAD_GATEWAY = 502,
  HTTP_SERVICE_UNAVAILABLE = 503,
  HTTP_GATEWAY_TIMEOUT = 504,
  HTTP_HTTP_VERSION_NOT_SUPPORTED = 505
} xps_http_status_code_t;

typedef enum {
  /* Request line states */
  RL_START = 0,
  RL_METHOD,
  RL_SP_AFTER_METHOD,

  RL_SCHEMA,
  RL_SCHEMA_SLASH,
  RL_SCHEMA_SLASH_SLASH,
  RL_HOST_START, // maybe Ipv4 or Ipv6
  RL_HOST,
  RL_HOST_END,
  RL_HOST_IP_LITERAL, // Ipv6; map to RL_HOST_END
  RL_PORT,
  RL_SLASH, // path
  RL_CHECK_URI,
  RL_PATH,
  RL_PATHNAME,
  RL_SP_AFTER_URI,

  RL_VERSION_START,
  RL_VERSION_H,
  RL_VERSION_HT,
  RL_VERSION_HTT,
  RL_VERSION_HTTP,
  RL_VERSION_HTTP_SLASH,
  RL_VERSION_MAJOR,
  RL_VERSION_DOT,
  RL_VERSION_MINOR,
  RL_CR,
  RL_LF,

  /* Header states */
  H_START = 0,
  H_NAME,
  H_COLON,
  H_SP_AFTER_COLON,
  H_VAL,
  H_CR,
  H_LF,
  H_LF_CR,
  H_LF_LF,
  H_LF_CR_LF,

} xps_http_parser_state_t;

int xps_http_parse_request_line(xps_http_req_t *http_req, xps_buffer_t *buffer);
int xps_http_parse_header_line(xps_http_req_t *http_req, xps_buffer_t *buffer);

const char *xps_http_get_header(vec_void_t *headers, const char *key);
xps_buffer_t *xps_http_serialize_headers(vec_void_t *headers);
int xps_http_set_header(vec_void_t *headers, const char *key, const char *val);

#endif