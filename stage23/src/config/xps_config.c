#include "xps_config.h"
#include <stdio.h>

void parse_server(JSON_Object *server_object, xps_config_server_t *server);
void parse_listener(JSON_Object *listener_object, xps_config_listener_t *listener);
void parse_route(JSON_Object *route_object, xps_config_route_t *route);
void parse_all_listeners(vec_void_t *_all_listeners, xps_config_server_t *server);

const char *default_gzip_mimes[] = {
  "text/html",
  "text/css",
  "text/plain",
  "text/xml",
  "text/x-component",
  "text/javascript",
  "application/x-javascript",
  "application/javascript",
  "application/json",
  "application/xml",
  "application/xhtml+xml",
  "application/rss+xml",
  "application/atom+xml",
  "application/vnd.ms-fontobject",
  "application/x-font-ttf",
  "application/x-font-opentype",
  "application/x-font-truetype",
  "image/svg+xml",
  "image/x-icon",
  "font/ttf",
  "font/eot",
  "font/otf",
  "font/opentype",
};

int n_default_gzip_mimes = sizeof(default_gzip_mimes) / sizeof(default_gzip_mimes[0]);

xps_config_t *xps_config_create(const char *config_path) {
  /*assert*/
  assert(config_path != NULL);
  /*allocate mem for config*/
  xps_config_t *config = malloc(sizeof(xps_config_t));
  if (config == NULL) {
    logger(LOG_ERROR, "xps_config_create()", "malloc() failed for config");
    return NULL;
  }
  /*get config_json using json_parse_file*/
  JSON_Value *config_json = json_parse_file(config_path);
  if (config_json == NULL) {
    logger(LOG_ERROR, "xps_config_create()", "json_parse_file() failed");
    return NULL;
  }
  /*initialize fields of config object*/
  config->_config_json = config_json;
  config->config_path = config_path;

  JSON_Object *root_object = json_value_get_object(config_json);
  if (root_object == NULL) {
    logger(LOG_ERROR, "xps_config_create()", "failed to parse root_object");
    return NULL;
  }
  /*initialize server_name,workers,servers fields - hint: use
  json_object_get_string ,json_object_get_number,json_object_get_array*/
  config->server_name = json_object_get_string(root_object, "server_name");
  config->workers = json_object_get_number(root_object, "workers");

  /*Setting Up `server` Array*/
  JSON_Array *servers = json_object_get_array(root_object, "servers");
  vec_init(&(config->servers));
  if (servers == NULL) {
    logger(LOG_ERROR, "xps_config_create()", "failed to parse servers");
    return NULL;
  }
  for (size_t i = 0; i < json_array_get_count(servers); i++) {
    JSON_Object *server_object = json_array_get_object(servers, i);
    if (server_object == NULL) {
      logger(LOG_ERROR, "xps_config_create()", "failed to parse server_object");
      return NULL;
    }
    xps_config_server_t *server = malloc(sizeof(xps_config_server_t));
    if (server == NULL) {
      logger(LOG_ERROR, "xps_config_create()", "malloc() failed for server");
      return NULL;
    }
    vec_init(&(server->routes));
    vec_init(&(server->listeners));
    vec_init(&server->hostnames);
    parse_server(server_object, server);
    vec_push(&(config->servers), server);
  }

  /*Setting up `_all_listeners` Array*/
  vec_init(&(config->_all_listeners));
  for (int i = 0; i < config->servers.length; i++) {
    parse_all_listeners(&(config->_all_listeners), config->servers.data[i]);
  }

  logger(LOG_DEBUG, "xps_config_create()", "Config file parsed successfully");
  return config;
}

