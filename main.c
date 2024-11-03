#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 80
#define METHOD 0
#define PATH 1

struct HttpReq{
	char *method;
	char *path;
};

char *file_404(){
	FILE *file_404;
	file_404 = fopen("./static/404.html", "r");

	fseek(file_404, 0, SEEK_END);
	long file_size = ftell(file_404);
	fseek(file_404, 0, SEEK_SET);
	char *response = (char *)malloc(file_size);
	if(response == NULL){
		perror("Failed to allocate memory");
		return NULL;
	}
	size_t bytes_read = fread(response, 1, file_size, file_404);
	if(bytes_read != file_size){
		perror("Failed to read file");
		free(response);
		fclose(file_404);
		return NULL;
	}

	return response;
}

char *get_handler(char *path){
	char *response = NULL;

	const char *base_path = "./static/";

	size_t full_path_size = strlen(base_path) + strlen(path) + 1;

	char *full_path = (char *)malloc(full_path_size);
	if(full_path == NULL){
		perror("Failed to allocate memory");
		response = file_404();
		return response;
	}

	snprintf(full_path, full_path_size, "%s%s", base_path, path);

	FILE *file;
	file = fopen(full_path, "r");
	if(file == NULL){
		perror("Failed to open file");	
		free(full_path);
		response = file_404();
		return response;
	}

	fseek(file, 0, SEEK_END);
	long file_size = ftell(file);
	fseek(file, 0, SEEK_SET);

	response = (char *)malloc(file_size);
	if(response == NULL){
		perror("Failed to allocate memory");
		free(full_path);
		fclose(file);
		response = file_404();
		return response;
	}
	size_t bytes_read = fread(response, 1, file_size, file);
	if(bytes_read != file_size){
		perror("Failed to read file");
		free(response);
		fclose(file);
		response = file_404();
		return response;
	}

	response[file_size] = '\0';

	fclose(file);
	free(full_path);

	return response;
}

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
	printf("LISTENING on %s %d: \n", inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));

	while(1){
		int accept_socket;
		accept_socket = accept(server_socket, (struct sockaddr *)&server_addr, &server_addr_len);

		if(accept_socket < 0){
			perror("Failed to accept the socket");
			close(accept_socket);
			close(server_socket);
			return -1;
		}

		char buffer[1024];
		memset(buffer, 0, sizeof(buffer));
		ssize_t bytes_read = read(accept_socket, buffer, sizeof(buffer));
		if(bytes_read < 0){
			perror("Failed to read data from socket");
			close(accept_socket);
			close(server_socket);
			return -1;
		}
		buffer[bytes_read] = '\0';
		printf("Request: \n%s\n", buffer);
		printf("Request: \n%zd\n", bytes_read);

		char *line = strtok(buffer, "\r\n");
		struct HttpReq httpReq = {0};
		int cnt = 0;
		char *token = strtok(line, " ");
		while(line != NULL && cnt < PATH+1){
			if(token != NULL && cnt == METHOD){
				httpReq.method = token;
			}
			else if(token != NULL && cnt == PATH){
				httpReq.path = token;
			}
			cnt++;
			token = strtok(NULL, " ");
		}

		char *response = NULL;
		if(httpReq.method == NULL || httpReq.path == NULL){
			perror("Error: Invalid request format\n");
			response = file_404();
		}

		printf("method: %s\n", httpReq.method);
		printf("path: %s\n", httpReq.path);
		if(strcmp(httpReq.method, "GET") == 0){
			response = get_handler(httpReq.path);
		}
		else if(strcmp(httpReq.method, "POST") == 0){
			printf("POST request!!!");
		}
		else{
			printf("NO request!!!");
		}

		ssize_t bytes_write = write(accept_socket, response, strlen(response));
		if(bytes_write < 0){
			perror("Failed to write data to socket");
			close(accept_socket);
			close(server_socket);
			return -1;
		}
		free(response);
		close(accept_socket);
	}

	close(server_socket);

	return 0;
}
