#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdarg.h>
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

void send_message(int socket_fd, message_t *message, int verbose)
{
    verbose_printf(verbose, "Mensagem: %s\n", message->buffer);
    if (send(socket_fd, message->buffer, strlen(message->buffer), 0) == -1)
    {
        perror(FAILED_TO_SEND_MESSAGE_EXCEPTION);
    }
    handle_receive_message(socket_fd, message->buffer, 0);
}

void send_upload(int socket_fd, message_t *message, int verbose)
{
    char upload_char = message->upload ? '1' : '0';
    strncpy(message->buffer, &upload_char, 1);
    message->buffer[1] = '\0';
    send_message(socket_fd, message, verbose);
}

void send_file_path(int socket_fd, message_t *message, char *file_path, int verbose)
{
    strncpy(message->buffer, file_path, BUFFER_SIZE - 1);
    message->buffer[BUFFER_SIZE - 1] = '\0';
    send_message(socket_fd, message, verbose);
}

void send_offset_size(int socket_fd, message_t *message, char *file_path, int verbose)
{
    char *file_path_with_part;
    get_part_file_path(file_path, &file_path_with_part);
    long size = get_size_file(file_path_with_part);
    char size_str[8];
    sprintf(size_str, "%ld", size);
    strncpy(message->buffer, size_str, sizeof(size_str));
    message->buffer[sizeof(size_str)] = '\0';
    free(file_path_with_part);
    send(socket_fd, message->buffer, strlen(message->buffer), 0);
    verbose_printf(verbose, "Mensagem: %s\n", message->buffer);
}

int send_file(int socket_fd, message_t *message, char *file_path_origin, int verbose)
{
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
        verbose_printf(verbose, "Offseted by %s bytes\n", message->buffer);
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
        }

        send_message(socket_fd, message, verbose);
    }
    if (!eof)
    {
        // Send EOF
        char eof_marker = EOF;
        if (send(socket_fd, &eof_marker, 1, 0) == -1)
        {
            perror(FAILED_TO_SEND_MESSAGE_EXCEPTION);
        }
        verbose_printf(verbose, "Enviado EOF\n");
        handle_receive_message(socket_fd, message->buffer, 0);
    }

    fclose(file);
    free(abs_path);
    return 0;
}

int handle_receive_message(int socket_fd, char *buffer, int verbose)
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
    verbose_printf(verbose, "Mensagem: %s\n", buffer);
    return valread;
}

void verbose_printf(int verbose, const char *format, ...)
{
    if (verbose)
    {
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
    }
}