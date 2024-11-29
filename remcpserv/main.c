#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <omp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../common/file_controller/file_controller.h"
#include "../common/socket/socket.h"

void kill_process_on_port(int port)
{
    char command[128];

    // Construct the command to find the PID of the process using the port
#ifdef __linux__
    snprintf(command, sizeof(command), "fuser -k %d/tcp", port); // Linux command to kill process on port
#elif __APPLE__
    snprintf(command, sizeof(command), "lsof -ti:%d | xargs kill -9", port); // macOS command to kill process on port
#elif _WIN32
    snprintf(command, sizeof(command), "netstat -ano | findstr :%d > pid.txt && for /f \"tokens=5\" %%a in (pid.txt) do taskkill /PID %%a /F", port); // Windows command
#else
    fprintf(stderr, "Unsupported OS\n");
    return;
#endif

    // Execute the command
    if (system(command) == -1)
    {
        perror("Failed to kill process on port");
    }
    else
    {
        printf("Killed any process using port %d\n", port);
    }
}

#define MAX_CLIENTS 10

int handle_buffer(char *buffer, int valread, int socket_fd, message_t *client)
{
    buffer[valread] = '\0';
    printf("Recebido: %s\n", buffer);

    if (client->upload == -1)
    {
        client->upload = atoi(buffer);
        printf("Upload: %d\n", client->upload);
        send(socket_fd, buffer, strlen(buffer), 0);
    }
    else if (client->file_path == NULL)
    {
        client->file_path = strdup(buffer);
        printf("File path: %s\n", client->file_path);

        int exists = file_exists(client->file_path);
        if (exists && client->upload)
        {
            printf(FILE_EXISTS_EXCEPTION);
            if (send(socket_fd, FILE_EXISTS_EXCEPTION, strlen(FILE_EXISTS_EXCEPTION), 0) == -1)
            {
                perror(FAILED_TO_SEND_MESSAGE_EXCEPTION);
            }
        }
        else if (!exists && !client->upload)
        {
            printf(FILE_NOT_FOUND_EXCEPTION);
            if (send(socket_fd, FILE_NOT_FOUND_EXCEPTION, strlen(FILE_NOT_FOUND_EXCEPTION), 0) == -1)
            {
                perror(FAILED_TO_SEND_MESSAGE_EXCEPTION);
            }
        }

        char file_path_with_part[512];
        get_part_file_path(client->file_path, file_path_with_part);
        printf("Part file path: %s\n", file_path_with_part);
        char size_str[32];
        get_size_file(file_path_with_part, size_str);
        if (send(socket_fd, size_str, strlen(size_str), 0) == -1)
        {
            perror(FAILED_TO_SEND_MESSAGE_EXCEPTION);
        }

        return 0;
    }
    else if (client->upload)
    {
        if (handle_write_part_file(buffer, valread, client) == -1)
        {
            perror(INVALID_FILE_PATH_EXCEPTION);
            return 0;
        }
        send(socket_fd, buffer, strlen(buffer), 0);
    }
    else
    {
        send_file(socket_fd, client, client->file_path);
    }

    return 0;
}

int handle_client_activity(message_t *client, struct pollfd *poolfd)
{
    int *socket_fd = &poolfd->fd;
    char *buffer = client->buffer;
    if (*socket_fd != -1 && (poolfd->revents & POLLIN))
    {
        int valread = read(*socket_fd, buffer, BUFFER_SIZE);
        if (valread == 0)
        {
            // Cliente desconectado
            printf("Cliente no socket %d desconectado\n", *socket_fd);
            close(*socket_fd);
            *socket_fd = -1;
            client->upload = -1;
            client->file_path = NULL;
            memset(client->buffer, 0, BUFFER_SIZE);
            return 0;
        }
        return handle_buffer(buffer, valread, *socket_fd, client);
    }
    return 0;
}

int main()
{
    kill_process_on_port(PORT);

    int socket_fd, new_socket;
    struct sockaddr_in address;
    struct pollfd poolfd[MAX_CLIENTS + 1];
    message_t clients[MAX_CLIENTS + 1];
    int addrlen = sizeof(address);
    int activity;

    create_socket(&socket_fd, &address, NULL);

    struct linger so_linger = {1, 0}; // Close immediately on shutdown
    setsockopt(socket_fd, SOL_SOCKET, SO_LINGER, &so_linger, sizeof(so_linger));

    if (bind(socket_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror(SOCKET_BIND_EXCEPTION);
        close(socket_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(socket_fd, 3) < 0)
    {
        perror(SOCKET_LISTEN_EXCEPTION);
        close(socket_fd);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i <= MAX_CLIENTS; i++)
    {
        poolfd[i].fd = -1;
        clients[i].upload = -1;
        clients[i].file_path = NULL;
        clients[i].buffer = (char *)malloc(BUFFER_SIZE * sizeof(char));
    }

    for (int i = 0; i <= MAX_CLIENTS; i++)
    {
        poolfd[i].fd = -1;
    }

    poolfd[0].fd = socket_fd;
    poolfd[0].events = POLLIN;

    printf("Escutando...\n");

    while (1)
    {
        activity = poll(poolfd, MAX_CLIENTS + 1, -1);

        if (activity < 0)
        {
            perror(POOL_EXCEPTION);
            close(socket_fd);
            exit(EXIT_FAILURE);
        }

        if (poolfd[0].revents & POLLIN)
        {
            new_socket = accept(socket_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
            if (new_socket < 0)
            {
                perror(ACCEPT_EXCEPTION);
                continue;
            }

            printf("Nova conexão aceita, socket fd: %d\n", new_socket);

            for (int i = 1; i <= MAX_CLIENTS; i++)
            {
                if (poolfd[i].fd == -1)
                {
                    poolfd[i].fd = new_socket;
                    poolfd[i].events = POLLIN;
                    break;
                }
            }
        }

        // clang-format off
        #pragma omp parallel for schedule(static, 1)
        // clang-format on
        for (int i = 1; i <= MAX_CLIENTS; i++)
        {
            if (handle_client_activity(&clients[i], &poolfd[i]) == -1)
            {
                close(socket_fd);
                exit(EXIT_SUCCESS);
            }
        }
    }

    close(socket_fd);

    return 0;
}
