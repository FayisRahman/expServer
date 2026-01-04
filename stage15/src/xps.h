#ifndef XPS_H
#define XPS_H

// Header files
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>


// 3rd party libraries
#include "lib/vec/vec.h" // https://github.com/rxi/vec

// Constants
#define DEFAULT_BACKLOG 64
#define MAX_EPOLL_EVENTS 32
#define DEFAULT_BUFFER_SIZE 100000       // 100 KB
#define DEFAULT_PIPE_BUFF_THRESH 1000000 // 1 MB
#define DEFAULT_NULLS_THRESH 32
#define LOCALHOST "127.0.0.1"
#define SERVER_NAME "Expserver"

// Error constants
#define OK 0            // Success
#define E_FAIL -1       // Un-recoverable error
#define E_AGAIN -2      // Try again
#define E_NEXT -3       // Do next
#define E_NOTFOUND -4   // File not found
#define E_PERMISSION -5 // File permission denied
#define E_EOF -6        // End of file reached

// Data types
typedef unsigned char u_char;
typedef unsigned int u_int;
typedef unsigned long u_long;

// Structs
struct xps_core_s;
struct xps_loop_s;
struct xps_listener_s;
struct xps_connection_s;
struct xps_buffer_s;
struct xps_buffer_list_s;
struct xps_pipe_s;
struct xps_pipe_source_s;
struct xps_pipe_sink_s;
struct xps_file_s;
struct xps_keyval_s {
  char *key;
  char *val;
};
struct xps_session_s;
struct xps_http_req_s;
struct xps_http_res_s;

// Struct typedefs
typedef struct xps_core_s xps_core_t;
typedef struct xps_loop_s xps_loop_t;
typedef struct xps_listener_s xps_listener_t;
typedef struct xps_connection_s xps_connection_t;
typedef struct xps_buffer_s xps_buffer_t;
typedef struct xps_buffer_list_s xps_buffer_list_t;
typedef struct xps_pipe_s xps_pipe_t;
typedef struct xps_pipe_source_s xps_pipe_source_t;
typedef struct xps_pipe_sink_s xps_pipe_sink_t;
typedef struct xps_file_s xps_file_t;
typedef struct xps_keyval_s xps_keyval_t;
typedef struct xps_session_s xps_session_t;
typedef struct xps_http_req_s xps_http_req_t;
typedef struct xps_http_res_s xps_http_res_t;

// Function typedefs
typedef void (*xps_handler_t)(void *ptr);

// xps headers
#include "core/xps_core.h"
#include "core/xps_loop.h"
#include "core/xps_pipe.h"
#include "core/xps_session.h"
#include "disk/xps_file.h"
#include "disk/xps_mime.h"
#include "http/xps_http.h"
#include "http/xps_http_req.h"
#include "http/xps_http_res.h"
#include "network/xps_connection.h"
#include "network/xps_listener.h"
#include "network/xps_upstream.h"
#include "utils/xps_buffer.h"
#include "utils/xps_logger.h"
#include "utils/xps_utils.h"

#endif