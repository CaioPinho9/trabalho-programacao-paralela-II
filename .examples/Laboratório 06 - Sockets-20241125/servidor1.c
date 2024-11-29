#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdlib.h>

int main()
{
	int server_socket_fd, client_socket_fd;
	int server_len, client_len;
	struct sockaddr_un server_address;
	struct sockaddr_un client_address;
	char ch;

	server_socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	server_address.sun_family = AF_UNIX;
	strcpy(server_address.sun_path, "server_socket");
	server_len = sizeof(server_address);
	bind(server_socket_fd, (struct sockaddr *)&server_address, server_len);
	listen(server_socket_fd, 5);
	while (1)
	{
		printf("server waiting\n");
		client_len = sizeof(client_address);
		client_socket_fd = accept(server_socket_fd, (struct sockaddr *)&client_address, &client_len);
		read(client_socket_fd, &ch, 1);
		ch++;
		write(client_socket_fd, &ch, 1);
		close(client_socket_fd);
	}
}
