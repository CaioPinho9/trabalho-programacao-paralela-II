#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "../common/file_controller.h"

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

void handle_receive_message(char *buffer, int *sockfd)
{
    // Recebe a resposta do servidor
    int valread = recv(*sockfd, buffer, BUFFER_SIZE, 0);
    if (valread > 0)
    {
        buffer[valread] = '\0'; // Garante que a string seja terminada com nulo
        // printf("Mensagem recebida do servidor: %s\n", buffer);
    }
    else if (valread == 0)
    {
        printf("Servidor fechou a conexão.\n");
    }
    else
    {
        perror("Erro ao receber dados do servidor");
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

    // Parse the first argument
    parse_arguments(argv[1], &host_origin, &file_path_origin);
    // Parse the second argument
    parse_arguments(argv[2], &host_destination, &file_path_destination);

    upload = strcmp(host_origin, "127.0.0.1") == 0;

    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    char message[BUFFER_SIZE];

    // Cria o socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Erro ao criar socket");
        exit(EXIT_FAILURE);
    }

    // Configurações do servidor
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(host_destination); // Conecta ao servidor local

    // Conecta ao servidor
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Erro ao conectar ao servidor");
        exit(EXIT_FAILURE);
    }

    printf("Conectado ao servidor. Enviando arquivo..\n");

    // Send the file path origin
    char upload_char = upload ? '1' : '0';
    if (send(sockfd, &upload_char, sizeof(char), 0) == -1)
    {
        perror("Erro ao enviar mensagem");
    }

    handle_receive_message(buffer, &sockfd);

    // Send the file path destination
    if (send(sockfd, file_path_destination, strlen(file_path_destination), 0) == -1)
    {
        perror("Erro ao enviar mensagem");
    }

    handle_receive_message(buffer, &sockfd);

    // Send data from file
    if (upload)
    {
        printf("Enviando arquivo...\n");
        char *abs_path;
        if (get_abs_path(file_path_origin, &abs_path) == -1)
        {
            perror("Erro ao obter o diretório atual");
            exit(EXIT_FAILURE);
        }

        FILE *file = fopen(abs_path, "r");
        if (file == NULL)
        {
            perror("Erro ao abrir o arquivo");
            exit(EXIT_FAILURE);
        }

        if (strcmp(buffer, file_path_destination) != 0)
        {
            printf("Offseted by %s bytes\n", buffer);
            // Buffer has how many bytes already were read, jump to next position
            fseek(file, atoi(buffer), SEEK_SET);
        }

        int eof = 0;
        while (fgets(buffer, BUFFER_SIZE, file) != NULL)
        {
            size_t len = strlen(buffer);

            // Check if this is the last part of the file
            if (len < BUFFER_SIZE - 1)
            {
                // Append EOF to the buffer if there's space
                buffer[len] = EOF_MARKER;
                buffer[len + 1] = '\0'; // Null-terminate the buffer
                len += 1;               // Update length to include EOF
                eof = 1;
                printf("EOF encontrado\n");
            }

            if (send(sockfd, buffer, strlen(buffer), 0) == -1)
            {
                perror("Erro ao enviar mensagem");
            }
            handle_receive_message(buffer, &sockfd);
        }
        if (!eof)
        {
            // Send EOF
            printf("Enviando EOF...\n");
            char eof_marker = EOF;
            if (send(sockfd, &eof_marker, 1, 0) == -1)
            {
                perror("Erro ao enviar EOF");
            }
            handle_receive_message(buffer, &sockfd);
        }

        fclose(file);
    }

    // Fecha o socket
    close(sockfd);

    return 0;
}
