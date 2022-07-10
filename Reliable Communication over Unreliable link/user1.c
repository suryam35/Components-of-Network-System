/*
	Group Number - 5
	Group Member 1 - Suryam Arnav Kalra (19CS30050)
	Group Member 2 - Kunal Singh (19CS30025)
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa//inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include "rsocket.h"

#define ROLL_NO 50

int main() {
	int client_socket;
	struct sockaddr_in client_addr, server_addr;
	bzero((char *)&client_addr, sizeof(client_addr));
	client_socket = r_socket(AF_INET, SOCK_MRP, 0);
	if(client_socket < 0) {
		perror("Client Socket Creation Failed!!");
		exit(1);
	}

	// create the client address
	client_addr.sin_family = AF_INET;
	client_addr.sin_port = htons(50000 + 2*ROLL_NO);
	client_addr.sin_addr.s_addr = INADDR_ANY;

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(50000 + 2*ROLL_NO + 1);
	server_addr.sin_addr.s_addr = INADDR_ANY;

	if(r_bind(client_socket, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0){
		r_close(client_socket);
		perror("Binding failed");
		exit(1);
	}

	printf("Enter a string: ");
	char msg[100];
	scanf("%s", msg);

	for(int i = 0; i < strlen(msg); i++) {
		char to_send[100];
		bzero(to_send, 100);
		to_send[0] = msg[i];
		to_send[1] = '\0';
		// printf("to_send = %s\n", to_send);
		r_sendto(client_socket, to_send, strlen(to_send) + 1, 0, (const struct sockaddr *) &server_addr, sizeof(server_addr));
	}
	// printf("exit loop\n");
	// r_close(client_socket);
	printf("Actual length = %ld\n", strlen(msg));
	while(1);
}