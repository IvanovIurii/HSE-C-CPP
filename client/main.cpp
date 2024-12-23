#include <iostream>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include "socketutils.h"

#define BUFFER_SIZE 4096

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cout << "Port is not provided!" << std::endl;
        exit(EXIT_FAILURE);
    }
    char *port = argv[1];

    int socketFd = createTCPIPv4Socket();
    struct sockaddr_in address = createIPv4Address("", atoi(port)); // on localhost

    int result = connect(socketFd, (struct sockaddr *)&address, sizeof(address));
    if (result < 0)
    {
        perror("Failed to connect");
        exit(EXIT_FAILURE);
    }

    std::cout << "Type a command to execute... Or exit" << std::endl;
    while (true)
    {
        std::cout << "COMMAND: ";
        std::string command;
        getline(std::cin, command);

        if (command == "exit")
        {
            break;
        }

        ssize_t bytes_sent = send(socketFd, command.c_str(), command.length(), 0);
        if (bytes_sent == -1)
        {
            perror("Something went wrong on sent...");
            continue;
        }

        char buffer[BUFFER_SIZE];
        ssize_t bytes_received = recv(socketFd, buffer, BUFFER_SIZE, 0);
        if (bytes_received == -1)
        {
            perror("Something went wrong on receive...");
            continue;
        }
        else if (bytes_received == 0)
        {
            std::cerr << "Connection closed by server" << std::endl;
            close(socketFd);
            exit(EXIT_FAILURE);
        }
        buffer[bytes_received] = 0;

        std::cout << buffer << std::endl;
    }

    close(socketFd);

    return 0;
}