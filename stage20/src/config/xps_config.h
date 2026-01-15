#ifndef XPS_CONFIG_H
#define XPS_CONFIG_H

#include "../xps.h"

struct xps_config_s {
  const char *config_path;
  const char *server_name;
  u_int workers;
  vec_void_t servers;
  vec_void_t _all_listeners;
  JSON_Value *_config_json;
};

struct xps_config_server_s {
  vec_void_t listeners;
  vec_void_t hostnames;
  vec_void_t routes;
};

struct xps_config_listener_s {
  const char *host;
  u_int port;
};

struct xps_config_route_s {
  const char *req_path;
  const char *type;
  const char *dir_path;
  vec_void_t index;
  vec_void_t ip_whitelist; 
  vec_void_t ip_blacklist; 
  bool gzip_enable;             //TODO : stage19
  int gzip_level;               //TODO : stage19
  vec_void_t gzip_mime_types;   //TODO : stage19  // get default mime types and append the rest
  vec_void_t upstreams;
  u_int http_status_code;
  const char *redirect_url;
  bool keep_alive;
};

typedef enum xps_req_type_e {
  REQ_FILE_SERVE,
  REQ_REVERSE_PROXY,
  REQ_REDIRECT,
  REQ_METRICS,
  REQ_INVALID
} xps_req_type_t;

struct xps_config_lookup_s {
  xps_req_type_t type;

  /* file_serve */
  char *file_path; // absolute path
  char *dir_path;  // absolute path
  long file_start; // parse range header
                   // https://developer.mozilla.org/en-US/docs/Web/HTTP/Range_requests
  long file_end;

  bool gzip_enable;           //TODO : stage19
  int gzip_level; // -1 to 9  //TODO : stage19

  /* reverse_proxy */
  const char *upstream;

  /* redirect */
  u_int http_status_code;
  const char *redirect_url;

  /* common */
  bool keep_alive;
  vec_void_t ip_whitelist; 
  vec_void_t ip_blacklist; 
};

xps_config_t *xps_config_create(const char *config_path);
void xps_config_destroy(xps_config_t *config);
xps_config_lookup_t *xps_config_lookup(xps_config_t *config, xps_http_req_t *http_req,
                                       xps_connection_t *client, int *error);
void xps_config_lookup_destroy(xps_config_lookup_t *config_lookup, xps_core_t *core);

#endif