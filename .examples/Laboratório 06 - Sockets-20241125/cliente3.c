#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
int main()
{
  int socket_fd;
  int len;
  struct sockaddr_in address;
  int result;
  char str[1024] = "Alo mundo";

  socket_fd = socket(AF_INET, SOCK_STREAM, 0);

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = inet_addr("127.0.0.1");
  // address.sin_addr.s_addr = inet_addr("192.168.0.15");
  // address.sin_addr.s_addr = htonl(INADDR_ANY);
  address.sin_port = htons(9734);

  len = sizeof(address);
  result = connect(socket_fd, (struct sockaddr *)&address, len);
  if (result == -1)
  {
    perror("oops: client1");
    exit(1);
  }
  write(socket_fd, &str, 1024);
  // sleep(30);
  read(socket_fd, &str, 1024);
  printf("mensagem do servidor: %s\n", str);
  close(socket_fd);
  exit(0);
}
