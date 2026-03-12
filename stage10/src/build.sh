gcc -g \
    main.c \
    lib/vec/vec.c \
    core/xps_core.c core/xps_loop.c core/xps_pipe.c \
    network/xps_connection.c network/xps_listener.c \
    utils/xps_logger.c utils/xps_utils.c utils/xps_buffer.c -o xps