#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

int main()
{

    struct sockaddr_in *serverAddr;
    serverAddr->sin_family = AF_INET;
    serverAddr->sin_port = htons(5000);

    memset(&(serverAddr->sin_addr), '\0',sizeof(serverAddr->sin_zero));

    inet_pton(AF_INET,"192.168.177.128", &(serverAddr->sin_addr));

    char servIp[INET_ADDRSTRLEN];

    printf("server address is: %s\n", inet_ntop(AF_INET, &(serverAddr->sin_addr), servIp, INET_ADDRSTRLEN));


    int clientSock = socket(PF_INET, SOCK_STREAM, 0);

    if(clientSock <0)
    {
        printf("client socket creation failed\n");
        exit(-1);
    }

    connect(clientSock, (struct sockaddr*)serverAddr, INET_ADDRSTRLEN);

    printf("connected to server\n");

    char msg[40];

    int bytesRead = recv(clientSock, msg, sizeof(msg), 0);

    printf("server sent msg: %s\n number of bytes: %d\n",msg, bytesRead);

    return 0;
}