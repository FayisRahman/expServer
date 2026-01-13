#ifndef XPS_DIRECTORY
#define XPS_DIRECTORY

#include "../xps.h"

//TODO: explain this module
xps_buffer_t *xps_directory_browsing(const char *dir_path, const char *pathname);

//might need to add xps_directory_destroy to destory the buffer;

#endif