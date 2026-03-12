#include "xps_connection.h"

xps_connection_t *xps_connection_create(int epoll_fd, int sock_fd) {

    xps_connection_t *connection = malloc(sizeof(xps_connection_t));
    if (connection == NULL) {
      logger(LOG_ERROR, "xps_connection_create()", "malloc() failed for 'connection'");
      return NULL;
    }
  
    /* attach sock_fd to epoll */
    xps_loop_attach(epoll_fd,sock_fd,EPOLLIN);
  
    // Init values
    connection->epoll_fd = epoll_fd;
    connection->sock_fd = sock_fd;
    connection->listener = NULL;
    connection->remote_ip = get_remote_ip(sock_fd);
  
    /* add connection to 'connections' list */
    vec_push(&connections,connection);
  
    logger(LOG_DEBUG, "xps_connection_create()", "created connection");
    return connection;
  
  }
  
  void xps_connection_destroy(xps_connection_t *connection) {
  
    /* validate params */
    assert(connection != NULL);
  
    /* set connection to NULL in 'connections' list */
    for (int i = 0; i < connections.length; i++) {
        xps_connection_t *curr = connections.data[i];
        if (curr == connection) {
          connections.data[i] = NULL;
          break;
        }
      }
    
  
    /* detach connection from loop */
    xps_loop_detach(connection->epoll_fd,connection->sock_fd);
  
    /* close connection socket FD */
    close(connection->sock_fd);
  
    /* free connection->remote_ip */
    free(connection->remote_ip);
  
    /* free connection instance */
    free(connection);
  
    logger(LOG_DEBUG, "xps_connection_destroy()", "destroyed connection");
  
}

// Function to reverse a string in-place
void connection_strrev(char *str) {
    for (int start = 0, end = strlen(str) - 2; start < end; start++, end--) {
      char temp = str[start];
      str[start] = str[end];
      str[end] = temp;
    }
}


void xps_connection_read_handler(xps_connection_t *connection) {

    /* validate params */
    assert(connection != NULL);
    
    u_char buff[DEFAULT_BUFFER_SIZE];
    memset(buff,0,sizeof(buff));

    /* read data from client using recv() */
    long read_n = recv(connection->sock_fd,buff,sizeof(buff),0);
  
    if (read_n < 0) {
      logger(LOG_ERROR, "xps_connection_read_handler()", "recv() failed");
      perror("Error message");
      xps_connection_destroy(connection);
      return;
    }
  
    if (read_n == 0) {
      logger(LOG_INFO, "connection_read_handler()", "peer closed connection");
      xps_connection_destroy(connection);
      return;
    }
  
    buff[read_n] = '\0';
  
    /* print client message */
    printf("[CLIENT MESSAGE] %s", buff);
  
    /* reverse client message */
    connection_strrev(buff);
  
    // Sending reversed message to client
    long bytes_written = 0;
    long message_len = read_n;
    while (bytes_written < message_len) {

        /* send message using send() */ 
        long write_n = send(connection->sock_fd,buff + bytes_written, message_len - bytes_written, 0); 
        if (write_n < 0) {
            logger(LOG_ERROR, "xps_connection_read_handler()", "send() failed");
            perror("Error message");
            xps_connection_destroy(connection);
            return;
        }
        bytes_written += write_n;
    }
  
}

