#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/epoll.h> 

#define PORT 8080
#define BUFF_SIZE 10000
#define MAX_ACCEPT_BACKLOG 5
#define MAX_EPOLL_EVENTS 10
#define UPSTREAM_PORT 3000
#define MAX_SOCKS 10
#define SERVER_ADDR "127.0.0.1"
#define UPSTREAM_ADDR "127.0.0.1"

struct epoll_event events[MAX_EPOLL_EVENTS];
int listen_sock_fd,epoll_fd;
int route_table[MAX_SOCKS][2], route_table_size = 0;

enum {
    CLIENT_SOCK_FD,
    UPSTREAM_SOCK_FD,
};

// Function to reverse a string in-place
void strrev(char *str) {
    for (int start = 0, end = strlen(str) - 2; start < end; start++, end--) {
      char temp = str[start];
      str[start] = str[end];
      str[end] = temp;
    }
}

int find_pair(int fd,int flag){
    int idx = flag == CLIENT_SOCK_FD ? UPSTREAM_SOCK_FD : CLIENT_SOCK_FD;

    for(int i=0;i<route_table_size;i++){
        if(route_table[i][idx] == fd){
            return route_table[i][flag];

        }
    }

    return -1;
}

int get_fd_type(int fd) {
    for(int i=0;i<route_table_size;i++) {
        if(route_table[i][0] == fd) return CLIENT_SOCK_FD; //client
        if(route_table[i][1] == fd) return UPSTREAM_SOCK_FD; //upstream
    }
    return -1; //Not found.
}

int create_loop() {

    int epoll_fd = epoll_create1(0);
    if(epoll_fd < 0) {
        perror("epoll_create");
        exit(EXIT_FAILURE);
    }
    return epoll_fd;
}

void loop_attach(int epoll_fd, int fd, int events) {


    struct epoll_event event;

    event.events = events;
    event.data.fd = fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event)  < 0) {
        perror("epoll_ctl: loop attach ");
        exit(EXIT_FAILURE);
    }

}

void loop_detach(int epoll_fd,int fd){

    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL) < -1) {
        perror("epoll_ctl: loop detach");
        exit(EXIT_FAILURE);
    }

}

