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
#include <omp.h>
#include <time.h>
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
int MAX_CLIENTS = 2;
int MAX_THROTTLE = 300;
int THROTTLING_TIME = 100000;

int handle_buffer(char *buffer, int valread, int socket_fd, message_t *message, int verbose)
{
    buffer[valread] = '\0';

    if (message->upload == -1)
    {
        message->upload = atoi(buffer);
        send(socket_fd, buffer, strlen(buffer), 0);
    }
    else if (message->file_path == NULL)
    {
        message->file_path = strdup(buffer);

        if (message->upload)
        {
            send_offset_size(socket_fd, message, message->file_path, verbose);
        }
        else
        {
            send(socket_fd, buffer, strlen(buffer), 0);
        }
    }
    else if (message->upload)
    {
        if (handle_write_part_file(buffer, valread, message, verbose) == -1)
        {
            perror(INVALID_FILE_PATH_EXCEPTION);
            return 0;
        }
        send(socket_fd, buffer, strlen(buffer), 0);
    }
    else
    {
        if (send_file(socket_fd, message, message->file_path, verbose) == -1)
        {
            perror(FILE_NOT_FOUND_EXCEPTION);
            return 0;
        }
    }

    return 0;
}

int handle_message_activity(message_t *message, struct pollfd *poolfd, int *client_count, int *request_count)
{
    int *socket_fd = &poolfd->fd;
    char *buffer = message->buffer;
    if (*socket_fd != -1 && (poolfd->revents & POLLIN))
    {
        int valread = read(*socket_fd, buffer, BUFFER_SIZE);
        if (valread == 0)
        {
            printf("Client in socket %d disconnected\n", *socket_fd);
            close(*socket_fd);
            *socket_fd = -1;
            message->upload = -1;
            message->file_path = NULL;
            memset(message->buffer, 0, BUFFER_SIZE);
            // clang-format off
            #pragma omp critical
            (*client_count)--;
            // clang-format on
            return 0;
        }
        // clang-format off
        #pragma omp critical
        (*request_count)++;
        // clang-format on
        return handle_buffer(buffer, valread, *socket_fd, message, verbose);
    }
    return 0;
}

void reset_request_count(int *request_count)
{
    static time_t last_time = 0;
    // Reset request count to 0 each 1s second
    time_t current_time = time(NULL);
    if (current_time - last_time >= 1)
    {
        // clang-format off
        #pragma omp critical
        // clang-format on
        {
            verbose_printf(verbose, "Rate: %d\n", *request_count);
            *request_count = 0;
        }
        last_time = current_time;
    }
}

// Function to parse arguments
void parse_arguments(int argc, char const *argv[])
{
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-v") == 0)
        {
            verbose = 1;
        }
        else if (strncmp(argv[i], "--max-clients=", 14) == 0)
        {
            MAX_CLIENTS = atoi(argv[i] + 14);
        }
        else if (strncmp(argv[i], "--max-throttle=", 15) == 0)
        {
            MAX_THROTTLE = atoi(argv[i] + 15);
        }
        else if (strncmp(argv[i], "--throttling-time=", 18) == 0)
        {
            THROTTLING_TIME = atoi(argv[i] + 18);
        }
        else
        {
            fprintf(stderr, "Unknown argument: %s\n", argv[i]);
        }
    }
}

int main(int argc, char const *argv[])
{
    kill_process_on_port(PORT);
    omp_set_nested(1);

    int socket_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int activity;
    int client_count = 0;
    int request_count = 0;

    printf("Usage: ./main [-v] [--max-clients=x] [--max-throttle=x] [--throttling-time=x]\n");
    parse_arguments(argc, argv);

    struct pollfd poolfd[MAX_CLIENTS + 1];
    message_t messages[MAX_CLIENTS + 1];

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
        messages[i].upload = -1;
        messages[i].file_path = NULL;
        messages[i].buffer = (char *)malloc(BUFFER_SIZE * sizeof(char));
    }

    poolfd[0].fd = socket_fd;
    poolfd[0].events = POLLIN;

    printf("Listening...\n");

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
            verbose_printf(verbose, "Clients %d\n", client_count);
            if (new_socket < 0)
            {
                perror(ACCEPT_EXCEPTION);
                continue;
            }

            // Check if there is space for a new client
            int slot_found = 0;
            for (int i = 1; i <= MAX_CLIENTS; i++)
            {
                if (poolfd[i].fd == -1)
                {
                    poolfd[i].fd = new_socket;
                    poolfd[i].events = POLLIN;
                    client_count++;
                    printf("New client connected. Total clients: %d\n", client_count);
                    slot_found = 1;
                    break;
                }
            }

            if (!slot_found)
            {
                printf("Too many clients connected. Refusing connection.\n");
                close(new_socket); // Close the new connection
            }
        }

        reset_request_count(&request_count);

        // clang-format off
        #pragma omp parallel for schedule(static, 1)
        // clang-format on
        for (int i = 1; i <= MAX_CLIENTS; i++)
        {
            int break_flag = 0;
            // clang-format off
            #pragma omp critical
            // clang-format on
            if (request_count >= MAX_THROTTLE)
            {
                verbose_printf(verbose, "Throttling...\n");
                usleep(THROTTLING_TIME);
                break_flag = 1;
            }
            if (!break_flag)
            {
                if (handle_message_activity(&messages[i], &poolfd[i], &client_count, &request_count) == -1)
                {
                    close(socket_fd);
                    exit(EXIT_SUCCESS);
                }
            }
        }
    }

    close(socket_fd);

    return 0;
}
