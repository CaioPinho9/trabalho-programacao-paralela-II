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
#include <string.h>

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

#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 256

typedef struct client_t
{
    char *file_path;
    int upload;
    char *buffer;
} client_t;

int handle_buffer(char *buffer, int valread, int *fd, client_t *client)
{
    buffer[valread] = '\0';

    if (client->upload == -1)
    {
        printf("Thread id: %d\n", omp_get_thread_num());
        printf("Upload: %d\n", client->upload);
        client->upload = atoi(buffer);
        printf("Upload: %d\n", client->upload);
    }
    else if (client->file_path == NULL)
    {
        printf("Thread id: %d\n", omp_get_thread_num());
        printf("File path: %s\n", client->file_path);
        client->file_path = strdup(buffer);
        printf("File path: %s\n", buffer);

        char file_path_with_part[BUFFER_SIZE];
        snprintf(file_path_with_part, sizeof(file_path_with_part), "%s.part", client->file_path); // Check if the file already exists
        FILE *file = fopen(file_path_with_part, "r");
        if (file != NULL)
        {
            // send the size of the file
            fseek(file, 0, SEEK_END);
            long size = ftell(file);
            char size_str[32];
            snprintf(size_str, sizeof(size_str), "%ld", size);
            if (send(*fd, size_str, strlen(size_str), 0) == -1)
            {
                perror("Erro ao enviar mensagem");
            }
            return 0;
        }
    }
    else
    {
        printf("Thread id: %d\n", omp_get_thread_num());
        printf("Data: '%s'\n", buffer);

        char file_path_with_part[BUFFER_SIZE];
        snprintf(file_path_with_part, sizeof(file_path_with_part), "%s.part", client->file_path);
        printf("File path with part: %s\n", file_path_with_part);
        FILE *file = fopen(file_path_with_part, "a");
        if (file == NULL)
        {
            perror("Erro ao abrir o arquivo");
            return -1;
        }

        fprintf(file, "%s", buffer);
        fclose(file);

        // Check if the last char of buffer is end of file rename the file removing .part
        if (buffer[valread - 1] == EOF)
        {
            printf("Renomeando arquivo...\n");
            rename(file_path_with_part, client->file_path);
        }
    }

    // Envia a resposta de volta para o mesmo cliente
    if (send(*fd, buffer, valread, 0) == -1)
    {
        perror("Erro ao enviar dados ao cliente");
    }

    return 0;
}

int handle_client_activity(client_t *client, struct pollfd *poolfd)
{
    int *fd = &poolfd->fd;
    char *buffer = client->buffer;
    if (*fd != -1 && (poolfd->revents & POLLIN))
    {
        int valread = read(*fd, buffer, BUFFER_SIZE);
        if (valread == 0)
        {
            // Cliente desconectado
            printf("Cliente no socket %d desconectado\n", *fd);
            close(*fd);
            *fd = -1;
        }
        else
        {
            return handle_buffer(buffer, valread, fd, client);
        }
    }
    client->upload = -1;
    client->file_path = NULL;

    return 0;
}

int main()
{
    kill_process_on_port(PORT);

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
        poolfd[i].fd = -1;           // Define todos os sockets como não usados
        clients[i].upload = -1;      // Initialize upload to -1
        clients[i].file_path = NULL; // Initialize file_path to NULL
        clients[i].buffer = (char *)malloc(BUFFER_SIZE * sizeof(char));
    }

    for (int i = 0; i <= MAX_CLIENTS; i++)
    {
        poolfd[i].fd = -1; // Define todos os sockets como não usados
    }

    // O socket do servidor é o primeiro
    poolfd[0].fd = server_fd;
    poolfd[0].events = POLLIN;

    printf("Escutando...\n");

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
