gcc -g -lz -fsanitize=address \
    main.c \
    lib/vec/vec.c lib/parson/parson.c \
    config/xps_config.c \
    core/xps_core.c core/xps_loop.c core/xps_pipe.c core/xps_session.c core/xps_timer.c core/xps_metrics.c\
    disk/xps_file.c disk/xps_mime.c disk/xps_directory.c disk/xps_gzip.c \
    http/xps_http.c http/xps_http_req.c http/xps_http_res.c \
    network/xps_connection.c network/xps_listener.c network/xps_upstream.c \
    utils/xps_logger.c utils/xps_utils.c utils/xps_buffer.c utils/xps_cliargs.c \
    -o xps
