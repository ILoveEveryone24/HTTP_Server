#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8000

int main(){
	int server_socket, client_socket;
	struct sockaddr_in server_addr, client_addr;
	socklen_t addr_size = sizeof(client_addr);

	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(PORT);

	bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));
	listen(server_socket, 10);

	return 0;
}
