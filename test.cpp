#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <string>

int main()
{
    int fd = open("/dev/my_device", O_RDONLY);
    if (fd < 0)
    {
        perror("open");
        return 1;
    }

    std::string key;

    // todo: if not exit, send a message
    // todo:
    std::cout << "PRESS ANY KEY...";
    std::getline(std::cin, key);
    std::cout << "Hello, " << key << "!\n";

    close(fd);
    return 0;
}
