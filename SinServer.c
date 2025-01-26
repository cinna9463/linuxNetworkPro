#include <stdio.h>
#include <string.h>    // strerror()
#include <errno.h>
#include <unistd.h>    // close()
#include <stdlib.h>    // exit()
#include <sys/socket.h> // sockets
#include <sys/types.h> // type macros
#include <netinet/in.h> // in_addr in6_addr sockaddr_in sockaddr_in6 
#include <netdb.h>  // gives struct addrinfo and getaddrinfo()
#include <arpa/inet.h> // for inet_ntop inet_ptons ptol etc network filling and conversions 

#define PORT "5500" // Port defined as a string for getaddrinfo

// Function to read the response file and construct an HTTP response
char* get_responseFile(const char *filename) {
    FILE *htmlFile = fopen(filename, "r");
    if (htmlFile == NULL) {
        perror("Error opening file");
        return NULL;
    }

    // Get file size
    fseek(htmlFile, 0, SEEK_END);
    int size = ftell(htmlFile);
    rewind(htmlFile);

    char *buff = (char *)calloc(size + 1, sizeof(char));
    if (buff == NULL) {
        perror("Memory allocation failed");
        fclose(htmlFile);
        return NULL;
    }

    fread(buff, 1, size, htmlFile);
    buff[size] = '\0';
    fclose(htmlFile);

    // Prepare the HTTP protocol header
    char httpHeader[256];
    snprintf(httpHeader, sizeof(httpHeader), 
        "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %d\r\n\r\n", size);

    // Allocate memory for the full response
    char *response = (char *)calloc(strlen(httpHeader) + size + 1, sizeof(char));
    if (response == NULL) {
        perror("Memory allocation failed");
        free(buff);
        return NULL;
    }

    strcpy(response, httpHeader);
    strcat(response, buff);

    free(buff);
    return response;
}

int main() {
    // Set up hints for getaddrinfo
    struct addrinfo hints, *servinfo;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;      // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP socket
    hints.ai_flags = AI_PASSIVE;     // Use device's IP address

    int status;
    if ((status = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(EXIT_FAILURE);
    }

    // Print available server addresses
    for (struct addrinfo *pl = servinfo; pl != NULL; pl = pl->ai_next) {
        char addr[INET6_ADDRSTRLEN];
        if (pl->ai_family == AF_INET) {
            struct sockaddr_in *addr4 = (struct sockaddr_in *)pl->ai_addr;
            inet_ntop(AF_INET, &(addr4->sin_addr), addr, sizeof(addr));
            printf("IPv4 address: %s\n", addr);
        } else if (pl->ai_family == AF_INET6) {
            struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)pl->ai_addr;
            inet_ntop(AF_INET6, &(addr6->sin6_addr), addr, sizeof(addr));
            printf("IPv6 address: %s\n", addr);
        }
    }

    // Create socket
    int sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if (sockfd == -1) {
        perror("Socket creation failed");
        freeaddrinfo(servinfo);
        exit(EXIT_FAILURE);
    }

    // Bind socket
    if (bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
        perror("Bind error");
        close(sockfd);
        freeaddrinfo(servinfo);
        exit(EXIT_FAILURE);
    }
    freeaddrinfo(servinfo); // No longer needed

    // Listen for connections
    if (listen(sockfd, 10) == -1) {
        perror("Listen failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    printf("Server is listening on port %s...\n", PORT);

    struct sockaddr_storage clientAddr;
    socklen_t clientAddrSize = sizeof(clientAddr);

    // Accept client connections
    int clientSockfd = accept(sockfd, (struct sockaddr *)&clientAddr, &clientAddrSize);
    if (clientSockfd == -1) {
        perror("Accept failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Display client IP address
    char clientIp[INET6_ADDRSTRLEN];
    if (clientAddr.ss_family == AF_INET) {
        inet_ntop(AF_INET, &((struct sockaddr_in *)&clientAddr)->sin_addr, clientIp, sizeof(clientIp));
        printf("Client IPv4 address: %s\n", clientIp);
    } else if (clientAddr.ss_family == AF_INET6) {
        inet_ntop(AF_INET6, &((struct sockaddr_in6 *)&clientAddr)->sin6_addr, clientIp, sizeof(clientIp));
        printf("Client IPv6 address: %s\n", clientIp);
    }

    // Receive data from client
    char buffer[1000];
    memset(buffer, 0, sizeof(buffer));
    int recvBytes = recv(clientSockfd, buffer, sizeof(buffer) - 1, 0);
    if (recvBytes > 0) {
        printf("Received message: %s\n", buffer);
    } else {
        perror("Receive failed");
    }

    // Send response to client
    char *response = get_responseFile("Response.html");
    if (response != NULL) {
        int sentBytes = send(clientSockfd, response, strlen(response), 0);
        if (sentBytes == -1) {
            perror("Send failed");
        }
        free(response);
    } else {
        fprintf(stderr, "Failed to load response file\n");
    }

    // Close sockets
    close(clientSockfd);
    close(sockfd);

    return 0;
}
