#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define PORT 8080
#define BUFFER_SIZE 256

int main()
{
    int socket_fd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    char message[BUFFER_SIZE];

    // Cria o socket
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Erro ao criar socket");
        exit(EXIT_FAILURE);
    }

    // Configurações do servidor
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Conecta ao servidor local

    // Conecta ao servidor
    if (connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Erro ao conectar ao servidor");
        exit(EXIT_FAILURE);
    }

    printf("Conectado ao servidor. Digite uma mensagem:\n");

    // Loop para enviar e receber mensagens
    while (1)
    {
        // Lê a mensagem do usuário
        fgets(message, BUFFER_SIZE, stdin);
        message[strcspn(message, "\n")] = 0; // Remove o caractere de nova linha

        // Envia a mensagem ao servidor
        if (send(socket_fd, message, strlen(message), 0) == -1)
        {
            perror(FAILED_TO_SEND_MESSAGE_EXCEPTION);
        }

        // Verifica se o cliente deseja sair
        if (strcmp(message, "exit") == 0)
        {
            printf("Encerrando a conexão...\n");
            break;
        }

        // Recebe a resposta do servidor
        int valread = recv(socket_fd, buffer, BUFFER_SIZE, 0);
        if (valread > 0)
        {
            buffer[valread] = '\0'; // Garante que a string seja terminada com nulo
            printf("Mensagem recebida do servidor: %s\n", buffer);
        }
        else if (valread == 0)
        {
            printf("Servidor fechou a conexão.\n");
            break;
        }
        else
        {
            perror("Erro ao receber dados do servidor");
        }
    }

    // Fecha o socket
    close(socket_fd);

    return 0;
}
