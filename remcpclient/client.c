#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "../common/file_controller/file_controller.h"
#include "../common/socket/socket.h"

#define PORT 8080
#define BUFFER_SIZE 256

void parse_arguments(const char *arg, char **host, char **file_path)
{
    char *colon = strchr(arg, ':');
    if (colon)
    {
        // Host exists before the colon
        *host = strndup(arg, colon - arg);
        *file_path = strdup(colon + 1);
    }
    else
    {
        // No host, only file path
        *host = "127.0.0.1";
        *file_path = strdup(arg);
    }
}

int receive_file(int socket_fd, message_t *message)
{
    while (1)
    {
        int valread = recv(socket_fd, message->buffer, BUFFER_SIZE, 0);
        handle_receive_message(socket_fd, message->buffer);

        int result = handle_write_part_file(message->buffer, valread, message);

        if (result != 0)
        {
            return result;
        }
    }
}

int main(int argc, char const *argv[])
{
    if (argc != 3)
    {
        printf("Usage: ./client [host:]file_path_origin [host:]file_path_destination\n");
        return 1;
    }

    char *host_origin = NULL;
    char *file_path_origin = NULL;
    char *host_destination = NULL;
    char *file_path_destination = NULL;
    int upload = 0;

    parse_arguments(argv[1], &host_origin, &file_path_origin);
    parse_arguments(argv[2], &host_destination, &file_path_destination);

    upload = strcmp(host_origin, "127.0.0.1") == 0;

    int socket_fd;
    struct sockaddr_in address;
    char buffer[BUFFER_SIZE];

    create_socket(&socket_fd, &address, host_destination);

    if (connect(socket_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror(CONNECTION_EXCEPTION);
        exit(EXIT_FAILURE);
    }

    printf("Conectado ao servidor. Enviando arquivo..\n");

    message_t *message = (message_t *)malloc(sizeof(message_t));
    message->buffer = buffer;
    message->upload = upload;

    // Send data from file
    if (upload)
    {
        message->file_path = file_path_destination;
        send_upload(socket_fd, message);
        send_file_path(socket_fd, message);
        send_file(socket_fd, message, file_path_origin);
    }
    else
    {
        message->file_path = file_path_origin;
        send_upload(socket_fd, message);
        send_file_path(socket_fd, message);
        receive_file(socket_fd, message);
    }

    free(message);
    close(socket_fd);

    return 0;
}
