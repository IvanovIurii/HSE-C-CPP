#include <stdio.h>
#include <iostream>
#include "socketutils.h"

struct AcceptedSocket
{
    int acceptedSocketFd;
    struct sockaddr_in address;
    int error;
    bool success;
};

struct AcceptedSocket acceptIncomingConnection(int serverSocketFd);

int main()
{
    int socketFd = createTCPIPv4Socket();
    struct sockaddr_in address = createIPv4Address("", 9999);
    int result = bind(socketFd, (const sockaddr *)&address, sizeof(address));
    if (result < 0)
    {
        perror("Failed to bind");
        exit(EXIT_FAILURE);
    }

    // queue up to 10 connection
    if (listen(socketFd, 10) < 0)
    {
        perror("Failed to start listenening");
        exit(EXIT_FAILURE);
    }

    struct AcceptedSocket clientSocket = acceptIncomingConnection(socketFd);

    char buffer[1024];
    while (true)
    {
        int receivedBytesSize = recv(clientSocket.acceptedSocketFd, buffer, 1024, 0);
        if (receivedBytesSize > 0)
        {
            buffer[receivedBytesSize] = 0;
            std::cout << buffer << std::endl;
        }
        if (receivedBytesSize == 0)
        {
            std::cout << "Connection closed" << std::endl;
            break;
        }
    }

    close(clientSocket.acceptedSocketFd);
    shutdown(socketFd, SHUT_RDWR);

    // todo: propagate a real return code here (1 in case of error)
    return 0;
}

struct AcceptedSocket acceptIncomingConnection(int serverSocketFd)
{
    struct sockaddr_in clientAddress;
    int clientAddressSize = sizeof(sockaddr_in);
    // blocking call
    int clientSocketFd = accept(serverSocketFd, (sockaddr *)&clientAddress, (socklen_t *)&clientAddressSize);

    struct AcceptedSocket acceptedSocket;
    if (clientSocketFd < 0)
    {
        acceptedSocket.error = 1;
        acceptedSocket.success = false;
    }
    else
    {
        acceptedSocket.error = 0;
        acceptedSocket.success = true;
    }

    acceptedSocket.acceptedSocketFd = clientSocketFd;
    acceptedSocket.address = clientAddress;

    return acceptedSocket;
}