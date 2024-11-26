#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 6000

int main() {
    int server_fd;
    char buffer[1024];
    struct sockaddr_in s_addr, c_addr;
    socklen_t c_addr_len;
    char *response = "Hello from UDP server";

    if ((server_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
  
    s_addr.sin_family = AF_INET;
    s_addr.sin_addr.s_addr = INADDR_ANY; 
    s_addr.sin_port = htons(PORT);

    if (bind(server_fd, (const struct sockaddr*)&s_addr, sizeof(s_addr)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server waiting..\n");
    c_addr_len = sizeof(c_addr);
    int n = recvfrom(server_fd, buffer, 1024, MSG_WAITALL, (struct sockaddr*)&c_addr, &c_addr_len);
    buffer[n] = '\0';
    printf("Message from client: %s\n", buffer);

    sendto(server_fd, response, strlen(response), MSG_CONFIRM, (const struct sockaddr*)&c_addr, c_addr_len);
    printf("Response sent to client\n");
    close(server_fd);
    return 0;
}
