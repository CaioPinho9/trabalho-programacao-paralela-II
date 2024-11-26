#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>

#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 256

int main()
{
    int server_fd, new_socket;
    struct sockaddr_in address;
    struct pollfd clients[MAX_CLIENTS + 1]; // Estrutura para os descritores de arquivo
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE];
    int activity;

    // Cria o socket do servidor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Erro ao criar o socket");
        exit(EXIT_FAILURE);
    }

    // Configurações do servidor
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Associa o socket ao endereço IP e à porta
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Erro ao associar o socket");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Coloca o socket em modo de escuta
    if (listen(server_fd, 3) < 0)
    {
        perror("Erro na função listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Inicializa os descritores de arquivo para o poll()
    for (int i = 0; i <= MAX_CLIENTS; i++)
    {
        clients[i].fd = -1; // Define todos os sockets como não usados
    }

    // O socket do servidor é o primeiro
    clients[0].fd = server_fd;
    clients[0].events = POLLIN;

    while (1)
    {
        // Chama a função poll para monitorar os sockets
        activity = poll(clients, MAX_CLIENTS + 1, -1);

        if (activity < 0)
        {
            perror("Erro na função poll");
            close(server_fd);
            exit(EXIT_FAILURE);
        }

        // Verifica se há uma nova conexão
        if (clients[0].revents & POLLIN)
        {
            new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
            if (new_socket < 0)
            {
                perror("Erro ao aceitar a conexão");
                continue;
            }

            printf("Nova conexão aceita, socket fd: %d\n",
                   new_socket);

            // Adiciona o novo socket à estrutura pollfd
            for (int i = 1; i <= MAX_CLIENTS; i++)
            {
                if (clients[i].fd == -1)
                {
                    clients[i].fd = new_socket;
                    clients[i].events = POLLIN; // Monitorar leitura
                    break;
                }
            }
        }

        // Verifica se algum cliente enviou dados
        for (int i = 1; i <= MAX_CLIENTS; i++)
        {
            if (clients[i].fd != -1 && (clients[i].revents & POLLIN))
            {
                int valread = read(clients[i].fd, buffer, BUFFER_SIZE);
                if (valread == 0)
                {
                    // Cliente desconectado
                    printf("Cliente no socket %d desconectado\n", clients[i].fd);
                    close(clients[i].fd);
                    clients[i].fd = -1;
                }
                else
                {
                    buffer[valread] = '\0';
                    printf("Mensagem recebida do cliente no socket %d: %s\n", clients[i].fd, buffer);

                    // Envia a resposta de volta para o mesmo cliente
                    if (send(clients[i].fd, buffer, valread, 0) == -1)
                    {
                        perror("Erro ao enviar dados ao cliente");
                    }
                }
            }
        }
    }

    // Fecha o socket do servidor
    close(server_fd);

    return 0;
}
