#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdlib.h>

int main()
{
	int socket_fd;
	int len;
	struct sockaddr_un address;
	int result;
	char ch = 'A';

	socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	address.sun_family = AF_UNIX;
	strcpy(address.sun_path, "server_socket");
	len = sizeof(address);
	result = connect(socket_fd, (struct sockaddr *)&address, len);
	if (result == -1)
	{
		perror("oops: client1");
		exit(1);
	}
	write(socket_fd, &ch, 1);
	read(socket_fd, &ch, 1);
	printf("char from server = %c\n", ch);
	close(socket_fd);
	exit(0);
}
