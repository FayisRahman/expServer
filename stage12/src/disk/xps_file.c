#include "xps_file.h"

void file_source_handler(void *ptr);
void file_source_close_handler(void *ptr);

xps_file_t *xps_file_create(xps_core_t *core, const char *file_path, int *error) {
  /*assert*/
  assert(core != NULL);
  assert(file_path != NULL);

  *error = E_FAIL;

  /*check if file is inside the public directory*/
  char *resolved_path = realpath(file_path, NULL);
  char *resolved_public = realpath("../public", NULL);

  if (resolved_path == NULL || resolved_public == NULL) {
    logger(LOG_ERROR, "xps_file_create()", "realpath() failed");
    /*free both path*/
    if (resolved_path != NULL)
      free(resolved_path);
    if (resolved_public != NULL)
      free(resolved_public);

    perror("Error message");

    return NULL;
  }

  size_t public_len = strlen(resolved_public);
  if (strncmp(resolved_path, resolved_public, public_len) != 0) {
    logger(LOG_WARNING, "xps_file_create()", "file is not inside the public directory");
    *error = E_PERMISSION;
    /*free both path*/
    free(resolved_path);
    free(resolved_public);
    return NULL;
  }

  /*free both path*/
  free(resolved_path);
  free(resolved_public);

  /*check if others have read permission*/
  struct stat file_stat;
  if (stat(file_path, &file_stat) != 0) {
    logger(LOG_ERROR, "xps_file_create()", "stat() failed");
    perror("Error message");
    return NULL;
  }

  if (!(file_stat.st_mode & S_IROTH)) {
    logger(LOG_WARNING, "xps_file_create()", "others do not have read permission");
    *error = E_PERMISSION;
    return NULL;
  }

  // Getting size of file from stat (already called above)
  long temp_size = file_stat.st_size;

  // Opening file
  FILE *file_struct = fopen(file_path, "rb");
  /*handle EACCES,ENOENT or any other error*/
  if (file_struct == NULL) {
    if (error != NULL) {
      if (errno == EACCES) {
        logger(LOG_WARNING, "xps_file_create()", "fopen() failed. permission denied");
        *error = E_PERMISSION;
      } else if (errno == ENOENT) {
        logger(LOG_WARNING, "xps_file_create()", "fopen() failed. file not found");
        *error = E_NOTFOUND;
      } else {
        logger(LOG_ERROR, "xps_file_create()", "fopen() failed");
        perror("Error message");
        *error = E_FAIL;
      }
    }
    return NULL;
  }

  const char *mime_type = xps_get_mime(file_path);

  /*Alloc memory for instance of xps_file_t*/

  xps_file_t *file = malloc(sizeof(xps_file_t));

  xps_pipe_source_t *source =
    xps_pipe_source_create((void *)file, file_source_handler, file_source_close_handler);

  /*if source is null, close file_struct and return*/
  if (source == NULL) {
    logger(LOG_ERROR, "xps_file_create()", "xps_pipe_source_create() failed");
    perror("Error message");
    fclose(file_struct);
    return NULL;
  }

  // Init values
  source->ready = true;
  /*initialise the fields of file instance*/
  file->core = core;
  file->file_path = file_path;
  file->source = source;
  file->file_struct = file_struct;
  file->size = temp_size;
  file->mime_type = mime_type;

  *error = OK;

  logger(LOG_DEBUG, "xps_file_create()", "created file");

  return file;
}

void xps_file_destroy(xps_file_t *file) {

  assert(file != NULL);

  /*fill as mentioned above*/
  fclose(file->file_struct);
  xps_pipe_source_destroy(file->source);
  free(file);

  logger(LOG_DEBUG, "xps_file_destroy()", "destroyed file");
}

void file_source_handler(void *ptr) {
  /*assert*/
  assert(ptr != NULL);

  xps_pipe_source_t *source = ptr;
  /*get file from source ptr*/
  xps_file_t *file = source->ptr;

  /*create buffer and handle any error*/
  xps_buffer_t *buff = xps_buffer_create(file->size, file->size, NULL);
  if (buff == NULL) {
    logger(LOG_ERROR, "file_source_handler()", "xps_buffer_create() failed");
    perror("Error message");
    return;
  }

  // Read from file
  size_t read_n = fread(buff->data, 1, buff->size, file->file_struct);
  buff->len = read_n;

  // Checking for read errors
  if (ferror(file->file_struct)) {
    logger(LOG_ERROR, "file_source_handler()", "fread() failed");
    perror("Error message");
    /*destroy buff, file and return*/
    xps_buffer_destroy(buff);
    xps_file_destroy(file);
    return;
  }

  // If end of file reached
  if (read_n == 0 && feof(file->file_struct)) {
    /*destroy buff, file and return*/
    xps_buffer_destroy(buff);
    xps_file_destroy(file);
    return;
  }

  /*Write to pipe form buff*/
  if (xps_pipe_source_write(source, buff) != OK) {
    logger(LOG_ERROR, "file_source_handler()", "xps_pipe_source_write() failed");
    xps_buffer_destroy(buff);
    xps_file_destroy(file);
    return;
  }
  /*destroy buff*/
  xps_buffer_destroy(buff);
}

void file_source_close_handler(void *ptr) {
  /*assert*/
  xps_pipe_source_t *source = ptr;
  /*get file from source ptr*/
  xps_file_t *file = source->ptr;
  /*destroy file*/
  xps_file_destroy(file);
}