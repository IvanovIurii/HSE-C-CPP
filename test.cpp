#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <string>

int main()
{
    int fd = open("/dev/my_device", O_RDWR);
    if (fd < 0)
    {
        perror("open");
        return 1;
    }

    std::string key;

    // todo: send to a device one by one
    // if message is PING -> just read and return PONG
    std::cout << "PRESS ANY KEY...";
    std::getline(std::cin, key);

    close(fd);
    return 0;
}
