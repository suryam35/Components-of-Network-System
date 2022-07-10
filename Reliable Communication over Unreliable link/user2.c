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
	int server_socket;
	struct sockaddr_in server_addr, client_addr;

	// reset the addresses
	bzero((char *)&server_addr, sizeof(server_addr));
	bzero((char *)&client_addr, sizeof(client_addr));

	// create the server socket
	server_socket = r_socket(AF_INET, SOCK_MRP, 0);
	if(server_socket < 0){
		perror("Server Socket Creation Failed");
		exit(1);
	}

	// create the server address
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(50000 + 2*ROLL_NO + 1);
	server_addr.sin_addr.s_addr = INADDR_ANY;

	// bind the socket to the address
	if(r_bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
		r_close(server_socket);
		perror("Binding failed");
		exit(1);
	}

	while(1) {
		socklen_t client_len = sizeof(client_addr);
		char to_recv[100];
		bzero(to_recv, 100);
		r_recvfrom(server_socket, to_recv, 100, 0, ( struct sockaddr *) &client_addr, &client_len);
		printf("%s", to_recv);
		fflush(stdout);
	}
}