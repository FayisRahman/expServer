#include "xps_gzip.h"

void gzip_source_handler(void *ptr);
void gzip_source_close_handler(void *ptr);
void gzip_sink_handler(void *ptr);
void gzip_sink_close_handler(void *ptr);
void gzip_compress(xps_gzip_t *gzip, bool flush);
void gzip_check_destroy(xps_gzip_t *gzip);

xps_gzip_t *xps_gzip_create(int level){

  //validate compression level
  if( level < -1 || level > 9){
    logger(LOG_ERROR, "xps_gzip_create()", "invalid compression level %d", level);
    return NULL;
  }

  xps_gzip_t *gzip = malloc(sizeof(xps_gzip_t));

  gzip->stream.zalloc = Z_NULL;
  gzip->stream.zfree = Z_NULL;
  gzip->stream.opaque = Z_NULL;

  //creating source for output endpoint
  gzip->source = xps_pipe_source_create((void *)gzip, 
                                         gzip_source_handler, 
                                         gzip_source_close_handler);
  if (gzip->source == NULL) {
    logger(LOG_ERROR, "xps_gzip_create()", "xps_pipe_source_create() failed");
    free(gzip);
    return NULL;
  }

  // Create sink (input endpoint)
  gzip->sink = xps_pipe_sink_create((void *)gzip, 
                                     gzip_sink_handler, 
                                     gzip_sink_close_handler);
  if (gzip->sink == NULL) {
    logger(LOG_ERROR, "xps_gzip_create()", "xps_pipe_sink_create() failed");
    xps_pipe_source_destroy(gzip->source);
    free(gzip);
    return NULL;
  }

  // Initialize the deflate (compression) stream
  // deflateInit2 arguments:
  //   stream     - pointer to z_stream
  //   level      - compression level (0-9)
  //   method     - Z_DEFLATED (only option)
  //   windowBits - 16 + MAX_WBITS for gzip format
  //   memLevel   - memory usage (8 is default)
  //   strategy   - Z_DEFAULT_STRATEGY
  if(deflateInit2(&(gzip->stream), level, Z_DEFLATED, 16 + MAX_WBITS, 8, Z_DEFAULT_STRATEGY) != Z_OK){
    logger(LOG_ERROR, "xps_gzip_create()", "deflateInit2() failed");
    xps_pipe_source_destroy(gzip->source);
    xps_pipe_sink_destroy(gzip->sink);
    free(gzip);
    return NULL;
  }

  // Initialize state
  gzip->sink->ready = true;     // Ready to receive input
  gzip->transfer_buff = NULL;   // No pending output
  gzip->total_in_len = 0;
  gzip->total_out_len = 0;

  logger(LOG_DEBUG, "xps_gzip_create()", "created gzip");

  return gzip;

}

void xps_gzip_destroy(xps_gzip_t *gzip) {
  assert(gzip != NULL);

  deflateEnd(&(gzip->stream));             // Free zlib internal state
  xps_pipe_source_destroy(gzip->source);   // Destroy output pipe
  xps_pipe_sink_destroy(gzip->sink);       // Destroy input pipe
  
  if (gzip->transfer_buff)
    xps_buffer_destroy(gzip->transfer_buff);  // Free pending data
    
  free(gzip);
  logger(LOG_DEBUG, "xps_gzip_destroy()", "destroyed gzip");
}

void gzip_source_handler(void *ptr){
  assert(ptr != NULL);

  xps_pipe_source_t *source = ptr;
  xps_gzip_t *gzip = source->ptr;

  if (xps_pipe_source_write(gzip->source, gzip->transfer_buff) != OK) {
    logger(LOG_ERROR, "gzip_source_handler()", "xps_pipe_source_write() failed");
    return;
  }

  //update length
  gzip->total_in_len += gzip->transfer_buff->len;

  // Clean up the sent buffer
  xps_buffer_destroy(gzip->transfer_buff);

  //reset state for next round
  gzip->transfer_buff = NULL;
  gzip->source->ready = false;  // Nothing to send now
  gzip->sink->ready = true;     // Ready for more input

  gzip_check_destroy(gzip);

}

void gzip_source_close_handler(void *ptr){
  assert(ptr != NULL);

  xps_pipe_source_t *source = ptr;
  xps_gzip_t *gzip = source->ptr;

  gzip_check_destroy(gzip);

}