void xps_config_destroy(xps_config_t *config) {
  assert(config != NULL);

  for (int i = 0; i < config->servers.length; i++) {
    xps_config_server_t *server = config->servers.data[i];
    for (int j = 0; j < server->listeners.length; j++)
      free(server->listeners.data[j]);
    vec_deinit(&server->listeners);
    for (int j = 0; j < server->routes.length; j++) {
      xps_config_route_t *route = server->routes.data[j];
      vec_deinit(&(route->index));
      vec_deinit(&(route->upstreams));
      vec_deinit(&(route->ip_whitelist));
      vec_deinit(&(route->ip_blacklist));
      vec_deinit(&(route->gzip_mime_types));
      free(route);
    }
    vec_deinit(&(server->routes));
    vec_deinit(&(server->hostnames));
    free(server);
  }
  vec_deinit(&(config->servers));
  vec_deinit(&(config->_all_listeners));

  json_value_free(config->_config_json);
  free(config);
}

xps_config_lookup_t *xps_config_lookup(xps_config_t *config, xps_http_req_t *http_req,
                                       xps_connection_t *client, int *error) {
  /*assert*/
  assert(config != NULL);
  assert(http_req != NULL);
  assert(client != NULL);

  // CASE : METRICS  TODO: STAGE22
  if (client->listener->port == METRICS_PORT) {
    xps_config_lookup_t *lookup = malloc(sizeof(xps_config_lookup_t));
    if (lookup == NULL) {
      logger(LOG_ERROR, "xps_config_lookup()", "malloc() failed for 'lookup'");
      return NULL;
    }
    lookup->type = REQ_METRICS;
    lookup->file_path = NULL;
    lookup->dir_path = NULL;

    *error = OK;
    return lookup;
  }

  *error = E_FAIL;
  /*get host,keep_alive(connection),accept encoding,pathname from http_req*/

  const char *h_host = xps_http_get_header(&(http_req->headers), "Host");
  const char *h_keep_alive = xps_http_get_header(&(http_req->headers), "Connection");
  const char *h_accept_encoding = xps_http_get_header(&(http_req->headers), "Accept-Encoding");
  const char *h_pathname = http_req->pathname;
  // Step 1: Find matching server block
  int target_server_index = -1;
  for (int i = 0; i < config->servers.length; i++) {
    xps_config_server_t *server = config->servers.data[i];
    // Check if client listener is present in server
    bool has_matching_listener = false;
    for (int j = 0; j < server->listeners.length; j++) {
      xps_config_listener_t *listener = server->listeners.data[j];
      if (strcmp(listener->host, client->listener->host) == 0 &&
          listener->port == client->listener->port) {
        has_matching_listener = true;
        break;
      }
    }
    if (!has_matching_listener)
      continue;

    /* Check if host header matches any hostname*/
    // NOTE: if not hostnames provided, it is considered a match
    bool has_matching_hostname = server->hostnames.length == 0;
    for (int j = 0; j < server->hostnames.length; j++) {
      if (strcmp(server->hostnames.data[j], h_host) == 0) {
        has_matching_hostname = true;
        break;
      }
    }
    if (has_matching_hostname) {
      target_server_index = i;
      break;
    }
  }

  if (target_server_index == -1) {
    *error = E_NOTFOUND;
    return NULL;
  }

  xps_config_server_t *server = config->servers.data[target_server_index];
  /*Find matching route block*/
  xps_config_route_t *route = NULL;
  int longest_path_len = -1;
  for (int i = 0; i < server->routes.length; i++) {
    xps_config_route_t *current_route = server->routes.data[i];
    size_t route_path_len = strlen(current_route->req_path);

    if (str_starts_with(h_pathname, current_route->req_path)) {

      if ((int)route_path_len > longest_path_len) {
        route = current_route;
        longest_path_len = (int)route_path_len;
      }
    }
  }

  if (route == NULL) {
    *error = E_NOTFOUND; // No matching route found - 404
    return NULL;
  }

  /* Init values of lookup*/
  xps_config_lookup_t *lookup = malloc(sizeof(xps_config_lookup_t));
  if (lookup == NULL) {
    logger(LOG_ERROR, "xps_config_lookup()", "malloc() failed for 'lookup'");
    return NULL;
  }
  if (strcmp(route->type, "file_serve") == 0) {
    lookup->type = REQ_FILE_SERVE;

  } else if (strcmp(route->type, "reverse_proxy") == 0)
    lookup->type = REQ_REVERSE_PROXY;
  else if (strcmp(route->type, "redirect") == 0)
    lookup->type = REQ_REDIRECT;
  else
    lookup->type = REQ_INVALID;
  // Initialize common fields
  lookup->keep_alive = h_keep_alive && strcmp(h_keep_alive, "keep-alive") == 0;
  /*need to understand why these values*/
  lookup->file_path = NULL;
  lookup->dir_path = NULL;
  lookup->file_start = 0;
  lookup->file_end = -1;
  lookup->gzip_enable =
    route->gzip_enable && h_accept_encoding && strstr(h_accept_encoding, "gzip");
  lookup->gzip_level = route->gzip_level;
  lookup->upstream = route->upstreams.length > 0 ? route->upstreams.data[0] : NULL;
  /*till here |^*/
  lookup->http_status_code = route->http_status_code;
  lookup->redirect_url = route->redirect_url;
  lookup->ip_whitelist = route->ip_whitelist;
  lookup->ip_blacklist = route->ip_blacklist;

  // Initialize type-specific fields
  if (lookup->type == REQ_FILE_SERVE) {
    char *resource_path = path_join(
      route->dir_path, h_pathname + strlen(route->req_path)); /*need to specify this in the doc*/
    if (!is_abs_path(resource_path)) { // we require abosulte path so we need to see if
                                       // the current path is absolute or not
      char *temp = str_create(config->config_path);
      char *temp2 = path_join(dirname(temp), resource_path);
      free(temp);
      free(resource_path);
      resource_path = temp2;

      // printf("resource_path: %s\n", resource_path);
    }
    // is file
    if (is_file(resource_path)) {
      lookup->file_path = resource_path;

    } else if (is_dir(resource_path)) { // is directory
      bool index_file_found = false;
      for (int i = 0; i < route->index.length; i++) {
        char *index_file = path_join(resource_path, route->index.data[i]);
        if (is_file(index_file)) {
          lookup->file_path = index_file;
          index_file_found = true;
          free(resource_path);
          break;
        } else {
          free(index_file);
        }
      }

      // no index file found so we show the directory contents (directory browsing)
      if (!index_file_found) {
        lookup->dir_path = resource_path;
      }
    } else {
      free(resource_path);
    }

    if (lookup->file_path) {
      const char *mime = xps_get_mime(lookup->file_path);

      if (mime) {
        bool mime_match = false;
        for (size_t i = 0; i < n_default_gzip_mimes; i++) {
          if (strcmp(mime, default_gzip_mimes[i]) == 0) {
            mime_match = true;

            break;
          }
        }

        for (size_t i = 0; i < route->gzip_mime_types.length && !mime_match; i++) {
          if (strcmp(mime, route->gzip_mime_types.data[i]) == 0) {
            mime_match = true;
            break;
          }
        }

        lookup->gzip_enable = lookup->gzip_enable && mime_match;
      } else {
        lookup->gzip_enable = false;
      }
    }

    logger(LOG_DEBUG, "xps_config_file_lookup()", "requested file path: %s", lookup->file_path);

  } else if (lookup->type == REQ_REVERSE_PROXY) {

    if (strcmp(route->load_balancing, "round_robin") == 0) {

      int index = route->_round_robin_counter % route->upstreams.length;
      route->_round_robin_counter++;
      lookup->upstream = route->upstreams.data[index];
    } else if (strcmp(route->load_balancing, "ip_hash") == 0) {
      u_int ip;
      inet_pton(AF_INET, client->remote_ip, &ip);
      int index = ip % route->upstreams.length;
      lookup->upstream = route->upstreams.data[index];
    } else {
      logger(LOG_ERROR, "xps_config_lookup()", "invalid load balancing strategy");
    }
  }
  *error = OK;
  return lookup;
}

