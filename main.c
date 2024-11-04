#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 80
#define METHOD 0
#define PATH 1
#define ALLOWED_FILES 3

const char *allowed_files[ALLOWED_FILES] = {
	"/index.html",
	"/login.html",
	"/404.html"
};

struct HttpReq{
	char *method;
	char *path;
	char *body;
};

int is_allowed_path(const char *path){
	for(int i = 0; i < ALLOWED_FILES; i++){
		if(strcmp(path, allowed_files[i]) == 0){
			return 1;	
		}
	}
	return 0;
}

char *file_404(){
	const char *http_404_header = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n";

	FILE *file_404;
	file_404 = fopen("./static/404.html", "r");

	fseek(file_404, 0, SEEK_END);
	long file_size = ftell(file_404);
	fseek(file_404, 0, SEEK_SET);
	char *response = (char *)malloc(file_size + strlen(http_404_header) + 1);
	if(response == NULL){
		perror("Failed to allocate memory");
		return NULL;
	}

	memcpy(response, http_404_header, strlen(http_404_header));

	size_t bytes_read = fread(response + strlen(http_404_header), 1, file_size, file_404);
	if(bytes_read != file_size){
		perror("Failed to read file");
		free(response);
		fclose(file_404);
		return NULL;
	}

	response[file_size + strlen(http_404_header)] = '\0';
	
	printf("\n\nResponse: %s\n\n", response);

	return response;
}

char *post_handler(char *data){
	if(data == NULL){
		return file_404();	
	}
	
	char username[100];
	char password[100];
	sscanf(data, "username=%99[^&]&password=%99s", username, password);

	char response[1024];
	snprintf(response, sizeof(response), "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nUsername: %s\nPassword: %s", username, password);
	return strdup(response);
}

char *get_handler(char *path){
	const char *http_ok_header = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";

	char *response = NULL;

	if(!is_allowed_path(path)){
		printf("Access denied!\n");
		return file_404();
	}

	const char *base_path = "./static/";

	size_t full_path_size = strlen(base_path) + strlen(path) + 1;

	char *full_path = (char *)malloc(full_path_size);
	if(full_path == NULL){
		perror("Failed to allocate memory");
		return file_404();
	}

	snprintf(full_path, full_path_size, "%s%s", base_path, path);

	FILE *file;
	file = fopen(full_path, "r");
	if(file == NULL){
		perror("Failed to open file");	
		free(full_path);
		return file_404();
	}

	fseek(file, 0, SEEK_END);
	long file_size = ftell(file);
	fseek(file, 0, SEEK_SET);

	response = (char *)malloc(file_size + strlen(http_ok_header) + 1);
	if(response == NULL){
		perror("Failed to allocate memory");
		free(full_path);
		fclose(file);
		return file_404();
	}
	
	memcpy(response, http_ok_header, strlen(http_ok_header));

	size_t bytes_read = fread(response + strlen(http_ok_header), 1, file_size, file);
	if(bytes_read != file_size){
		perror("Failed to read file");
		free(response);
		fclose(file);
		return file_404();
	}

	response[file_size + strlen(http_ok_header)] = '\0';

	fclose(file);
	free(full_path);

	printf("\n\nResponse: %s\n\n", response);

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
		
		struct HttpReq httpReq = {0};

		char *body_start = strstr(buffer, "\r\n\r\n");
		if(body_start != NULL){
			body_start += 4;
			httpReq.body = body_start;	
		}
		else{
			httpReq.body = NULL;	
		}

		char *line = strtok(buffer, "\r\n");
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
		printf("Method: %s\n", httpReq.method);
		printf("Path: %s\n", httpReq.path);
		printf("Body: %s\n", httpReq.body);

		char *response = NULL;
		if(httpReq.method == NULL || httpReq.path == NULL){
			perror("Error: Invalid request format\n");
			response = file_404();
			continue;
		}

		if(strcmp(httpReq.method, "GET") == 0){
			response = get_handler(httpReq.path);
		}
		else if(strcmp(httpReq.method, "POST") == 0){
			response = post_handler(httpReq.body);
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
