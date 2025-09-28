#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

#define SERVER_PORT 8080
#define BUFF_SIZE 10000
#define SERVER_IP "127.0.0.1"

int main(){

    int sockfd;
    char buffer[BUFF_SIZE],message[BUFF_SIZE];
    struct sockaddr_in server_addr;

    // Creating listening sock
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    //initialise buffer
    memset(buffer,0,sizeof(buffer));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(SERVER_PORT);

    while(1){

        // Get message from client terminal
        printf("Enter a string : ");
        fgets(message, BUFF_SIZE, stdin);/* read a line from the user using getline()*/

        sendto(sockfd, message,sizeof(message),0,(struct sockaddr *)&server_addr, sizeof(server_addr));

        size_t recv_n = recvfrom(sockfd,buffer,BUFF_SIZE,0,NULL, NULL);
        buffer[recv_n] = '\0';

        printf("[SERVER MESSAGE] %s", buffer);

    }

    close(sockfd);
    printf("[INFO] Disconnected\n");

    return 0;

}