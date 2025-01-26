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
#define numclients 10

// Function to get the response file
char* get_responseFile(char *filename, char *contype) {
    char file[60];
    strcpy(file, filename);

    if (strstr(filename, ".html") == NULL) {
        strcat(filename, ".html"); // Automatically append .html if not present
    }

    printf("file name: %s\n", filename);

    FILE *htmlFile = fopen(file, "rb");
    if (htmlFile == NULL) {
        perror("Error opening file");
        return NULL;
    }

    long size = 0;
    fseek(htmlFile, 0, SEEK_END);
    size = ftell(htmlFile);
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

    char *httpProtocol = (char*)calloc(100, sizeof(char));
    sprintf(httpProtocol, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n", contype, size);

    char *response = (char *)calloc(strlen(httpProtocol) + size + 1, sizeof(char));
    if (response == NULL) {
        perror("Memory allocation failed");
        free(buff);
        return NULL;
    }

    strcpy(response, httpProtocol);
    strcat(response, buff);

    free(httpProtocol);
    free(buff);

    return response;
}

// Function to send the response file to client
int sendresponseFile(int client_sock, char *filename, char *contype) {
    char *response = get_responseFile(filename, contype);
    if (response == NULL) {
        response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\nContent-Length: 0\r\n\r\n";
        send(client_sock, response, strlen(response), 0);
        return -1;
    }

    long sentNo = send(client_sock, response, strlen(response), 0);
    if (sentNo == -1) {
        perror("send error");
        free(response);
        return -1;
    }

    while (sentNo != strlen(response)) {
        sentNo += send(client_sock, response + sentNo, strlen(response) - sentNo, 0);
    }

    free(response);
    return sentNo;
}

// Function to get the route from the HTTP request
char *getroute(char *request) {
    char reqtype[30];
    memset(reqtype, 0, sizeof(reqtype));

    char *route = (char *)calloc(100, sizeof(char));
    sscanf(request, "%s %s HTTP/1.1", reqtype, route);

    return route;
}

// Function to get the request from the client
char *getRequest(int client_sock) {
    int buffsize = 1024;
    char *clientsent = (char *)calloc(buffsize, sizeof(char));

    int bytesread = 0;
    int totalread = 0;

    while (1) {
        bytesread = recv(client_sock, clientsent + totalread, buffsize - totalread, 0);
        if (bytesread < 0) {
            perror("recv failed");
            free(clientsent);
            return NULL;
        } else if (bytesread == 0) {
            break; // Client disconnected
        }

        totalread += bytesread;

        // Check if the full request has been received
        if (strstr(clientsent, "\r\n\r\n")) {
            break; // End of HTTP headers
        }
    }

    clientsent[totalread] = '\0';
    return clientsent;
}

// Function to handle the client response in a new thread
void *clientResponse(void *args) {
    int clientsock = *((int *)args);
    free(args); // Free dynamically allocated memory for socket descriptor

    char *clientsent = NULL, *route = NULL;

    clientsent = getRequest(clientsock);
    if (clientsent == NULL) {
        printf("Error: Failed to receive the request from the client\n");
        close(clientsock);
        pthread_exit(NULL);
    }

    printf("Client sent:\n%s\n", clientsent);

    route = getroute(clientsent);
    if (route == NULL) {
        printf("Error: Failed to extract route from the request\n");
        free(clientsent);
        close(clientsock);
        pthread_exit(NULL);
    }

    printf("Requested route: %s\n", route);

    // Route handling
    if (strstr(route, "..") != NULL) {
        // Prevent directory traversal
        sendresponseFile(clientsock, "404.html", "text/html");
    } else if (strcmp(route, "/") == 0) {
        sendresponseFile(clientsock, "Response.html", "text/html");
    } else if (strcmp(route, "/favicon.ico") == 0) {
        sendresponseFile(clientsock, "favicon.ico", "image/x-icon");
    } else {
        sendresponseFile(clientsock, route + 1, "text/html"); // +1 to skip the leading "/"
    }

    free(route);
    free(clientsent);

    close(clientsock);
    pthread_exit(NULL);
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
    if (bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
        perror("bind failed");
        exit(-2);
    }
    else{
        printf("bind sucess\n");
    }

    return sockfd;
}

int main() {
    int servSock = serverSetup(PORT);

    pthread_t resThread[numclients];

    int result = listen(servSock, 10);
    if (result == 0) {
        printf("listening for connections...\n");
    }

    int i = 0;
    struct sockaddr_in clientAddr;
    int addrlen = sizeof(clientAddr);

    while (1) {
        int *clientSock = malloc(sizeof(int));
        *clientSock = accept(servSock, (struct sockaddr *)&clientAddr, &addrlen);
        if (*clientSock < 0) {
            perror("Accept failed");
            free(clientSock);
            continue;
        } else {
            printf("client connected\n");
        }

        // Create a new thread for each client connection
        pthread_create(&resThread[i], NULL, clientResponse, (void *)clientSock);

        if (i == numclients - 1) {
            printf("max clients are served!\n");
            pthread_cancel(resThread[i]);
            return 0;
        }
        i++;
    }

    // Wait for all threads to complete
    for (int j = 0; j < numclients; j++) {
        pthread_join(resThread[j], NULL);
    }

    return 0;
}