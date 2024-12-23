#include <iostream>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include "socketutils.h"

#define BUFFER_SIZE 4096

int main(int argc, char *argv[])
{
    // todo: add args validation
    // todo: add usage
    char *port = argv[1];

    int socketFd = createTCPIPv4Socket();
    struct sockaddr_in address = createIPv4Address("", atoi(port)); // todo: add IP address argument

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

        // todo: handle errors
        send(socketFd, command.c_str(), command.length(), 0);

        char buffer[BUFFER_SIZE];
        // todo: handle errors
        ssize_t receivedBytesSize = recv(socketFd, buffer, BUFFER_SIZE, 0);
        buffer[receivedBytesSize] = 0;

        std::cout << buffer << std::endl;
    }

    close(socketFd);

    return 0;
}