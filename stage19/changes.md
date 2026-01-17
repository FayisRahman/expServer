# Stage 19: GZIP Compression

In this stage, we added GZIP compression support to reduce response sizes for text-based content.

## Dependencies

- **zlib** - Compression library for GZIP encoding
  - Header: `<zlib.h>`
  - Install: `sudo apt install zlib1g-dev` (Debian/Ubuntu)

## Build

Add the `-lz` flag to link the zlib library:

```bash
gcc -g -lz -fsanitize=address \
    main.c \
    ... \
    xps_config.c \
    ... \
    -o xps
```

> **Note**: The `-lz` flag must be included or you'll get linker errors for zlib functions like `deflateInit2`, `deflate`, `deflateEnd`.

## New Files

- **`disk/xps_gzip.c`** - GZIP compression module using zlib
- **`disk/xps_gzip.h`** - Header file for gzip module

## Modified Files

### `config/xps_config.h`
- Added fields to `xps_config_route_s`:
  - `gzip_enable` (bool)
  - `gzip_level` (int, -1 to 9)
  - `gzip_mime_types` (vec_void_t)
- Added same fields to `xps_config_lookup_s`

### `config/xps_config.c`
- Added `default_gzip_mimes[]` array with common compressible MIME types
- Added `vec_init(&route->gzip_mime_types)` in `parse_server()`
- Parse gzip settings in `parse_route()`: `gzip_enable`, `gzip_level`, `gzip_mime_types`
- In `xps_config_lookup()`: check if file's MIME type is in default or custom gzip MIME list

### `core/xps_session.h`
- Added `xps_gzip_t *gzip` field to session struct

### `core/xps_session.c`
- Set `Content-Encoding: gzip` header when gzip is enabled (instead of `Content-Length`)
- Create gzip pipe chain: `file->source → gzip->sink` and `gzip->source → session->file_sink`

### `disk/xps_mime.c`
- Added `.h` extension mapping to `text/x-c` MIME type

## Config Example

```json
{
  "gzip_enable": true,
  "gzip_level": 8,
  "gzip_mime_types": ["text/x-c"]
}
```

## How It Works

1. Client sends `Accept-Encoding: gzip` header
2. Server checks if file's MIME type is compressible
3. If enabled, data flows: `File → Gzip Module → Client`
4. Response includes `Content-Encoding: gzip` header
5. Browser automatically decompresses