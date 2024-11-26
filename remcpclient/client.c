#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

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

    printf("Conectado ao servidor. Digite uma mensagem:\n");

    // Send the file path origin
    char upload_char = upload ? '1' : '0';
    if (send(sockfd, &upload_char, sizeof(char), 0) == -1)
    {
        perror("Erro ao enviar mensagem");
    }

    // Send the file path origin
    if (send(sockfd, file_path_origin, strlen(file_path_origin), 0) == -1)
    {
        perror("Erro ao enviar mensagem");
    }

    // Send data from file
    if (upload)
    {
        // FILE *file = fopen(full_path, "r");
        // if (file == NULL)
        // {
        //     perror("Erro ao abrir o arquivo");
        //     exit(EXIT_FAILURE);
        // }

        // while (fgets(buffer, BUFFER_SIZE, file) != NULL)
        // {
        //     if (send(sockfd, buffer, strlen(buffer), 0) == -1)
        //     {
        //         perror("Erro ao enviar mensagem");
        //     }
        // }

        // fclose(file);
    }

    // Lê a mensagem do usuário
    fgets(message, BUFFER_SIZE, stdin);
    message[strcspn(message, "\n")] = 0; // Remove o caractere de nova linha

    // Envia a mensagem ao servidor
    if (send(sockfd, message, strlen(message), 0) == -1)
    {
        perror("Erro ao enviar mensagem");
    }

    // Verifica se o cliente deseja sair
    if (strcmp(message, "exit") == 0)
    {
        printf("Encerrando a conexão...\n");
    }

    // Recebe a resposta do servidor
    int valread = recv(sockfd, buffer, BUFFER_SIZE, 0);
    if (valread > 0)
    {
        buffer[valread] = '\0'; // Garante que a string seja terminada com nulo
        printf("Mensagem recebida do servidor: %s\n", buffer);
    }
    else if (valread == 0)
    {
        printf("Servidor fechou a conexão.\n");
    }
    else
    {
        perror("Erro ao receber dados do servidor");
    }

    // Fecha o socket
    close(sockfd);

    return 0;
}