void xps_config_lookup_destroy(xps_config_lookup_t *config_lookup, xps_core_t *core) {
  assert(config_lookup != NULL);
  assert(core != NULL);

  if (config_lookup->dir_path)
    free(config_lookup->dir_path);

  if (config_lookup->file_path)
    free(config_lookup->file_path);

  free(config_lookup);
}

void parse_server(JSON_Object *server_object, xps_config_server_t *server) {

  /*Setting Up `listeners` Array*/
  JSON_Array *listeners = json_object_get_array(server_object, "listeners");

  if (listeners)
    for (size_t j = 0; j < json_array_get_count(listeners); j++) {
      JSON_Object *listener_object = json_array_get_object(listeners, j);
      xps_config_listener_t *listener = malloc(sizeof(xps_config_listener_t));
      if (listener == NULL) {
        logger(LOG_ERROR, "parse_server()", "malloc() failed for listener");
        return;
      }
      parse_listener(listener_object, listener);
      vec_push(&(server->listeners), listener);
    }

  /*Setting up `routes` Array*/
  JSON_Array *routes = json_object_get_array(server_object, "routes");
  if (routes)
    for (size_t j = 0; j < json_array_get_count(routes); j++) {
      JSON_Object *route_object = json_array_get_object(routes, j);
      xps_config_route_t *route = malloc(sizeof(xps_config_route_t));
      if (route == NULL) {
        logger(LOG_ERROR, "parse_server()", "malloc() failed for route");
        return;
      }
      route->req_path = NULL;
      route->type = NULL;
      route->dir_path = NULL;
      vec_init(&(route->upstreams));
      vec_init(&(route->index));
      vec_init(&route->ip_whitelist);
      vec_init(&route->ip_blacklist);
      vec_init(&route->gzip_mime_types);
      route->gzip_enable = false;
      route->gzip_level = -1; // valid values: [-1, 9]
      route->load_balancing = "round_robin";
      route->_round_robin_counter = 0;
      route->http_status_code = 0;
      route->redirect_url = NULL;
      route->keep_alive = false;

      parse_route(route_object, route);

      vec_push(&(server->routes), route);
    }

  /*Setting up `hostnames` Array*/
  JSON_Array *hostnames = json_object_get_array(server_object, "hostnames");
  if (hostnames)
    for (int i = 0; i < json_array_get_count(hostnames); i++) {
      const char *hostname = json_array_get_string(hostnames, i);
      vec_push(&server->hostnames, (void *)hostname);
    }
}

