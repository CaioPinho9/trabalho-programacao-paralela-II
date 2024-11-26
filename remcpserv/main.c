#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <omp.h>

#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 256

typedef struct client_t
{
    char *file_path;
    int upload;
    char *buffer[BUFFER_SIZE];
} client_t;

int handle_buffer(char *buffer, int valread, int *fd, client_t *client)
{
    buffer[valread] = '\0';

    printf("Thread id: %d\n", omp_get_thread_num());

    if (client->upload == -1)
    {
        client->upload = atoi(buffer);
        printf("Upload: %d\n", client->upload);
    }
    else if (client->file_path == NULL)
    {
        printf("File path: %s\n", buffer);
        client->file_path = buffer;
    }
    else
    {
        printf("Data: %s\n", buffer);
    }

    // Envia a resposta de volta para o mesmo cliente
    if (send(*fd, buffer, valread, 0) == -1)
    {
        perror("Erro ao enviar dados ao cliente");
    }

    if (strcmp(buffer, "exit") == 0)
    {
        return -1;
    }
    return 0;
}

int handle_client_activity(client_t *client, struct pollfd *poolfd)
{
    int fd = poolfd->fd;
    char *buffer = *client->buffer;
    if (fd != -1 && (poolfd->revents & POLLIN))
    {
        int valread = read(fd, buffer, BUFFER_SIZE);
        if (valread == 0)
        {
            // Cliente desconectado
            printf("Cliente no socket %d desconectado\n", fd);
            close(fd);
            fd = -1;
        }
        else
        {
            return handle_buffer(buffer, valread, &fd, client);
        }
    }

    return 0;
}

int main()
{
    int server_fd, new_socket;
    struct sockaddr_in address;
    struct pollfd poolfd[MAX_CLIENTS + 1];
    client_t clients[MAX_CLIENTS + 1]; // Estrutura para os descritores de arquivo
    int addrlen = sizeof(address);
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

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("Erro ao configurar SO_REUSEADDR");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

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
        poolfd[i].fd = -1; // Define todos os sockets como não usados
        clients[i].upload = -1; // Initialize upload to -1
        clients[i].file_path = NULL; // Initialize file_path to NULL
    }

    for (int i = 0; i <= MAX_CLIENTS; i++)
    {
        poolfd[i].fd = -1; // Define todos os sockets como não usados
    }

    // O socket do servidor é o primeiro
    poolfd[0].fd = server_fd;
    poolfd[0].events = POLLIN;

    while (1)
    {
        // Chama a função poll para monitorar os sockets
        activity = poll(poolfd, MAX_CLIENTS + 1, -1);

        if (activity < 0)
        {
            perror("Erro na função poll");
            close(server_fd);
            exit(EXIT_FAILURE);
        }

        // Verifica se há uma nova conexão
        if (poolfd[0].revents & POLLIN)
        {
            new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
            if (new_socket < 0)
            {
                perror("Erro ao aceitar a conexão");
                continue;
            }

            printf("Nova conexão aceita, socket fd: %d\n", new_socket);

            // Adiciona o novo socket à estrutura pollfd
            for (int i = 1; i <= MAX_CLIENTS; i++)
            {
                if (poolfd[i].fd == -1)
                {
                    poolfd[i].fd = new_socket;
                    poolfd[i].events = POLLIN; // Monitorar leitura
                    break;
                }
            }
        }

        // clang-format off
        // Verifica se algum cliente enviou dados
        #pragma omp parallel for
        // clang-format on
        for (int i = 1; i <= MAX_CLIENTS; i++)
        {
            if (handle_client_activity(&clients[i], &poolfd[i]) == -1)
            {
                close(server_fd);
                exit(EXIT_SUCCESS);
            }
        }
    }

    // Fecha o socket do servidor
    close(server_fd);

    return 0;
}
