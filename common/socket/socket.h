#ifndef SOCKET_H
#define SOCKET_H

#include <arpa/inet.h>

#define FAILED_TO_SEND_MESSAGE_EXCEPTION "Failed to send message. [0100]\n"
#define SOCKET_CREATE_EXCEPTION "Failed to create socket. [0101]\n"
#define SOCKET_BIND_EXCEPTION "Failed to bind socket. [0102]\n"
#define SOCKET_LISTEN_EXCEPTION "Failed to listen on socket. [0103]\n"
#define POOL_EXCEPTION "Failed to poll on socket. [0104]\n"
#define ACCEPT_EXCEPTION "Failed to accept connection. [0105]\n"
#define CONNECTION_EXCEPTION "Failed to connect to server. [0106]\n"
#define SERVER_CLOSED_EXCEPTION "Server closed connection. [0107]\n"
#define FAILED_TO_RECEIVE_MESSAGE_EXCEPTION "Failed to receive message. [0108]\n"

#define PORT 8080
#define BUFFER_SIZE 256

typedef struct message_t
{
    char *file_path;
    char *buffer;
    int upload;
} message_t;

void create_socket(int *socket_fd, struct sockaddr_in *address, char *host_destination);

void send_message(int socket_fd, message_t *message);

void send_upload(int socket_fd, message_t *message);

void send_file_path(int socket_fd, message_t *message, char *file_path);

void send_offset_size(int socket_fd, message_t *message, char *file_path);

int send_file(int socket_fd, message_t *message, char *file_path_origin);

int handle_receive_message(int socket_fd, char *buffer);

#endif // SOCKET_H