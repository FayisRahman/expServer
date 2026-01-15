#ifndef XPS_UTILS_H
#define XPS_UTILS_H

#include "../xps.h"

// Sockets
bool is_valid_port(u_int port);
int make_socket_non_blocking(u_int sock_fd);
struct addrinfo *xps_getaddrinfo(const char *host, u_int port);
char *get_remote_ip(u_int sock_fd);


// Other functions
void vec_filter_null(vec_void_t *v);
const char *get_file_ext(const char *file_path);

// string
char *str_from_ptrs(const char *start, const char *end);
bool str_starts_with(const char *str, const char *prefix);
char *str_create(const char *str);

//files and path
char* path_join(const char* str1, const char* str2);
bool is_abs_path(char* path);
bool is_file(char* path);
bool is_dir(char* path);




#endif