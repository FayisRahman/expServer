#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define SERVER_PORT 8080
#define BUFF_SIZE 10000

void send_file(int client_sock_fd){

    // Open the file from the data is being read
    FILE *fp;
    const char *filename="t1.txt";
    fp = fopen(filename, "r");
    if (fp == NULL) {
        perror("[-]Error in opening file.");
        exit(1);
    }

    char data[BUFF_SIZE] = {0};
    printf("[INFO] Sending data to server...\n");

    while (fgets(data, BUFF_SIZE, fp) != NULL) {
        if (send(client_sock_fd,data,sizeof(data),0) == -1) {
            perror("[-]Error in sending data.");
            fclose(fp); // Ensure file is closed on error
            exit(1);
        }
        printf("[FILE DATA] %s", data);
        bzero(data, BUFF_SIZE); // clear the buffer
    }

    printf("[INFO] File data sent successfully.\n");
    fclose(fp); // Close the file after sending

}

int main(){

    int client_sock_fd = socket(AF_INET,SOCK_STREAM, 0);

    struct sockaddr_in server_addr;

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(SERVER_PORT);

    if(connect(client_sock_fd, (struct sockaddr *)&server_addr,sizeof(server_addr)) != 0){
        printf("[ERROR] Failed to connect to tcp server\n");
        exit(1);
    }else{
        printf("[INFO] Connected to server successfully\n");
    }

    send_file(client_sock_fd);
    
    close(client_sock_fd);

}