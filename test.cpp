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
    std::cout << "TYPE ANYTHING..." << std::endl;
    std::getline(std::cin, key);

    char characters[key.size()];
    key.copy(characters, key.size(), 0);

    while (true)
    {
        for (unsigned int i = 0; i < sizeof(characters) / sizeof(char); ++i)
        {
            std::cout << "Writing: " << characters[i] << std::endl;
            char bytes_to_sent[2] = {characters[i], '\0'};

            if (write(fd, bytes_to_sent, 2) < 0)
            {
                perror("Can not write to device");
                return 1;
            }
        }

        // sleep and healthcheck after
        sleep(5);
        std::cout << "PING" << std::endl;

        char buffer[4];
        read(fd, buffer, 4);

        buffer[4] = 0;

        std::cout << buffer << std::endl; // PONG
    }

    close(fd);
    return 0;
}