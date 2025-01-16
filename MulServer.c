#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <pthread.h>
#include <semaphore.h>

#define Len 10

typedef struct
{
    int clientSock;
    char *msg;
    int len;
}clientDet;

clientDet clients[10];
sem_t semaphore;

void *clientResponse(void *args)
{
    clientDet *client = (clientDet *)args; 
    int i=0;
    while(1)
    {
        sem_wait(&semaphore);
        int sentNo = send(client[i].clientSock,client[i].msg, sizeof(char)*(client[i].len), 0);

        printf("message sent = %s\n",client[i].msg);
        printf("number of bytes sent= %d\n\n",sentNo);

        if(i==Len)
        break;

        i++;
    }
    pthread_exit(NULL);
}

int main()
{

    struct addrinfo hints, *servinfo;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int result = getaddrinfo(NULL, "5000", &hints, &servinfo);

    //check result and print server details
    if(result ==0)
    {
        for(struct addrinfo *p=servinfo; p!=NULL; p=p->ai_next)
        {
            char ip[INET_ADDRSTRLEN];
            if(p->ai_family == AF_INET)
            {
                struct sockaddr_in *addr4 = (struct sockaddr_in*)p->ai_addr;
                inet_ntop(AF_INET, &(addr4->sin_addr), ip, INET6_ADDRSTRLEN);
                printf("IPv4 address: %s\n", ip);
            }
            else if(p->ai_family == AF_INET6)
            {
                struct sockaddr_in6 *addr6 = (struct sockaddr_in6*)p->ai_addr;
                inet_ntop(AF_INET6, &(addr6->sin6_addr), ip, INET6_ADDRSTRLEN);
                printf("IPv6 address: %s\n", ip);
            }
        }
    }
    else
    {
        printf("getaddrinfo error!\n");
        return -1;
    }

    //server socket
    int servSock = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    
    //initialize semaphore
    sem_init(&semaphore, 0, 0);

    //initialize thread
    pthread_t resThread;
    pthread_create(&resThread, NULL, clientResponse, (void *)clients);

    //bind to first address
    for(struct addrinfo *p=servinfo; p!=NULL; p=p->ai_next)
    {
        result = bind(servSock, p->ai_addr, p->ai_addrlen);
        if(result ==0)
        {
            printf("bind sucess\n");
        }
    }

    //start listening at the address
    result = listen(servSock,10);
    if(result ==0)
    printf("listening for connections...\n");

    int i=0;
    int clientSock;
    
FILE *htmlFile;
    htmlFile = fopen("Response.html", "r");
    if (htmlFile == NULL) {
        perror("Error opening file");
        return 1;
    }

    int size = 0;
    char *response;

    fseek(htmlFile, 0, SEEK_END);
    size = ftell(htmlFile);
    rewind(htmlFile);

    char *buff = (char *)calloc(size + 1, sizeof(char));
    if (buff == NULL) {
        perror("Memory allocation failed");
        fclose(htmlFile);
        return 1;
    }

    fread(buff, 1, size, htmlFile);
    buff[size] = '\0';
    fclose(htmlFile);

    char *httpProtocol = (char*)calloc(100,sizeof(char));
    sprintf(httpProtocol, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %d\r\n\r\n", size);

    response = (char *)calloc(strlen(httpProtocol) + size + 1, sizeof(char));
    if (response == NULL) {
        perror("Memory allocation failed");
        free(buff);
        return 1;
    }

    strcpy(response, httpProtocol);
    strcat(response, buff);

    free(httpProtocol);
    free(buff);

    char *msg = response;

    while(1)
    {
        struct sockaddr *clientAddr;
        int *addrlen = (int*)malloc(sizeof(int));
        *addrlen = 0;
        
        clientSock = accept(servSock, clientAddr, addrlen);
        
        clientDet currClient = {clientSock, msg, strlen(msg)};    
        clients[i] = currClient;
        
        if(i==Len-1)
        {
            printf("max clients are served!\n");
            pthread_cancel(resThread);
            return 0;
        }
        i++;

        sem_post(&semaphore);
    }

    pthread_join(resThread, NULL);


    return 0;
}