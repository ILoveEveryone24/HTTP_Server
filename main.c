#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 80

int main(){
	int server_socket;
	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if(server_socket < 0){
		perror("Failed to create socket");	
		return -1;
	}

	struct sockaddr_in server_addr;
	socklen_t server_addr_len = sizeof(server_addr);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT);
	server_addr.sin_addr.s_addr = INADDR_ANY;

	if(bind(server_socket, (struct sockaddr *)&server_addr, server_addr_len) < 0){
		perror("Failed to bind the socket");
		close(server_socket);
		return -1;
	}

	if(listen(server_socket, 3) < 0){
		perror("Failed to listen to the socket");
		close(server_socket);
		return -1;
	}

	int accept_socket;
	accept_socket = accept(server_socket, (struct sockaddr *)&server_addr, &server_addr_len);

	if(accept_socket < 0){
		perror("Failed to accept the socket");
		close(accept_socket);
		close(server_socket);
		return -1;
	}

	int buffer[1024];
	memset(buffer, 0, sizeof(buffer));
	ssize_t bytes_read = read(accept_socket, buffer, sizeof(buffer));
	if(bytes_read < 0){
		perror("Failed to read data from socket");
		close(accept_socket);
		close(server_socket);
		return -1;
	}
	printf("Request: \n%zd\n", bytes_read);

	const char *response = "HTTP/1.1 200 OK\nContent-Length; 13\n\nHello, world!";
	ssize_t bytes_write = write(accept_socket, response, strlen(response));
	if(bytes_write < 0){
		perror("Failed to write data to socket");
		close(accept_socket);
		close(server_socket);
		return -1;
	}

	close(accept_socket);
	close(server_socket);

	return 0;
}
