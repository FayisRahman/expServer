#ifndef XPS_H
#define XPS_H

#define _GNU_SOURCE

// Header files
#include <arpa/inet.h>
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <netdb.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <zlib.h>
#include <time.h>
#include <sys/resource.h>
#include <pthread.h>

// 3rd party libraries
#include "lib/parson/parson.h"
#include "lib/vec/vec.h"

// Constants
#define SERVER_NAME "expServer"
#define LOCALHOST "127.0.0.1"
#define DEFAULT_BACKLOG 64
#define MAX_EPOLL_EVENTS 32
#define DEFAULT_NULLS_THRESH 32
#define DEFAULT_BUFFER_SIZE 100000       // 100 KB
#define DEFAULT_PIPE_BUFF_THRESH 1000000 // 1 MB
#define DEFAULT_HTTP_REQ_TIMEOUT_MSEC 60000 // 60sec
#define DEFAULT_METRICS_UPDATE_MSEC 500     // 500 msec
#define METRICS_HOST "0.0.0.0"
#define METRICS_PORT 8004


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
struct xps_config_s;
struct xps_config_server_s;
struct xps_config_listener_s;
struct xps_config_route_s;
struct xps_config_lookup_s;
struct xps_cliargs_s;
struct xps_gzip_s;
struct xps_timer_s;
struct xps_metrics_s;

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
typedef struct xps_config_s xps_config_t;
typedef struct xps_config_server_s xps_config_server_t;
typedef struct xps_config_listener_s xps_config_listener_t;
typedef struct xps_config_route_s xps_config_route_t;
typedef struct xps_config_lookup_s xps_config_lookup_t;
typedef struct xps_cliargs_s xps_cliargs_t;
typedef struct xps_gzip_s xps_gzip_t;
typedef struct xps_timer_s xps_timer_t;
typedef struct xps_metrics_s xps_metrics_t;

// Function typedefs
typedef void (*xps_handler_t)(void *ptr);

// Global Variables
extern xps_core_t **cores;
extern int n_cores;

// xps headers
#include "config/xps_config.h"
#include "core/xps_core.h"
#include "core/xps_loop.h"
#include "core/xps_pipe.h"
#include "core/xps_session.h"
#include "core/xps_timer.h"
#include "core/xps_metrics.h"
#include "disk/xps_directory.h"
#include "disk/xps_file.h"
#include "disk/xps_gzip.h"
#include "disk/xps_mime.h"
#include "http/xps_http.h"
#include "http/xps_http_req.h"
#include "http/xps_http_res.h"
#include "network/xps_connection.h"
#include "network/xps_listener.h"
#include "network/xps_upstream.h"
#include "utils/xps_buffer.h"
#include "utils/xps_cliargs.h"
#include "utils/xps_logger.h"
#include "utils/xps_utils.h"

#endif