void parse_listener(JSON_Object *listener_object, xps_config_listener_t *listener) {
  listener->host = json_object_get_string(listener_object, "host");
  if (listener->host == NULL) {
    logger(LOG_ERROR, "parse_listener()", "host is required");
    return;
  }
  listener->port = json_object_get_number(listener_object, "port");
  if (listener->port == 0) {
    logger(LOG_ERROR, "parse_listener()", "port is required");
    return;
  } else if (listener->port == METRICS_PORT) { // TODO: stage22
    logger(LOG_ERROR, "parse_listener()", "cannot use METRICS_PORT");
    return;
  }
}

void parse_route(JSON_Object *route_object, xps_config_route_t *route) {

  route->req_path = json_object_get_string(route_object, "req_path");

  if (!route->req_path) {
    logger(LOG_ERROR, "parse_route()", "failed to parse req_path");
    return;
  }

  route->type = json_object_get_string(route_object, "type");
  if (strcmp(route->type, "file_serve") && strcmp(route->type, "reverse_proxy") &&
      strcmp(route->type, "redirect")) {
    logger(LOG_ERROR, "parse_route()", "invalid route type");
    return;
  }

  if (strcmp(route->type, "file_serve") == 0) {

    /*if file server */
    route->dir_path = json_object_get_string(route_object, "dir_path");
    if (route->dir_path == NULL) {
      logger(LOG_ERROR, "parse_route()", "dir_path is required");
      return;
    }
    JSON_Array *indexes = json_object_get_array(route_object, "index");
    if (indexes)
      for (size_t k = 0; k < json_array_get_count(indexes); k++) {
        const char *index = json_array_get_string(indexes, k);
        vec_push(&(route->index), (void *)index);
      }

    // gzip_enable
    route->gzip_enable = (bool)json_object_get_boolean(route_object, "gzip_enable");

    // gzip level
    route->gzip_level = (int)json_object_get_number(route_object, "gzip_level");
    if (route->gzip_level < -1 || route->gzip_level > 9) {
      logger(LOG_ERROR, "parse_route()", "gzip_level out of range -1 to 9");
      return;
    }

    // gzip_mime_types
    JSON_Array *gzip_mime_types = json_object_get_array(route_object, "gzip_mime_types");
    if (gzip_mime_types)
      for (size_t i = 0; i < json_array_get_count(gzip_mime_types); i++)
        vec_push(&route->gzip_mime_types, (void *)json_array_get_string(gzip_mime_types, i));

  } else if (strcmp(route->type, "redirect") == 0) {

    /*if redirect*/
    route->http_status_code = json_object_get_number(route_object, "http_status_code");
    route->redirect_url = json_object_get_string(route_object, "redirect_url");
    if (route->redirect_url == NULL) {
      logger(LOG_ERROR, "parse_route()", "redirect_url required");
      return;
    }

  } else if (strcmp(route->type, "reverse_proxy") == 0) {

    /*if upstream*/
    JSON_Array *upstreams = json_object_get_array(route_object, "upstreams");

    if (upstreams == NULL) {
      logger(LOG_ERROR, "parse_route()", "upstreams is required");
      return;
    }
    for (size_t k = 0; k < json_array_get_count(upstreams); k++) {
      const char *upstream = json_array_get_string(upstreams, k);
      vec_push(&(route->upstreams), (void *)upstream);
    }

    // load_balancing
    const char *lb = json_object_get_string(route_object, "load_balancing");
    if (lb)
      route->load_balancing = lb;
  }

  JSON_Array *ip_whitelist = json_object_get_array(route_object, "ip_whitelist");
  JSON_Array *ip_blacklist = json_object_get_array(route_object, "ip_blacklist");

  if (ip_whitelist && !ip_blacklist) {
    for (size_t i = 0; i < json_array_get_count(ip_whitelist); i++)
      vec_push(&(route->ip_whitelist), (void *)json_array_get_string(ip_whitelist, i));
  } else if (ip_blacklist && !ip_whitelist) {
    for (size_t i = 0; i < json_array_get_count(ip_blacklist); i++)
      vec_push(&(route->ip_blacklist), (void *)json_array_get_string(ip_blacklist, i));
  } else if (ip_whitelist && ip_blacklist) {
    // since both whitelist and blacklist are present we just need to filter the whitelist ips from
    // the blacklist ips and then we dont requrie blacklist ip array the reason for that is when
    // both exist teh whitelist takes priority as we just need to allow ips present in the whitelist
    // so what we do here is we remove ips from whitelist array that is also present in blacklist
    // array as well

    for (size_t i = 0; i < json_array_get_count(ip_whitelist); i++) {
      const char *ip_w = json_array_get_string(ip_whitelist, i);

      bool in_blacklist = false;
      for (size_t j = 0; j < json_array_get_count(ip_blacklist); j++) {
        const char *ip_b = json_array_get_string(ip_blacklist, j);
        if (strcmp(ip_w, ip_b) == 0) {
          in_blacklist = true;
          break;
        }
      }

      if (!in_blacklist)
        vec_push(&(route->ip_whitelist), (void *)ip_w);
    }
  }
}

void parse_all_listeners(vec_void_t *_all_listeners, xps_config_server_t *server) {

  for (int i = 0; i < server->listeners.length; i++) {

    xps_config_listener_t *server_listener = server->listeners.data[i];

    /*check if this listener is in the _all_listener array*/
    bool listener_found = false;

    for (int j = 0; j < _all_listeners->length; j++) {
      xps_config_listener_t *curr = _all_listeners->data[i];

      if (strcmp(server_listener->host, curr->host) == 0 && server_listener->port == curr->port) {
        listener_found = true;
        break;
      }
    }

    if (!listener_found) {
      vec_push(_all_listeners, server_listener);
    }
  }
}