#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>

#define PORT "5500"

char *defaultfile = "sendFile.txt";

// Function to get the response file
char* get_responseFile(char *filename) {
    printf("file name: %s\n", filename);

    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Error opening file");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);

    char *buff = (char *)calloc(size + 1, sizeof(char));
    if (buff == NULL) {
        perror("Memory allocation failed");
        fclose(file);
        return NULL;
    }

    fread(buff, 1, size, file);
    buff[size] = '\0';
    fclose(file);

    return buff;
}

// Function to send the response file to client
int sendresponseFile(int client_sock, char *filename) {
    char *response = get_responseFile(filename);
    if (response == NULL) {
        const char *error_msg = "file doesn't exist";
        send(client_sock, error_msg, strlen(error_msg), 0);
        const char *eof_msg = "__EOF__";
        send(client_sock, eof_msg, strlen(eof_msg), 0);
        return -1;
    }

    long totalSent = 0;
    long responseLen = strlen(response);

    while (totalSent < responseLen) {
        long sentNo = send(client_sock, response + totalSent, responseLen - totalSent, 0);
        if (sentNo == -1) {
            perror("send error");
            free(response);
            return -1;
        }
        totalSent += sentNo;
    }

    const char *eof_msg = "__EOF__";
    send(client_sock, eof_msg, strlen(eof_msg), 0);

    free(response);
    return totalSent;
}

// Function to set up the server
int serverSetup(char *port) {
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo *servinfo;
    int status;
    if ((status = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(-1);
    }

    int sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if (sockfd == -1) {
        perror("socket creation failed");
        freeaddrinfo(servinfo);
        exit(-2);
    }

    if (bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
        perror("bind failed");
        close(sockfd);
        freeaddrinfo(servinfo);
        exit(-3);
    }

    printf("bind success\n");
    freeaddrinfo(servinfo);

    return sockfd;
}

// will send a default file(sendFile.txt) else send the stated file
int main(int argc, char **argv) {
    int servsock = serverSetup(PORT);

    if (listen(servsock, 5) == -1) {
        perror("listen");
        exit(-4);
    }

    struct sockaddr_in clientaddr;
    socklen_t clientsock_len = sizeof(clientaddr);

    int clientsock = accept(servsock, (struct sockaddr *)&clientaddr, &clientsock_len);
    if (clientsock == -1) {
        perror("accept");
        close(servsock);
        exit(-5);
    }

    if(argc == 2)
    {
        defaultfile = argv[1];
    }
    
    printf("sending file: %s\n",defaultfile);
    sendresponseFile(clientsock, defaultfile);

    close(clientsock);
    close(servsock);

    return 0;
}