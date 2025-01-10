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
    // stores address of info of a device or web etc
    struct addrinfo hints;
    hints.ai_family = AF_UNSPEC; // takes what ever ip version ipv4 or ipv6 
    hints.ai_socktype = SOCK_STREAM; // tcp socket
    hints.ai_flags = AI_PASSIVE; // filles our ip address

    struct addrinfo *servinfo;
    int status;
    //NULL means our device ip also can be given as "www.xyz.com",
    // 5000 is port, hints is our given info,
    // servinfo is all stuff filled version return
    if((status = getaddrinfo(NULL, "5000", &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddinfo error: %s\n", gai_strerror(status));
        exit(-1);
    }

    // lets print all the server details in linked list
    for(struct addrinfo * PL = servinfo; PL != NULL; PL = PL->ai_next)
    {
        //ipv6 length can store both ipv6 and ipv4
        char ALLtypeAddr[INET6_ADDRSTRLEN];
        if(PL->ai_family == AF_INET)
        {
            struct sockaddr_in *addr4 = (struct sockaddr_in*)PL->ai_addr;
            inet_ntop(AF_INET, &(addr4->sin_addr), ALLtypeAddr, INET6_ADDRSTRLEN);
            printf("IPv4 address: %s\n", ALLtypeAddr);
        }
        else
        {
            struct sockaddr_in6 *addr6 = (struct sockaddr_in6*)PL->ai_addr;
            inet_ntop(AF_INET6, &(addr6->sin6_addr), ALLtypeAddr, INET6_ADDRSTRLEN);
            printf("IPv6 address: %s\n", ALLtypeAddr);
        }
    }

    /*struct sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_port = htons(80);
    memset(&sa.sin_zero,0,sizeof(sa.sin_zero));
    
    if( inet_pton(AF_INET, "10.12.110.57", &(sa.sin_addr)) < 0)
    {
        printf("ipv4 presentation to netwok unsuccesfull\n");
        exit(-1);
    }
    struct sockaddr_in6 sa6;
    sa6.sin6_family = AF_INET6;
    sa6.sin6_port = htons(80);

    if( inet_pton(AF_INET6, "2001:db8:63b3:1::3490", &(sa6.sin6_addr)) < 0)
        printf("ipv6 presentation to network unsuccesfull\n");

    //printf("size of char = %d",sizeof(char));


    char sa_p[INET_ADDRSTRLEN];
    char sa6_p[INET6_ADDRSTRLEN];

    inet_ntop(AF_INET,&(sa.sin_addr), sa_p, INET_ADDRSTRLEN);
    printf("presentation of ipv4 = %s\n",sa_p);

    inet_ntop(AF_INET6,&(sa6.sin6_addr), sa6_p, INET6_ADDRSTRLEN);

    printf("presentation of ipv6 = %s\n",sa6_p);*/

    // socket is created with info about the type of portocol only and returns the file descriptor
    int sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

    // bind for servers to reserve port and listen for connections 
    // now bind will associate or add the socket the ip info and reserves the port 
    if(bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1)
    {
        printf("bind error: %s\n", strerror(errno));
        exit(-2);
    }
    else 
    {
        printf("bind success\n");
    }

    // gives the cliend address after accept
    struct sockaddr_storage clientAddr;
    //size of client address intialized might be changed
    socklen_t clientAddr_size = sizeof(clientAddr);

    // listen from out socket and queue for incoming connections is set to be 10
    listen(sockfd, 10);

    // accept a connection from queue and return new socket and client details
    int client_sockfd = accept(sockfd, (struct sockaddr *)&clientAddr, &clientAddr_size);

    char msg[] = "hello client";
    int sentno = 0;
    
    /*display the connected client details*/
    {
        char clientIp[16];
        inet_ntop(AF_INET6, &((struct sockaddr_in6*)&clientAddr)->sin6_addr, clientIp,INET6_ADDRSTRLEN);
        printf("client Ipv4 address= %s\n",clientIp);

        inet_ntop(AF_INET, &((struct sockaddr_in*)&clientAddr)->sin_addr, clientIp,INET_ADDRSTRLEN);
        printf("client Ipv6 address= %s\n",clientIp);
    }
    
    // send the message and returns the number of bytes sent
    sentno = send(client_sockfd, msg, sizeof(msg), 0);
    
    printf("sent the msg = %s\n number of bytes sent = %d\n", msg, sentno);

    close(client_sockfd);
    close(sockfd);
    free(servinfo);

    return 0;
}