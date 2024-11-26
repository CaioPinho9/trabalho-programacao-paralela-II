#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 6000

int main() {
    int sockfd;
    struct sockaddr_in serv_addr;
    char *message = "Hello, here is a UDP client";
    char buffer[1024];

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    sendto(sockfd, message, strlen(message), MSG_CONFIRM, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    printf("UDP message sent to server\n");

    sleep(6);
    socklen_t len = sizeof(serv_addr);
    int n = recvfrom(sockfd, buffer, 1024, MSG_WAITALL, (struct sockaddr*)&serv_addr, &len);
    buffer[n] = '\0';
    printf("Message received from server: %s\n", buffer);

    close(sockfd);
    return 0;
}
