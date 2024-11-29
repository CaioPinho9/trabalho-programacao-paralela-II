#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "socket.h"
#include "../../common/file_controller/file_controller.h"

void create_socket(int *socket_fd, struct sockaddr_in *address, char *host_destination)
{
    if ((*socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror(SOCKET_CREATE_EXCEPTION);
        exit(EXIT_FAILURE);
    }

    address->sin_family = AF_INET;
    address->sin_port = htons(PORT);
    address->sin_addr.s_addr = host_destination != NULL ? inet_addr(host_destination) : INADDR_ANY;
}

void send_upload(int socket_fd, message_t *message)
{
    printf("Enviando upload...\n");
    if (send(socket_fd, message->buffer, sizeof(char), 0) == -1)
    {
        perror(FAILED_TO_SEND_MESSAGE_EXCEPTION);
    }
    handle_receive_message(socket_fd, message->buffer);
}

void send_file_path(int socket_fd, message_t *message)
{
    printf("Enviando caminho do arquivo...\n");
    if (send(socket_fd, message->file_path, strlen(message->file_path), 0) == -1)
    {
        perror(FAILED_TO_SEND_MESSAGE_EXCEPTION);
    }
    handle_receive_message(socket_fd, message->buffer);
}

int send_file(int socket_fd, message_t *message, char *file_path_origin)
{
    printf("Enviando arquivo...\n");
    char *abs_path;
    if (get_abs_path(file_path_origin, &abs_path) == -1)
    {
        perror(INVALID_FILE_PATH_EXCEPTION);
        return -1;
    }

    FILE *file = fopen(abs_path, "r");
    if (file == NULL)
    {
        perror(FILE_NOT_FOUND_EXCEPTION);
        return -1;
    }

    if (strcmp(message->buffer, file_path_origin) != 0)
    {
        printf("Offseted by %s bytes\n", message->buffer);
        // Buffer has how many bytes already were read, jump to next position
        fseek(file, atoi(message->buffer), SEEK_SET);
    }

    int eof = 0;
    while (fgets(message->buffer, BUFFER_SIZE, file) != NULL)
    {
        size_t len = strlen(message->buffer);

        // Check if this is the last part of the file
        if (len < BUFFER_SIZE - 1)
        {
            // Append EOF to the buffer if there's space
            message->buffer[len] = EOF_MARKER;
            message->buffer[len + 1] = '\0'; // Null-terminate the buffer
            eof = 1;
            printf("EOF encontrado\n");
        }

        if (send(socket_fd, message->buffer, strlen(message->buffer), 0) == -1)
        {
            perror(FAILED_TO_SEND_MESSAGE_EXCEPTION);
        }
        handle_receive_message(socket_fd, message->buffer);
    }
    if (!eof)
    {
        // Send EOF
        printf("Enviando EOF...\n");
        char eof_marker = EOF;
        if (send(socket_fd, &eof_marker, 1, 0) == -1)
        {
            perror(FAILED_TO_SEND_MESSAGE_EXCEPTION);
        }
        handle_receive_message(socket_fd, message->buffer);
    }

    fclose(file);
}

int handle_receive_message(int socket_fd, char *buffer)
{
    int valread = recv(socket_fd, buffer, BUFFER_SIZE, 0);
    if (valread == 0)
    {
        perror(SERVER_CLOSED_EXCEPTION);
        return -1;
    }
    else if (valread < 0)
    {
        perror(FAILED_TO_RECEIVE_MESSAGE_EXCEPTION);
        return -1;
    }

    buffer[valread] = '\0';
    if (strcmp(buffer, FILE_EXISTS_EXCEPTION) == 1)
    {
        perror(FILE_EXISTS_EXCEPTION);
        return -1;
    }

    if (strcmp(buffer, FILE_NOT_FOUND_EXCEPTION) == 1)
    {
        perror(FILE_NOT_FOUND_EXCEPTION);
        return -1;
    }
    return 0;
}
