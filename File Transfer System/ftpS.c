// TCP Concurrent server
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
#include <dirent.h>

#define PORT 8080
#define SIZE 2000

const int BUFF_SIZE = 100;

int max(int a, int b) {
	return (a> b)? a: b;
}

int main(int argc, char *argv[]) {
	int server_socket, client_socket, new_socket;
	int opt = 1;
	struct sockaddr_in server_addr, client_addr;

	// reset the addresses
	bzero((char *)&server_addr, sizeof(server_addr));
	bzero((char *)&client_addr, sizeof(client_addr));

	// create the server socket
	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if(server_socket < 0){
		perror("Server Socket Creation Failed");
		exit(1);
	}

	// do the forceful binding of the socket to the address
	if(setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,&opt, sizeof(opt))) {
		perror("setsockopt");
		exit(1);
	}

	// create the server address
	char *ip = "127.0.0.1";
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT);
	server_addr.sin_addr.s_addr = inet_addr(ip);

	// bind the socket to the address
	if(bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
		close(server_socket);
		perror("Binding failed");
		exit(1);
	}

	printf("\nYAAY!!! I'm running the TCP server at %d\n\n", PORT);

	// listen on the server
	if(listen(server_socket, 5) < 0) {
		close(server_socket);
		perror("Error in Listen()");
		exit(1);
	}

	// run the server infinitely
	while(1) {

		// accept the connection from the socket
		int client_len = sizeof(client_addr);
		if ((client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len)) < 0) {
			// close(server_socket);
			perror("Failure in Accept()");
			// exit(1);
		}
		else{
			if(fork() == 0){
				close(server_socket);
				printf("Client connected\n");
				char buffer[SIZE];
				int bytes_read = 0;
				int word_count = 0, character_count = 0, sentence_count = 0, i = 0;
				int current = 0;

				int file_d = open("user.txt", O_RDONLY);
				char read_file[2000];
				int bytes_from_file = read(file_d, read_file, 2000-1);
				read_file[bytes_from_file] = '\0';
				char user1[100], user2[100], pswd1[100], pswd2[100];
				i = 0;
				int j = 0;
				while(i < strlen(read_file) && read_file[i] != ' ') {
					user1[j++] = read_file[i++];
				}
				while(i < strlen(read_file) && read_file[i] == ' '){
					i++;
				}
				user1[j] = '\0';
				j = 0;
				while(i < strlen(read_file) && read_file[i] != '\n') {
					pswd1[j++] = read_file[i++];
				}
				pswd1[j] = '\0';
				i++, j = 0;
				while(i < strlen(read_file) && read_file[i] != ' ') {
					user2[j++] = read_file[i++];
				}
				while(i < strlen(read_file) && read_file[i] == ' '){
					i++;
				}
				user2[j] = '\0';
				j = 0;
				while(i < strlen(read_file)-1) {
					pswd2[j++] = read_file[i++];
				}

				pswd2[j] = '\0';
				close(file_d);
				while(1) {
					int which_user = 0;
					while(1) {
						bzero(buffer, SIZE);
						bytes_read = recv(client_socket, buffer, SIZE, 0);
						// printf("command received : %s\n", buffer);
						// check for user
						char command1[SIZE];
						int j = 0;
						i = 0;
						while(i < strlen(buffer) && buffer[i] != ' ') {
							command1[j++] = buffer[i++];
						}
						command1[j] = '\0';
						i++;
						if(strcmp(command1, "user") == 0) {
							char error_code[4];
							
							char username[BUFF_SIZE];
							j = 0;
							while(i < strlen(buffer)) {
								username[j++] = buffer[i++];
							}
							username[j] = '\0';
							if(strcmp(username, user1) == 0) {
								which_user = 1;
								error_code[0] = '2', error_code[1] = '0', error_code[2] = '0', error_code[3] = '\0';
							}
							else if(strcmp(username, user2) == 0) {
								which_user = 2;
								error_code[0] = '2', error_code[1] = '0', error_code[2] = '0', error_code[3] = '\0';
							}
							else {
								error_code[0] = '5', error_code[1] = '0', error_code[2] = '0', error_code[3] = '\0';
							}
							int x = send(client_socket, error_code, 4, 0);
							if(which_user != 0) {
								break;
							}
						}
						else {
							char error_code[4];
							error_code[0] = '6', error_code[1] = '0', error_code[2] = '0', error_code[3] = '\0';
							int x = send(client_socket, error_code, 4, 0);
						}
					}
					bzero(buffer, SIZE);
					bytes_read = recv(client_socket, buffer, SIZE, 0);
					char command1[SIZE];
					int j = 0;
					i = 0;
					while(i < strlen(buffer) && buffer[i] != ' ') {
						command1[j++] = buffer[i++];
					}
					command1[j] = '\0';
					i++;
					if(strcmp(command1, "pass") == 0) {
						char password[BUFF_SIZE];
						j = 0;
						int match = 0;
						while(i < strlen(buffer)) {
							password[j++] = buffer[i++];
						}
						password[j] = '\0';
						if(which_user == 1) {
							if(strcmp(password, pswd1) == 0) {
								match = 1;
							}
						}
						else {
							if(strcmp(password, pswd2) == 0) {
								match = 1;
							}
						}
						char error_code[4];
						if(match == 0) {
							error_code[0] = '5', error_code[1] = '0', error_code[2] = '0', error_code[3] = '\0';
						}else {
							error_code[0] = '2', error_code[1] = '0', error_code[2] = '0', error_code[3] = '\0';
						}
						int x = send(client_socket, error_code, 4, 0);
						if(match != 0) {
							break;
						} 
					}
					else {
						char error_code[4];
						error_code[0] = '6', error_code[1] = '0', error_code[2] = '0', error_code[3] = '\0';
						int x = send(client_socket, error_code, 4, 0);
					}
				}
				
				while(1) {
					char command[SIZE];
					bzero(command, SIZE);
					int bytes_read = recv(client_socket, command, SIZE, 0);
					if(strcmp(command, "quit") == 0) {
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
						char directory_name[100];
						bzero(directory_name, 100);
						j = 0;
						while(i < strlen(command)) {
							directory_name[j++] = command[i++];
						}
						directory_name[j] = '\0';
						char error_code[4];

						int directory_change = chdir(directory_name);
						if(directory_change < 0) {
							error_code[0] = '5', error_code[1] = '0', error_code[2] = '0', error_code[3] = '\0';
						}
						else {
							error_code[0] = '2', error_code[1] = '0', error_code[2] = '0', error_code[3] = '\0';
						}
						int x = send(client_socket, error_code, 4, 0);
					}
					else if(strcmp(command1, "dir") == 0) {
						DIR *d;
						struct dirent *dir;
						d = opendir(".");
						char directory_content[1000];
						bzero(directory_content, 1000);
						if(d) {
							while((dir = readdir(d)) != NULL) {
								bzero(directory_content, 1000);
							  	// printf("%s\n", dir->d_name);
							  	strcpy(directory_content, dir->d_name);
							  	int x = send(client_socket, directory_content, strlen(directory_content)+1, 0);
							  	// printf("bytes sent = %d\n", x);
							}
							bzero(directory_content, 1000);
							directory_content[0] = '\0';
							// directory_content[1] = '\0';
							int x = send(client_socket, directory_content, 1, 0);
							closedir(d);
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
						int file_d = open(remote_file, O_RDONLY);
						char error_code[4];
						error_code[0] = '2', error_code[1] = '0', error_code[2] = '0', error_code[3] = '\0'; 
						if(file_d < 0) {
							error_code[0] = '5', error_code[1] = '0', error_code[2] = '0', error_code[3] = '\0'; 
						}
						int x = send(client_socket, error_code, 4, 0);
						if(file_d >= 0) {
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
									j = 1;
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
						int file_d = open(remote_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
						char error_code[4];
						error_code[0] = '2', error_code[1] = '0', error_code[2] = '0', error_code[3] = '\0'; 
						if(file_d < 0) {
							error_code[0] = '5', error_code[1] = '0', error_code[2] = '0', error_code[3] = '\0'; 
						}
						int x = send(client_socket, error_code, 4, 0);
						if(file_d >= 0) {
							char sz[17];
							while(1) {
								bzero(sz, 17);
								int bytes_read = recv(client_socket, sz, 17, MSG_WAITALL);
								int to_read = 0;
								if(sz[0] == 'L') {
									close(file_d);
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
								write(file_d, file_content, to_read);
								
							}
						}
					}
				}	
				close(client_socket);
				exit(0);
			}
			close(client_socket);

		}
		
		// close(client_socket);  // close the client socket
	}
	// close(server_socket);
}