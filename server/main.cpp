#include <stdio.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <signal.h>
#include <thread>
#include "socketutils.h"

#define BUFFER_SIZE 4096

#define TIMEOUT_SECONDS 10
#define MAX_CONNECTIONS 5
#define PORT 9999

struct AcceptedSocket
{
    int acceptedSocketFd;
    struct sockaddr_in address;
};

struct AcceptedSocket acceptIncomingConnection(int serverSocketFd);
std::vector<std::string> split(const std::string &target, char c);

void handleCommand(char buffer[], int clientSocketFd);
void receiveAndAndHandleCommand(int clientSocketFd);
void receiveAndAndHandleCommandOnThread(int clientSocketFd);
void execute_command(std::string command);

int main()
{
    int socketFd = createTCPIPv4Socket();
    struct sockaddr_in address = createIPv4Address("", PORT);
    int result = bind(socketFd, (const sockaddr *)&address, sizeof(address));
    if (result < 0)
    {
        perror("Failed to bind");
        exit(EXIT_FAILURE);
    }

    if (listen(socketFd, MAX_CONNECTIONS) < 0)
    {
        perror("Failed to start listenening");
        exit(EXIT_FAILURE);
    }

    while (true)
    {
        struct AcceptedSocket clientSocket = acceptIncomingConnection(socketFd);
        std::cout << "New connection from port: " << clientSocket.address.sin_port << std::endl;

        receiveAndAndHandleCommandOnThread(clientSocket.acceptedSocketFd);
    }

    shutdown(socketFd, SHUT_RDWR);
    return 0;
}

void receiveAndAndHandleCommandOnThread(int clientSocketFd)
{
    std::thread my_thread(receiveAndAndHandleCommand, clientSocketFd);
    my_thread.detach();
}

void receiveAndAndHandleCommand(int clientSocketFd)
{
    char buffer[BUFFER_SIZE];
    while (true)
    {
        int receivedBytesSize = recv(clientSocketFd, buffer, BUFFER_SIZE, 0);
        if (receivedBytesSize > 0)
        {
            buffer[receivedBytesSize] = 0;
            handleCommand(buffer, clientSocketFd);
        }
        if (receivedBytesSize == 0)
        {
            std::cout << "Connection closed by client" << std::endl;
            break;
        }
    }

    // once client is done
    close(clientSocketFd);
}

void handleCommand(char buffer[], int response_socket)
{
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

        execute_command(command);

        close(pfd[1]);
    }
    close(pfd[1]); // because we only read from pipe

    int status;
    pid_t wait_pid;
    std::string result;

    wait_pid = waitpid(pid, &status, 0);
    if (WIFSIGNALED(status))
    {
        std::cout << "Command execution reached timeout: " << TIMEOUT_SECONDS << " sec" << std::endl;
        std::cout << "Child process was terminated by signal: " << WTERMSIG(status) << std::endl;

        result = "Command execution timeout";
        send(response_socket, result.c_str(), result.length(), 0);
        return;
    }

    char read_buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    while ((bytes_read = read(pfd[0], read_buffer, BUFFER_SIZE)) > 0)
    {
        result.append(read_buffer, bytes_read);
        read_buffer[bytes_read] = 0;
    }

    close(pfd[0]);

    std::cout << result << std::endl;

    size_t bytes_sent = send(response_socket, result.c_str(), result.length(), 0);
    if (bytes_sent == -1)
    {
        perror("Something went wrong on sent...");
    }
}

struct AcceptedSocket acceptIncomingConnection(int serverSocketFd)
{
    struct sockaddr_in clientAddress;
    int clientAddressSize = sizeof(sockaddr_in);
    // blocking
    int clientSocketFd = accept(serverSocketFd, (sockaddr *)&clientAddress, (socklen_t *)&clientAddressSize);

    struct AcceptedSocket acceptedSocket;
    acceptedSocket.acceptedSocketFd = clientSocketFd;
    acceptedSocket.address = clientAddress;

    return acceptedSocket;
}

void execute_command(std::string command)
{
    auto args = split(command, ' ');
    char *argv[args.size() + 1];

    for (int i = 0; i < args.size(); i++)
    {
        char *str = (char *)args[i].c_str();
        if (strlen(str) > 0)
        {
            argv[i] = str;
        }
    }
    argv[args.size()] = NULL;

    execvp(argv[0], argv);
}

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