#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080
#define BUFF_SIZE 10000
#define MAX_ACCEPT_BACKLOG 5

void reverseString(char * str) {
    int n = strlen(str);
    for(int i = 0; i < (n/2); i++) {
        int temp = str[i];
        str[i] = str[n-i-1];
        str[n-1-i] = temp ;
    }
}

void write_to_file(int conn_sock_fd){
    char buffer[BUFF_SIZE];
    ssize_t bytes_received;

    // Open the file to which the data from the client is being written
    FILE *fp;
    const char *filename= "t2.txt";
    fp = fopen(filename, "w");
    if (fp == NULL) {
        perror("[-]Error in creating file");
        exit(EXIT_FAILURE);
    }


    printf("[INFO] Receiving data from client...\n");
    while ((bytes_received = recv(conn_sock_fd,buffer,sizeof(buffer),0)) > 0) {
        printf("[FILE DATA] %s", buffer); // Print received data to the console
        fprintf(fp, "%s", buffer);      // Write data to file
        memset(buffer,0,sizeof(buffer));   // Clear the buffer
    }

    if (bytes_received < 0) {
        perror("[-]Error in receiving data");
    }

    fclose(fp);
    printf("[INFO] Data written to file successfully.\n");

}

int main() {
    int listen_sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    //To be able to reuse this socket we SO_REUSEADDR even if it goes to TIME_WAIT state.
    //Setting sock opt reuse addr
    int enable = 1;
    setsockopt(listen_sock_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

    // Creating an object of struct socketaddr_in
    struct sockaddr_in server_addr;

    // Setting up server addr
    server_addr.sin_family = AF_INET;                   //Uses IpV4
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);    //used to convert the IP from host byte order to network byte order
    server_addr.sin_port = htons(PORT);                 //Used to convert port number from host byte order to network byte order

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

    // Creating an object of struct socketaddr_in
    struct sockaddr_in client_addr;
    socklen_t client_addr_len;

    // Accept client connection
    int conn_sock_fd = accept(listen_sock_fd, (struct sockaddr *)&client_addr, &client_addr_len);
    printf("[INFO] Client connected to server\n");

    
    write_to_file(conn_sock_fd);

    close(conn_sock_fd);


}