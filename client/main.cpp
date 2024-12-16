#include <iostream>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include "socketutils.h"

int main(int argc, char *argv[])
{
    // add args validation
    char *port = argv[1];

    int socketFd = createTCPIPv4Socket();
    struct sockaddr_in address = createIPv4Address("", atoi(port)); // this is just empty IP, treated as ANY

    int result = connect(socketFd, (struct sockaddr *)&address, sizeof(address));
    if (result < 0)
    {
        perror("Failed to connect");
        exit(EXIT_FAILURE);
    }

    std::cout << "What is your name?" << std::endl;
    std::string name;
    getline(std::cin, name);

    std::cout << "You can chat now..." << std::endl;
    while (true)
    {
        std::cout << "YOU: ";

        std::string message;
        getline(std::cin, message);
        std::string resultMessage = name + ": " + message;

        std::cout << resultMessage << std::endl;
        std::cout << resultMessage.length() << std::endl;

        send(socketFd, resultMessage.c_str(), resultMessage.length(), 0);
    }

    close(socketFd);

    return 0;
}