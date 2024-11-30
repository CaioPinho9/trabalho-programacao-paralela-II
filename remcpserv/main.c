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

int verbose = 0;

#define MAX_messageS 10

int handle_buffer(char *buffer, int valread, int socket_fd, message_t *message)
{
    buffer[valread] = '\0';

    if (message->upload == -1)
    {
        message->upload = atoi(buffer);
        send(socket_fd, buffer, strlen(buffer), 0);
        verbose_printf("Mensagem: %d\n", message->buffer);
    }
    else if (message->file_path == NULL)
    {
        message->file_path = strdup(buffer);

        if (message->upload)
        {
            send_offset_size(socket_fd, message, message->file_path);
        }
        else
        {
            send(socket_fd, buffer, strlen(buffer), 0);
            verbose_printf("Mensagem: %d\n", message->buffer);
        }
    }
    else if (message->upload)
    {
        if (handle_write_part_file(buffer, valread, message) == -1)
        {
            perror(INVALID_FILE_PATH_EXCEPTION);
            return 0;
        }
        send(socket_fd, buffer, strlen(buffer), 0);
        verbose_printf("Mensagem: %d\n", message->buffer);
    }
    else
    {
        if (send_file(socket_fd, message, message->file_path) == -1)
        {
            perror(FILE_NOT_FOUND_EXCEPTION);
            return 0;
        }
    }

    return 0;
}

int handle_message_activity(message_t *message, struct pollfd *poolfd)
{
    int *socket_fd = &poolfd->fd;
    char *buffer = message->buffer;
    if (*socket_fd != -1 && (poolfd->revents & POLLIN))
    {
        int valread = read(*socket_fd, buffer, BUFFER_SIZE);
        if (valread == 0)
        {
            // Client desconectado
            printf("Client no socket %d desconectado\n", *socket_fd);
            close(*socket_fd);
            *socket_fd = -1;
            message->upload = -1;
            message->file_path = NULL;
            memset(message->buffer, 0, BUFFER_SIZE);
            return 0;
        }
        return handle_buffer(buffer, valread, *socket_fd, message);
    }
    return 0;
}

int main()
{
    kill_process_on_port(PORT);

    int socket_fd, new_socket;
    struct sockaddr_in address;
    struct pollfd poolfd[MAX_messageS + 1];
    message_t messages[MAX_messageS + 1];
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

    for (int i = 0; i <= MAX_messageS; i++)
    {
        poolfd[i].fd = -1;
        messages[i].upload = -1;
        messages[i].file_path = NULL;
        messages[i].buffer = (char *)malloc(BUFFER_SIZE * sizeof(char));
    }

    for (int i = 0; i <= MAX_messageS; i++)
    {
        poolfd[i].fd = -1;
    }

    poolfd[0].fd = socket_fd;
    poolfd[0].events = POLLIN;

    printf("Escutando...\n");

    while (1)
    {
        activity = poll(poolfd, MAX_messageS + 1, -1);

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

            for (int i = 1; i <= MAX_messageS; i++)
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
        for (int i = 1; i <= MAX_messageS; i++)
        {
            if (handle_message_activity(&messages[i], &poolfd[i]) == -1)
            {
                close(socket_fd);
                exit(EXIT_SUCCESS);
            }
        }
    }

    close(socket_fd);

    return 0;
}
