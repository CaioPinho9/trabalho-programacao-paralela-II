#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#define PORT 6000

int main(int argc, char const *argv[])
{
    int sockfd;
    struct sockaddr_in s_addr;
    char message[1024];
    char buffer[1024];

    printf("Enter the message to send: ");
    fgets(message, sizeof(message), stdin);
    message[strcspn(message, "\n")] = '\0';

    // Create a socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    s_addr.sin_family = AF_INET;
    s_addr.sin_port = htons(PORT);
    s_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect to the server
    if (connect(sockfd, (struct sockaddr *)&s_addr, sizeof(s_addr)) < 0)
    {
        perror("Connection failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Send the message to the server
    send(sockfd, message, strlen(message), 0);

    // Receive the response from the server
    int n = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (n < 0)
    {
        perror("Receiving failed");
    }
    else
    {
        buffer[n] = '\0';
        printf("Server response: %s\n", buffer);
    }

    // Close the socket
    close(sockfd);
    return 0;
}
