#include <iostream>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, char *argv[])
{
    char *ipAddress = argv[1];
    char *port = argv[2];

    int socketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFd < 0)
    {
        perror("Failed to create a socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in address;
    address.sin_port = htons(atoi(port));
    address.sin_family = AF_INET;

    inet_pton(AF_INET, ipAddress, &address.sin_addr.s_addr);

    int result = connect(socketFd, (struct sockaddr *)&address, sizeof(address));
    if (result < 0)
    {
        perror("Failed to connect");
        exit(EXIT_FAILURE);
    }

    // send message faking http protocol
    char *message;
    message = "GET \\ HTTP/1.1 \r\nHost:google.com\r\n\r\n";

    send(socketFd, message, strlen(message), 0);

    char buffer[1024];
    recv(socketFd, buffer, 1024, 0);

    std::cout << buffer << std::endl;

    return 0;
}