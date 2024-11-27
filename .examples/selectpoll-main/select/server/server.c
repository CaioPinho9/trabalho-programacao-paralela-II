#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 256

int main(void)
{
    fd_set master, read_fds; // conjuntos de descritores
    int fdmax;               // número máximo de descritores
    int listener, newfd;     // socket do servidor e novo socket
    struct sockaddr_in serveraddr, clientaddr;
    socklen_t addrlen;
    int yes = 1;

    FD_ZERO(&master); // limpa os conjuntos master e temporário
    FD_ZERO(&read_fds);

    // Cria o socket do servidor
    if ((listener = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Erro ao criar socket");
        exit(1);
    }

    // Configurações do socket
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    // Configurações do endereço do servidor
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = INADDR_ANY;
    serveraddr.sin_port = htons(PORT);

    // Associa o socket ao endereço e porta
    if (bind(listener, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
    {
        perror("Erro ao associar socket");
        exit(2);
    }

    // Coloca o socket em modo de escuta
    if (listen(listener, MAX_CLIENTS) == -1)
    {
        perror("Erro na função listen");
        exit(3);
    }

    // Adiciona o socket do servidor ao conjunto master
    FD_SET(listener, &master);
    fdmax = listener; // inicialmente, o maior descritor é o do servidor

    int i;
    char buf[BUFFER_SIZE]; // buffer para dados dos clientes
    int nbytes;
    // Loop principal
    while (1)
    {
        read_fds = master; // cópia do conjunto master

        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1)
        {
            perror("Erro na função select");
            exit(4);
        }

        // Verifica quais descritores estão prontos
        for (i = 0; i <= fdmax; i++)
        {
            if (FD_ISSET(i, &read_fds))
            {
                // Novo cliente tentando se conectar
                if (i == listener)
                {
                    addrlen = sizeof(clientaddr);
                    newfd = accept(listener, (struct sockaddr *)&clientaddr, &addrlen);

                    if (newfd == -1)
                    {
                        perror("Erro ao aceitar nova conexão");
                    }
                    else
                    {
                        FD_SET(newfd, &master); // Adiciona o novo socket ao conjunto master
                        if (newfd > fdmax)
                        {
                            fdmax = newfd; // atualiza o máximo descritor
                        }
                        printf("Nova conexão do cliente no socket %d\n", newfd);
                    }
                }
                else
                {
                    // Dados de um cliente já conectado
                    if ((nbytes = recv(i, buf, sizeof(buf), 0)) <= 0)
                    {
                        if (nbytes == 0)
                        {
                            // Cliente fechou a conexão
                            printf("Conexão encerrada no socket %d\n", i);
                        }
                        else
                        {
                            perror("Erro ao receber dados");
                        }
                        close(i);           // Fecha o socket
                        FD_CLR(i, &master); // Remove do conjunto master
                    }
                    else
                    {
                        // Dados recebidos do cliente
                        buf[nbytes] = '\0'; // Garante que a string seja terminada com nulo
                        printf("Mensagem do cliente no socket %d: %s\n", i, buf);

                        // Envia os dados de volta apenas para o cliente que enviou
                        if (send(i, buf, nbytes, 0) == -1)
                        {
                            perror("Erro ao enviar dados");
                        }
                    }
                }
            }
        }
    }

    return 0;
}
