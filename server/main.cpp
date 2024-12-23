#include <stdio.h>
#include <iostream>
#include <sstream>
#include <vector>
#include "socketutils.h"
#include <signal.h>

#define TIMEOUT_SECONDS 5

struct AcceptedSocket
{
    int acceptedSocketFd;
    struct sockaddr_in address;
    int error;
    bool success;
};

struct AcceptedSocket acceptIncomingConnection(int serverSocketFd);
std::vector<std::string> split(const std::string &target, char c);

void handle_alarm(int sig)
{
    std::cout << sig << std::endl;
}

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
            //
            std::string command(buffer);

            int pfd[2];
            pipe(pfd);

            pid_t pid = fork();

            if (pid == 0)
            {
                alarm(TIMEOUT_SECONDS);

                close(pfd[0]); // because we do not read from pipe
                dup2(pfd[1], STDOUT_FILENO);
                dup2(pfd[1], STDERR_FILENO);

                // todo: extract as fun and move up
                auto args = split(command, ' '); // todo: use auto
                char *argv[args.size() + 1];

                int i;
                for (i = 0; i < args.size(); i++)
                {
                    char *str = (char *)args[i].c_str();
                    if (strlen(str) > 0)
                    {
                        argv[i] = str;
                    }
                }
                argv[i] = NULL; // null terminator

                execvp(argv[0], argv); // todo add timeout on the execution
                close(pfd[1]);
                fflush(stdout);

                return 0;
            }
            close(pfd[1]); // because we only read from pipe, but do not write into

            int status;
            pid_t wait_pid;

            wait_pid = waitpid(pid, &status, 0);
            if (WIFSIGNALED(status))
            {
                std::cout << "Command execution reached timeout: " << TIMEOUT_SECONDS << " sec" << std::endl;
                std::cout << "Child process was terminated by signal: " << WTERMSIG(status) << std::endl;
            }

            std::string long_string; // todo: rename to result

            // todo: put in the same block
            if (WIFSIGNALED(status))
            {
                long_string = "Command execution timeout";
                send(clientSocket.acceptedSocketFd, long_string.c_str(), long_string.length(), 0);
                continue;
            }

            char foo[4096];
            ssize_t bytes_read;

            while ((bytes_read = read(pfd[0], foo, 4096)) > 0)
            {
                long_string.append(foo, bytes_read);
                foo[bytes_read] = 0;
            }

            close(pfd[0]);

            std::cout << long_string << std::endl;

            // send back via socket
            send(clientSocket.acceptedSocketFd, long_string.c_str(), long_string.length(), 0);
            // todo: handle errors
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

// todo: refactor
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

// todo: rename to split by space and make lambda out of it, because it is used only in one place
std::vector<std::string> split(const std::string &target, char c)
{
    std::string temp;
    std::stringstream stringstream{target};
    std::vector<std::string> result;

    while (std::getline(stringstream, temp, c))
    {
        if (temp.length() > 0)
        {
            result.push_back(temp);
        }
    }

    return result;
}