int create_server() {

    int listen_sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    // Setting sock opt reuse addr
    int enable = 1;
    setsockopt(listen_sock_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

    // Creating an object of struct socketaddr_in
    struct sockaddr_in server_addr;

    // Setting up server addr
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(PORT);

    //Binding the port to teh socket
    if(bind(listen_sock_fd,(struct sockaddr *)&server_addr,sizeof(server_addr)) < 0) {
        perror("Bind");
        exit(EXIT_FAILURE);
    }

    //listening through the socket
    if(listen(listen_sock_fd,MAX_ACCEPT_BACKLOG) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    printf("[INFO] Server listening on port %d\n", PORT);

    return listen_sock_fd;
}

int connect_upstream() {

    int upstream_sock_fd = socket(AF_INET, SOCK_STREAM, 0); /* create a upstrem socket */
    
    // Setting up upstream proxy addr
    struct sockaddr_in upstream_addr;
    upstream_addr.sin_family = AF_INET;
    upstream_addr.sin_addr.s_addr = inet_addr(UPSTREAM_ADDR);
    upstream_addr.sin_port = htons(UPSTREAM_PORT);
    
    /* connect to upstream server */
    if(connect(upstream_sock_fd,(struct sockaddr *)&upstream_addr,sizeof(upstream_addr)) < 0) {
        perror("[ERROR] Upstream Connection couldnt be established\n");
        close(upstream_sock_fd);
        return -1;
    }

    return upstream_sock_fd;
  
}

void accept_connection(int listen_sock_fd){

    // Creating an object of struct socketaddr_in
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    int conn_sock_fd = accept(listen_sock_fd, (struct sockaddr *)&client_addr, &client_addr_len);

    printf("[INFO] Client connected to server\n");

    loop_attach(epoll_fd,conn_sock_fd,EPOLLIN);

    // create connection to upstream server
    int upstream_sock_fd = connect_upstream();

    if(upstream_sock_fd == -1){
        close(conn_sock_fd);
        return;
    }

    loop_attach(epoll_fd,upstream_sock_fd,EPOLLIN);

    // add conn_sock_fd and upstream_sock_fd to routing table
    route_table[route_table_size][0] = conn_sock_fd;
    route_table[route_table_size][1] = upstream_sock_fd;
    route_table_size++;

}

void handle_client(int conn_sock_fd) {
    char buffer[BUFF_SIZE];
    memset(buffer,0,BUFF_SIZE);

    int read_n = recv(conn_sock_fd,buffer,sizeof(buffer),0);

    if(read_n <= 0) {
        if(read_n < 0){
            printf("[INFO] Closing connection (handle client)\n");
        }else{
            printf("[ERROR] Error encountered, Closing connection (handle client)\n");
        }
        loop_detach(epoll_fd,conn_sock_fd);
        close(conn_sock_fd);
        return;
    } 

    printf("[CLIENT MESSAGE] %s\n",buffer);

    int upstream_sock_fd = find_pair(conn_sock_fd,UPSTREAM_SOCK_FD); //find the right upstream socket from the route table 

    // sending client message to upstream
    int bytes_written = 0;
    int message_len = read_n;
    while(bytes_written < message_len) {
        int n = send(upstream_sock_fd,buffer + bytes_written,message_len - bytes_written,0);
        bytes_written += n;
    }
}

void handle_upstream(int upstream_sock_fd) {

    char buff[BUFF_SIZE];
    memset(buff,0,BUFF_SIZE);

    int read_n = recv(upstream_sock_fd,buff,sizeof(buff),0); /* read message from upstream to buffer using recv */
  
    // Upstream closed connection or error occurred
    if (read_n <= 0) {
        if(read_n < 0){
            printf("[ERROR] Upstream Connection Closed");
        }else{
            printf("[INFO] Upstream Connection Closed");
        }
        loop_detach(epoll_fd,upstream_sock_fd);
        close(upstream_sock_fd);
        
        return;
    }
  
    /* print client message (helpful for Milestone #2) */

    printf("[UPSTREAM MESSAGE] %s",buff);
  
    /* find the right client socket from the route table */

    int client_sock_fd = find_pair(upstream_sock_fd,CLIENT_SOCK_FD);

    if(client_sock_fd == -1){
        printf("[ERROR] Routing Table Not Set for this Client.");
        printf("[INFO] Client Disconnected");
        close(upstream_sock_fd);
        loop_detach(epoll_fd,upstream_sock_fd);
        return;
    }

    // sending upstream message to client
    int bytes_written = 0;
    int message_len = read_n;
    while(bytes_written < message_len) {
        int n = send(client_sock_fd,buff + bytes_written,message_len - bytes_written,0);
        bytes_written += n;
    }
  
}

void loop_run(int epoll_fd) {

    /* infinite loop and processing epoll events */

    while(1){

        printf("[DEBUG] Epoll wait\n");

        //wait for any events to happen
        int n_ready_fds = epoll_wait(epoll_fd, events, MAX_EPOLL_EVENTS, -1);

        /* interate from 0 to n_ready_fds */
        for(int i=0;i<n_ready_fds;i++) {

            int curr_fd = events[i].data.fd;

            
            if(curr_fd == listen_sock_fd){
                
                /* accept client connection and add to epoll */
                accept_connection(listen_sock_fd);
                continue;

            }else{

                int fd_type = get_fd_type(curr_fd);

                if(fd_type == CLIENT_SOCK_FD){ //CLIENT
                    printf("[INFO] CLIENT CONNECTION DETECTED\n");
                    handle_client(curr_fd);
                }else if(fd_type == UPSTREAM_SOCK_FD){ //UPSTREAM
                    printf("[INFO] UPSTREAM CONNECTION DETECTED\n");
                    handle_upstream(curr_fd);
                }
            }
        }   
        
    }
}  

int main(){

    // Creating listening sock
    listen_sock_fd = create_server();

    epoll_fd = create_loop();

    loop_attach(epoll_fd,listen_sock_fd,EPOLLIN);

    loop_run(epoll_fd);

    return 0;
}