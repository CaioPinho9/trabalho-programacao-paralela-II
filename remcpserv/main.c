#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#define PORT 6000

int main()
{
    int server_fd, client_fd;
    char buffer[1024];
    struct sockaddr_in s_addr, c_addr;
    socklen_t c_addr_len;

    // Create a socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    s_addr.sin_family = AF_INET;
    s_addr.sin_addr.s_addr = INADDR_ANY;
    s_addr.sin_port = htons(PORT);

    // Bind the socket to the address
    if (bind(server_fd, (const struct sockaddr *)&s_addr, sizeof(s_addr)) < 0)
    {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Start listening for incoming connections
    if (listen(server_fd, 5) < 0)
    {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    while (1)
    {
        // Accept a client connection
        c_addr_len = sizeof(c_addr);
        if ((client_fd = accept(server_fd, (struct sockaddr *)&c_addr, &c_addr_len)) < 0)
        {
            perror("Accept failed");
            continue;
        }

        printf("Client connected.\n");

        // Receive data from the client
        int n = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (n < 0)
        {
            perror("Receiving failed");
            close(client_fd);
            continue;
        }

        buffer[n] = '\0';

        char *result;
        if (strchr(buffer, '+') != NULL && strchr(buffer, '*') != NULL)
        {
            result = "Invalid String.";
        }
        else if (strchr(buffer, '+') != NULL)
        {
            char *token = strtok(buffer, "+");
            int sum = 0;
            while (token != NULL)
            {
                sum += atoi(token);
                token = strtok(NULL, "+");
            }
            sprintf(buffer, "%d", sum);
            result = buffer;
            printf("The string contains a plus sign (+).\n");
        }
        else if (strchr(buffer, '*') != NULL)
        {
            printf("The string contains an asterisk (*).\n");
            result = "Asterisk processing not implemented.";
        }
        else
        {
            result = "Invalid String.";
        }

        printf("Result: %s\n", result);

        // Send the result back to the client
        send(client_fd, result, strlen(result), 0);

        // Close the client connection
        close(client_fd);
        printf("Client disconnected.\n");
    }

    // Close the server socket
    close(server_fd);

    return 0;
}
