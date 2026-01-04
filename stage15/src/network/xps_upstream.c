#include "xps_upstream.h"

xps_connection_t *xps_upstream_create(xps_core_t *core, const char *host,
                                      u_int port) {
  /* validate parameter */
  assert(core != NULL);
  assert(host != NULL);
  assert(is_valid_port(port));

  int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (sock_fd < 0) {
    logger(LOG_ERROR, "xps_upstream_create()", "socket() failed");
    perror("Error message");
    return NULL;
  }

  /* create a socket and connect to host and port to upstream using
   * xps_getaddrinfo and connect function */

  struct addrinfo *upstream_addrinfo = xps_getaddrinfo(host, port);
  if (upstream_addrinfo == NULL) {
    logger(LOG_ERROR, "xps_upstream_create()", "getaddrinfo() failed");
    perror("Error message");
    close(sock_fd);
    return NULL;
  }

  int connect_error = connect(sock_fd, upstream_addrinfo->ai_addr,
                              upstream_addrinfo->ai_addrlen);

  if (!(connect_error == 0 || errno == EINPROGRESS)) {
    logger(LOG_ERROR, "xps_upstream_create()", "connect() failed");
    perror("Error message");
    close(sock_fd);
    return NULL;
  }

  /* create a connection to upstream with core and sock_fd*/
  xps_connection_t *connection = xps_connection_create(core, sock_fd);
  if (connection == NULL) {
    logger(LOG_ERROR, "xps_upstream_create()",
           "xps_connection_create() failed");
    perror("Error message");
    close(sock_fd);
    return NULL;
  }

  logger(LOG_DEBUG, "xps_upstream_create()", "upstream connection created");

  return connection;
}