// TCP Client
// Group No. 5
// 19CS30050 + 19CS30025


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>

// #define PORT 8080

#define SIZE 2000

const int BUFF_SIZE = 100;

void myFTP() {
	printf("%s", "\033[1;33mmyFTP> \033[0m");
}

int max(int a, int b) {
	return (a> b)?a: b;
}

int get(char *remote_file, char *local_file, char *command, int client_socket) {
	int client_file = open(local_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if(client_file < 0) {
		return -2;
	}
	int x = send(client_socket, command, strlen(command), 0);
	char error_code[4];
	int bytes_read = recv(client_socket, error_code, 4, 0);
	if(error_code[0] == '5') {
		printf("Error Code: 500 | Error Executing Command\n");
		return -1;
	}
	char sz[17];
	while(1) {
		bzero(sz, 17);
		int bytes_read = recv(client_socket, sz, 17, MSG_WAITALL);
		int to_read = 0;
		if(sz[0] == 'L') {
			close(client_file);
			break;
		}
		for(int i = 1; i < 17; i++) {
			to_read = to_read*2 + (sz[i]-'0');
		}
		// printf("sz = %s  %d, bytes_read = %d\n", sz, to_read, bytes_read);
		char file_content[to_read];
		bzero(file_content, to_read);
		bytes_read = recv(client_socket, file_content, to_read, MSG_WAITALL);
		// printf("bytes_read = %d\n", bytes_read);
		// file_content[to_read] = '\0';
		// printf("length = %d  \ncontent = %s\n", strlen(file_content),file_content);
		write(client_file, file_content, to_read);
		
	}
	return 0;
}

int put(char *remote_file, char *local_file, char *command, int client_socket) {
	int file_d = open(local_file, O_RDONLY);
	if(file_d < 0) {
		return -2;
	}
	int x = send(client_socket, command, strlen(command), 0);
	char error_code[4];
	int bytes_read = recv(client_socket, error_code, 4, 0);
	if(error_code[0] == '5') {
		printf("Error Code: 500 | Error Executing Command\n");
		return -1;
	}
	char file_content[100];
	char main_array[117];
	while(1) {
		bzero(file_content, 100);
		bzero(main_array, 117);
		int bytes_from_file = read(file_d, file_content, 100-1);
		// printf("bytes_from_file = %d\n", bytes_from_file);
		file_content[bytes_from_file] = '\0';
		if(bytes_from_file == 0) {
			main_array[0] = 'L';
			// bytes_from_file++;
			for(int i = 1; i < 16; i++) {
				main_array[i] = '0';
			}
			main_array[16] = '0';
			// main_array[17] = '\0';
			
			int x = send(client_socket, main_array, bytes_from_file+17, 0);
			// printf("x = %d\nsend  = %s\n", x,main_array);
			close(file_d);
			break;
		}
		else {
			main_array[0] = 'M';
			// bytes_from_file++;
			int j = 1;
			for(int i = 15; i >= 0; i--) {
				if(bytes_from_file & (1 << i)) {
					main_array[j++] = '1';
				}
				else {
					main_array[j++] = '0';
				}
			}
			for(int i = 0; i < max(bytes_from_file,strlen(file_content)); i++) {
				main_array[j++] = file_content[i];
			}
			// main_array[j] = '\0';
			// printf("send  = %s\n", main_array);
			int x = send(client_socket, main_array, bytes_from_file+17, 0);
			// printf("x = %d\nsend  = %s\n", x,main_array);
		}
	}
	// printf("out of while\n");
	return 0;
}

int main() {

	int client_socket;
	struct sockaddr_in client_addr;
	bzero((char *)&client_addr, sizeof(client_addr));  // set the address to 0
	client_socket = socket(AF_INET, SOCK_STREAM, 0);   // create the socket
	if(client_socket < 0) {
		perror("Client Socket Creation Failed!!");
		exit(1);
	}
	
	while(1) {
		myFTP();
		char command[SIZE];
		scanf("%[^\n]%*c", command);
		char command1[SIZE], ip[SIZE];
		int i = 0, j = 0;
		while(i < strlen(command) && command[i] != ' ') {
			command1[j++] = command[i++];
		}
		i++;
		command1[j] = '\0';
		if(strcmp(command1, "quit") == 0){
			printf("Closing client\n");
			close(client_socket);
			exit(0);
		}
		if(strcmp(command1, "open") == 0) {
			j = 0;
			while(i < strlen(command) && command[i] != ' ') {
				ip[j++] = command[i++];
			}
			i++;
			ip[j] = '\0';
			int PORT = 0;
			while(i < strlen(command)) {
				PORT = (PORT*10) + (command[i++]-'0');
			}
			client_addr.sin_family = AF_INET;
			client_addr.sin_port = htons(PORT);
			client_addr.sin_addr.s_addr = inet_addr(ip);
			if(connect(client_socket, (struct sockaddr*)&client_addr, sizeof(client_addr)) < 0){
				// close(client_socket);
				perror ("Connection Failed When Creating from Client");
				continue;
				// exit(1);
			}
			printf("Connection Established\n");
			break;
		}
		else {
			printf("open <ip> <port> should be the first command\n");
		}
	}

	while(1) {
		while(1) {
			char command[SIZE];
			myFTP();
			scanf("%[^\n]%*c", command);
			if(strcmp(command, "quit") == 0){
				printf("Closing client\n");
				close(client_socket);
				exit(0);
			}
			int x = send(client_socket, command, strlen(command), 0);
			char error_code[4];
			int bytes_read = recv(client_socket, error_code, 4, 0);
			if(error_code[0] == '6') {
				printf("Error Code: 600 | Error Executing Command\n");
				printf("user <username> should be the first command\n");
			}
			else if(error_code[0] == '5') {
				printf("Error Code: 500 | Error Executing Command\n");
				printf("No such user exists\n");
			}
			else {
				printf("Command Executed Successfully\n");
				printf("User found\n");
				break;
			}
		}
		myFTP();
		char command[SIZE];
		scanf("%[^\n]%*c", command);
		if(strcmp(command, "quit") == 0){
			printf("Closing client\n");
			close(client_socket);
			exit(0);
		}
		int x = send(client_socket, command, strlen(command), 0);
		char error_code[4];
		int bytes_read = recv(client_socket, error_code, 4, 0);
		if(error_code[0] == '6') {
			printf("Error Code: 600 | Error Executing Command\n");
			printf("pass <password> should be the first command after user <username>\n");
		}
		else if(error_code[0] == '5') {
			printf("Error Code: 500 | Error Executing Command\n");
			printf("Password does not match\n");
		}
		else {
			printf("Command Executed Successfully\n");
			printf("User login successful\n");
			break;
		}
	}
	
	// client_addr.sin_family = AF_INET;
	// client_addr.sin_port = htons(8080);
	// client_addr.sin_addr.s_addr = INADDR_ANY;
	// if(connect(client_socket, (struct sockaddr*)&client_addr, sizeof(client_addr)) < 0){
	// 	close(client_socket);
	// 	perror ("Connection Failed When Creating from Client");
	// 	// continue;
	// 	exit(1);
	// }
	// printf("Connection Established\n");
	while(1) {
		myFTP();
		char command[SIZE];
		bzero(command, SIZE);
		scanf("%[^\n]%*c", command);

		if(strcmp(command, "quit") == 0) {
			int x = send(client_socket, command, strlen(command), 0);
			printf("Closing client\n");
			break;
		}

		char command1[SIZE];
		bzero(command1, SIZE);
		int i = 0, j = 0;
		while(i < strlen(command) && command[i] != ' ') {
			command1[j++] = command[i++];
		}
		command1[j] = '\0';
		i++;
		if(strcmp(command1, "cd") == 0) {
			int x = send(client_socket, command, strlen(command), 0);
			char error_code[4];
			bzero(error_code, 4);
			int bytes_read = recv(client_socket, error_code, 4, 0);
			if(error_code[0] == '2') {
				printf("Directory change on server side successful\n");
			}
			else {
				printf("Error Code: %s | Error Executing Command\n", error_code);
				printf("Directory change on server side failed\n");
			}
		}
		else if(strcmp(command1, "lcd") == 0) {
			char directory_name[100];
			bzero(directory_name, 100);
			j = 0;
			while(i < strlen(command)) {
				directory_name[j++] = command[i++];
			}
			directory_name[j] = '\0';

			int directory_change = chdir(directory_name);
			if(directory_change < 0) {
				printf("Directory change on client side failed\n");
			}
			else {
				printf("Directory change on client side successful\n");
			}
		}
		else if(strcmp(command1, "dir") == 0) {
			int x = send(client_socket, command, strlen(command), 0);
			char file_buffer[100];
			char temp_array[100];
			// bzero(file_buffer, 100);
			for(int i = 0; i < 100; i++) {
				file_buffer[i] = '\r';
			}
			bzero(temp_array, 100);
			int prev = 0;
			while(1) {
				for(int i = 0; i < 100; i++) {
					file_buffer[i] = '\r';
				}
				int global_break = 0;
				int bytes_read = recv(client_socket, file_buffer, 100, 0);
				if(file_buffer[0] == '\0') {
					break;
				}
				int i = 0, j = prev;
				while(i < 100) {
					while(i < 100 && file_buffer[i] != '\0') {
						temp_array[j++] = file_buffer[i++];
					}
					if(i < 99 && file_buffer[i+1] == '\0') {
						global_break = 1;
					}
					if(i == 100) {
						prev = j;
						break;
					}
					temp_array[j] = '\0';
					printf("%s\n", temp_array);
					i++, j = 0;
					bzero(temp_array, 100);
					if(global_break == 1) {
						break;
					}
				}
				if(global_break == 1) {
					break;
				}
			}
		}
		else if(strcmp(command1, "get") == 0) {
			char remote_file[100], local_file[100];
			j = 0;
			while(i < strlen(command) && command[i] != ' ') {
				remote_file[j++] = command[i++];
			}
			remote_file[j] = '\0';
			j = 0;
			i++;
			while(i < strlen(command)) {
				local_file[j++] = command[i++];
			}
			local_file[j] = '\0';
			int x = get(remote_file, local_file, command, client_socket);
			if(x == -2) {
				printf("The file cannot be opened for writing\n");
			}
			else if(x == -1) {
				printf("The file cannot be opened on the server side\n");
			}
			else {
				printf("File transfer successful\n");
			}
		}
		else if(strcmp(command1, "put") == 0) {
			char remote_file[100], local_file[100];
			j = 0;
			while(i < strlen(command) && command[i] != ' ') {
				local_file[j++] = command[i++];
			}
			local_file[j] = '\0';
			j = 0;
			i++;
			while(i < strlen(command)) {
				remote_file[j++] = command[i++];
			}
			remote_file[j] = '\0';
			int x = put(remote_file, local_file, command, client_socket);
			if(x == -2) {
				printf("The file cannot be opened for reading\n");
			}
			else if(x == -1) {
				printf("The file cannot be opened on the server side\n");
			}
			else {
				printf("File transfer successful\n");
			}
		}
		else if(strcmp(command1, "mget") == 0) {
			int count = 0;
			while(i < strlen(command)) {
				char file_name[100];
				j = 0;
				while(i < strlen(command) && command[i] != ' ') {
					file_name[j++] = command[i++];
				}
				file_name[j] = '\0';
				i++;
				char temp_command[SIZE];
				temp_command[0] = 'g', temp_command[1] = 'e', temp_command[2] = 't', temp_command[3] = ' ';
				j = 4;
				for(int i = 0; i < strlen(file_name); i++) {
					temp_command[j++] = file_name[i];
				}
				temp_command[j++] = ' ';
				for(int i = 0; i< strlen(file_name); i++) {
					temp_command[j++] = file_name[i];
				}
				temp_command[j] = '\0';
				int x = get(file_name, file_name, temp_command, client_socket);
				if(x == -2) {
					printf("The file cannot be opened for writing\n");
					break;
				}
				else if(x == -1) {
					printf("The file cannot be opened on the server side\n");
					break;
				}
				else {
					printf("File transfer %d successful\n", ++count);
				}
			}

		}
		else if(strcmp(command1, "mput") == 0) {
			int count = 0;
			while(i < strlen(command)) {
				char file_name[100];
				j = 0;
				while(i < strlen(command) && command[i] != ' ') {
					file_name[j++] = command[i++];
				}
				file_name[j] = '\0';
				i++;
				char temp_command[SIZE];
				temp_command[0] = 'p', temp_command[1] = 'u', temp_command[2] = 't', temp_command[3] = ' ';
				j = 4;
				for(int i = 0; i < strlen(file_name); i++) {
					temp_command[j++] = file_name[i];
				}
				temp_command[j++] = ' ';
				for(int i = 0; i< strlen(file_name); i++) {
					temp_command[j++] = file_name[i];
				}
				temp_command[j] = '\0';
				int x = put(file_name, file_name, temp_command, client_socket);
				if(x == -2) {
					printf("The file cannot be opened for reading\n");
					break;
				}
				else if(x == -1) {
					printf("The file cannot be opened on the server side\n");
					break;
				}
				else {
					printf("File transfer %d successful\n", ++count);
				}
			}
		}
		else {
			printf("Invalid command\n");
		}
	}
	
	close(client_socket);
}