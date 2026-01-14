#include "xps_directory.h"

xps_buffer_t *xps_directory_browsing(const char *dir_path, const char *pathname) {
  assert(dir_path != NULL);
  assert(pathname != NULL);

  // char* buff = malloc(DEFAULT_BUFFER_SIZE);

  // buffer for HTTP message
  xps_buffer_t *directory_browsing = xps_buffer_create(DEFAULT_BUFFER_SIZE, 0, NULL);
  if (directory_browsing == NULL) {
    logger(LOG_ERROR, "xps_directory_browsing()", "xps_buffer_create() returned NULL");
    free(NULL);
    return NULL;
  }

  int written = snprintf(
    (char *)directory_browsing->pos, directory_browsing->size - directory_browsing->len,
    "<html><head lang='en'><meta http-equiv='Content-Type' content='text/html; "
    "charset=UTF-8'><meta name='viewport' content='width=device-width, "
    "initial-scale=1.0'><title>Directory: %s</title><style>body{font-family: monospace; "
    "font-size: 15px;}td {padding: 1.5px 6px; padding-right: 20px;} h1{font-family: serif; "
    "margin: 0;} h3{font-family: serif;margin: 12px 0px; background-color: rgba(0,0,0,0.1); "
    "padding: 4px 0px;}</style></head><body><h1>eXpServer</h1><h3>Index of&nbsp;%s</h3><hr><table>",
    pathname, pathname);

  directory_browsing->pos += written;
  directory_browsing->len += written;

  // open DIR given by dir_path
  DIR *dir = opendir(dir_path);
  if (dir == NULL) {
    logger(LOG_ERROR, "xps_directory_browsing()", "opendir() failed for dir_path");
    xps_buffer_destroy(directory_browsing);
    return NULL;
  }

  struct dirent *dir_entry;

  while ((dir_entry = readdir(dir)) != NULL) {

    // skip . and .. entries
    if (strcmp(dir_entry->d_name, ".") == 0 || strcmp(dir_entry->d_name, "..") == 0)
      continue;

    // open file
    char full_path[1024];
    snprintf(full_path, 1024, "%s/%s", dir_path, dir_entry->d_name);

    int file_fd = open(full_path, O_RDONLY);
    if (file_fd == -1) {
      logger(LOG_ERROR, "xps_directory_browsing()", "failed to open the file %s", full_path);
      continue;
    }

    // Metadata of the file

    struct stat file_stat;

    // get metadata of the file
    if (fstat(file_fd, &file_stat) == -1) {
      logger(LOG_ERROR, "xps_directory_browsing()", "fstat()");
      close(file_fd);
      continue;
    }

    // check if regular file or a directory
    if (S_ISREG(file_stat.st_mode) || S_ISDIR(file_stat.st_mode)) {
      char *is_dir = S_ISDIR(file_stat.st_mode) ? "/" : "";

      char *temp_pathname = str_create(pathname);

      // to remove the extra '/' when concatenating with the entry_name
      if (temp_pathname[strlen(temp_pathname) - 1] == '/')
        temp_pathname[strlen(temp_pathname) - 1] = '\0';

      // get modified time
      char time_buff[20];
      strftime(time_buff, sizeof(time_buff), "%Y-%m-%d %H:%M:%S", localtime(&file_stat.st_mtime));

      written = 0;
      if (S_ISREG(file_stat.st_mode)) // is a regular file so we will show teh size of the file
        written =
          snprintf(directory_browsing->pos, directory_browsing->size - directory_browsing->len,
                   "<tr><td><a href='%s/%s'>%s%s</a></td><td>%.2fKB</td><td>%s</td></tr>\n",
                   temp_pathname, dir_entry->d_name, dir_entry->d_name, is_dir,
                   (double)file_stat.st_size / 1024.0, time_buff);
      else // is a directory so we wont show the size
        written =
          snprintf(directory_browsing->pos, directory_browsing->size - directory_browsing->len,
                   "<tr><td><a href='%s/%s'>%s%s</a></td><td></td><td>%s</td></tr>\n",
                   temp_pathname, dir_entry->d_name, dir_entry->d_name, is_dir, time_buff);
      directory_browsing->pos += written;
      directory_browsing->len += written;
      free(temp_pathname);
    }
    close(file_fd);
  }
  closedir(dir);
  written = 0;
  written = snprintf(directory_browsing->pos, directory_browsing->size - directory_browsing->len,
                     "</table></body></html>");

  directory_browsing->pos += written;
  directory_browsing->len += written;

  return directory_browsing;
}