#include "socketutils.h"

int createTCPIPv4Socket()
{
    int socketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFd < 0)
    {
        perror("Failed to create a socket");
        exit(EXIT_FAILURE);
    }

    return socketFd;
}

sockaddr_in createIPv4Address(char *ip, int port)
{
    struct sockaddr_in address;
    address.sin_port = htons(port);
    address.sin_family = AF_INET;

    if (strlen(ip) == 0)
    {
        address.sin_addr.s_addr = INADDR_ANY; // listens from any incoming IP (for server)
    }
    else
    {
        inet_pton(AF_INET, ip, &address.sin_addr.s_addr);
    }

    return address;
}