#include "xps_mime.h"

// here, some extension-mime types pairs are given, you can add as required
xps_keyval_t mime_types[] = {
  {".c", "text/x-c"},
  {".h", "text/x-c"},
  {".cc", "text/x-c"},
  {".cpp", "text/x-c"},
  {".dir", "application/x-director"},
  {".dxr", "application/x-director"},
  {".fgd", "application/x-director"},
  {".swa", "application/x-director"},
  {".text", "text/plain"},
  {".txt", "text/plain"},
  {".png", "image/png"},
  {".png", "image/x-png"},
  {".gz", "application/x-gzip"},
  {".tgz", "application/x-gzip"},
  {".gz", "application/gzip"},
  {".tgz", "application/gzip"},
};
int n_mimes = sizeof(mime_types) / sizeof(mime_types[0]);

const char *xps_get_mime(const char *file_path) {
  const char *ext = get_file_ext(file_path);

  if (ext == NULL)
    return NULL;

  for (int i = 0; i < n_mimes; i++) {
    if (strcmp(mime_types[i].key, ext) == 0)
      return mime_types[i].val;
  }

  return NULL;
}