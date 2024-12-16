#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

int createTCPIPv4Socket();
struct sockaddr_in createIPv4Address(char *ip, int port);