void gzip_sink_handler(void *ptr){
  assert(ptr != NULL);

  xps_pipe_sink_t *sink = ptr;
  xps_gzip_t *gzip = sink->ptr;

  // Compress with flush=false (more data may come) 
  gzip_compress(gzip, false);
}

void gzip_sink_close_handler(void *ptr) {
  assert(ptr != NULL);

  xps_pipe_sink_t *sink = ptr;
  xps_gzip_t *gzip = sink->ptr;

  // Compress with flush=true (finalize the stream)
  gzip_compress(gzip, true);
}


void gzip_compress(xps_gzip_t *gzip, bool flush){
  assert(gzip != NULL);

  // ---STEP 1:  Get Input Data---
  xps_buffer_t *in_buff = NULL;
  if(!flush){
    // Normal: read from the sink pipe
    in_buff = xps_pipe_sink_read(gzip->sink, gzip->sink->pipe->buff_list->len);
    if(in_buff == NULL){
      logger(LOG_ERROR, "gzip_compress()", "xps_pipe_sink_read() failed");
      return;
    }
  }else{
    // Flush: create empty buffer (just to finalize)
    in_buff = xps_buffer_create(1, 0, NULL);
    if (in_buff == NULL) {
      logger(LOG_ERROR, "gzip_compress()", "xps_buffer_create() failed");
      return;
    }
  }

  // step 2: prepare output buffer list
  xps_buffer_list_t *out_buff_list = xps_buffer_list_create();
  if (out_buff_list == NULL) {
    xps_buffer_destroy(in_buff);
    logger(LOG_ERROR, "gzip_compress()", "xps_buffer_list_create() failed");
    return;
  }

  //step 3: set up zlib input
  gzip->stream.avail_in = in_buff->len;   // How many bytes to compress
  gzip->stream.next_in = in_buff->data;   // Pointer to input data

  //choose flush mode
  int flush_stream = flush ? Z_FINISH : Z_NO_FLUSH;

  //step 4: Compression loop
  //may need multiple iteration if compressed output is large
  do {

    //create a buffer for compressed output
    xps_buffer_t *out_buff = xps_buffer_create(DEFAULT_BUFFER_SIZE, 0, NULL);
    if (out_buff == NULL) {
      logger(LOG_ERROR, "gzip_compress()", "xps_buffer_create() failed");
      xps_buffer_list_destroy(out_buff_list);
      xps_buffer_destroy(in_buff);
      return;
    }

    // Tell zlib where to write output
    gzip->stream.avail_out = out_buff->size;
    gzip->stream.next_out = out_buff->data;

    // *** DO THE COMPRESSION ***
    int error = deflate(&(gzip->stream), flush_stream);
    assert(error != Z_STREAM_ERROR);  // Should never happen

    // Calculate how many bytes were written
    out_buff->len = out_buff->size - gzip->stream.avail_out;

    // Keep or discard buffer
    if (out_buff->len > 0)
      xps_buffer_list_append(out_buff_list, out_buff);
    else
      xps_buffer_destroy(out_buff);

  } while (gzip->stream.avail_out == 0);

  // Verify all input was consumed
  assert(gzip->stream.avail_in == 0);

  //step 5: clear processed data
  if(!flush){
    xps_pipe_sink_clear(gzip->sink, gzip->sink->pipe->buff_list->len);
    gzip->total_in_len += in_buff->len;
  }

  //step 6: compine all output buffers
  xps_buffer_t *compressed_buff = NULL;
  if (out_buff_list->len > 0)
    compressed_buff = xps_buffer_list_read(out_buff_list, out_buff_list->len);

  // Cleanup
  xps_buffer_list_destroy(out_buff_list);
  xps_buffer_destroy(in_buff);

  // --- STEP 7: Set up for sending ---
  gzip->transfer_buff = compressed_buff;
  gzip->source->ready = (compressed_buff != NULL);
  gzip->sink->ready = (compressed_buff == NULL);

}

void gzip_check_destroy(xps_gzip_t *gzip){
  assert(gzip != NULL);

  // Destroy if:
  // - downstream is closed, OR
  // - upstream is closed AND no pending data

  if(!gzip->source->active || (!gzip->sink->active && gzip->transfer_buff == NULL)){
    xps_gzip_destroy(gzip);
  }
}