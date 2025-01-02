#include <stdio.h>
#include <string.h> // strerror()
#include <errno.h>
#include <stdlib.h> // exit()
#include <sys/socket.h> // sockets
#include <sys/types.h> // type macros
#include <netinet/in.h> // in_addr in6_addr sockaddr_in sockaddr_in6 
#include <netdb.h>  // gives struct addrinfo and getaddrinfo()
#include <arpa/inet.h> // for inet_ntop inet_ptons ptol etc network filling and conversions 


int main()
{
    struct addrinfo hints, *res;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(NULL, "5000", &hints, &res);

    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    if(connect(sockfd, res->ai_addr, res->ai_addrlen) != -1)
    {
        printf("connect unsucessfull\n");
    }
    else
    {
        printf("connection sucessfull\n");
    }

    char msg[50];

    int recvno = recv(sockfd,msg, sizeof(msg), 0);

    printf("msg received = %s\n number of bytes received = %d\n", msg, recvno);

    close(sockfd);
    free(res);
    return 0;
}