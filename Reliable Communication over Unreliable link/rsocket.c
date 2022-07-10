/*
	Group Number - 5
	Group Member 1 - Suryam Arnav Kalra (19CS30050)
	Group Member 2 - Kunal Singh (19CS30025)
*/

#include "rsocket.h"

typedef struct unack_table {
	char msg[MSG_SIZE];
	size_t msg_len;
	time_t send_time;
	int destination_port;
	int msg_id;
	struct sockaddr_in dest_addr;
    socklen_t addrlen;
    int flags;
    int actual_id;
} unack_table;

typedef struct recv_table {
	int msg_id;
	char msg[MSG_SIZE];
	struct sockaddr_in src_addr;
    socklen_t addrlen;
} recv_table;

unack_table *unacknowledged_message_table;
recv_table *received_message_table;
int udpfd = -1, cnt = 0, cnt_transmission = 0;
struct sockaddr_in recv_source_addr;
socklen_t recv_addr_len = 0;
pthread_t tid_R, tid_S;
pthread_mutex_t lock;

int find_empty_place_unack_table() {
	for(int i = 0; i < TABLE_SIZE; i++) {
		if(unacknowledged_message_table[i].msg_id == -1) {
			return i;
		}
	}
	return -1;
}

int get_top_msg_recv_table() {
	for(int i = 0; i < TABLE_SIZE; i++) {
		if(received_message_table[i].msg_id != -1) {
			return i;
		}
	}
	return -1;
}

int get_empty_place_recv_table() {
	for(int i = 0; i < TABLE_SIZE; i++) {
		if(received_message_table[i].msg_id == -1) {
			return i;
		}
	}
	return -1;
}

char *get_msg_with_seq(char *buf, int id) {
	char *new_buffer;
	int len_id = 0;
	int temp_id = id;
	do {
		len_id++;
		temp_id /= 10;
	} while(temp_id);
	new_buffer = (char *) malloc(100);
	for(int j = 0; j< 100; j++) {
		new_buffer[j] = '\0';
	}
	temp_id = id;
	int i = 0;
	do {
		new_buffer[i++] = temp_id%10 + '0';
		temp_id /= 10;
	} while(temp_id);

	new_buffer[i++] = ' ';
	for(int j = 0; j < strlen(buf); j++) {
		new_buffer[i++] = buf[j];
	}
	new_buffer[i] = '\0';
	return new_buffer;
}

int dropMessage(float t) {
	float r = (float)rand() / ((float)RAND_MAX);
    return (r < t);
}

void *runnerR(void *param) {
	while(1) {
		char buffer[100];
		bzero(buffer, 100);
		recvfrom(udpfd, buffer, 100, 0, ( struct sockaddr *) &recv_source_addr, &recv_addr_len);
		pthread_mutex_lock(&lock);
		// Extracting the id of the recived message
		int id = 0;
		int i = 0;
		int pow = 1;
		while(i < strlen(buffer) && buffer[i] != ' ') {
			id = id + (buffer[i++] - '0')*pow;
			pow *= 10;
		}
		// Calling drop function to see if this message has to be dropped
		int drop = dropMessage(p);
		if(drop == 1) {
			pthread_mutex_unlock(&lock);
			continue;
		}
		i++;
		char msg[100];
		bzero(msg, 100);
		if(strlen(buffer) - i >= 3) {
			char ack[4];
			bzero(ack, 4);
			int k = 0;
			for(int j = 0; j < 3; j++) {
				ack[j] = buffer[i];
				msg[k++] = buffer[i++];
			}
			ack[3] = '\0';
			while(i < strlen(buffer)) {
				msg[k++] = buffer[i++];
			}
			msg[k] = '\0';
			if(strcmp(ack, "ACK") == 0) {
				// pthread_mutex_lock(&lock);
				// checking if ACK message is recived then removing the message 
				// from the unacknowledged_message_table
				printf("ACK: %d\n", unacknowledged_message_table[id].actual_id);
				unacknowledged_message_table[id].msg_id = -1;
				// pthread_mutex_unlock(&lock);
			}
			else {
				// it is a data message
				// pthread_mutex_lock(&lock);
				// if it is a data message then putting it into in the 
				// received_message_table
				int recv_empty = get_empty_place_recv_table();
				received_message_table[recv_empty].msg_id = id;
				strcpy(received_message_table[recv_empty].msg, msg);
				// pthread_mutex_unlock(&lock);
				char to_send[100];
				bzero(to_send, 100);
				k = 0;
				do {
					to_send[k++] = id%10 + '0';
					id /= 10;
				} while(id);
				// since it was a data message so we have to send ack back to the sender
				to_send[k++] = ' ';
				to_send[k++] = 'A', to_send[k++] = 'C', to_send[k++] = 'K';
				to_send[k] = '\0';
				sendto(udpfd, to_send, 100, 0, ( struct sockaddr *) &recv_source_addr, recv_addr_len);
			}
		}
		else {
			// it has to be a data message
			// pthread_mutex_lock(&lock);
			int recv_empty = get_empty_place_recv_table();
			received_message_table[recv_empty].msg_id = id;
			int k = 0;
			while(i < strlen(buffer)) {
				msg[k++] = buffer[i++];
			}
			msg[k] = '\0';
			strcpy(received_message_table[recv_empty].msg, msg);
			// pthread_mutex_unlock(&lock);
			char to_send[100];
			bzero(to_send, 100);
			k = 0;
			do {
				to_send[k++] = id%10 + '0';
				id /= 10;
			} while(id);
			// since it was a data message so we have to send ack back to the sender
			to_send[k++] = ' ';
			to_send[k++] = 'A'; to_send[k++] = 'C'; to_send[k++] = 'K';
			to_send[k] = '\0';
			sendto(udpfd, to_send, 100, 0, ( struct sockaddr *) &recv_source_addr, recv_addr_len);
		}
		pthread_mutex_unlock(&lock);
	}
}

void *runnerS(void *param) {
	while(1) {
		sleep(T);
		pthread_mutex_lock(&lock);
		for(int i = 0; i < TABLE_SIZE; i++) {
			// checking if there is a message in the unacknowledged_message_table
			if(unacknowledged_message_table[i].msg_id != -1) {
				time_t cur_time = time(NULL);
				time_t send_time = unacknowledged_message_table[i].send_time;
				
				// printf("Thread S: %s\tid : %d\n", unacknowledged_message_table[i].msg, unacknowledged_message_table[i].msg_id);
				if(cur_time -send_time >= 2*T) {
					// if the difference between current time and last send time of the message is greater than
					// 2*T then retransmitting the message.
					printf("Retransmit: %d\n", unacknowledged_message_table[i].actual_id);
					// retransmit and reset the time
					unacknowledged_message_table[i].send_time = cur_time;
					sendto(udpfd, unacknowledged_message_table[i].msg, 100, 
						unacknowledged_message_table[i].flags, ( struct sockaddr *) &unacknowledged_message_table[i].dest_addr, 
						unacknowledged_message_table[i].addrlen);
					cnt_transmission++;
				}
			}
		}
		pthread_mutex_unlock(&lock);
	}
}

int r_socket(int domain, int type, int protocol) {
	// check type should be SOCK_MRP
	// assert(type == SOCK_MRP);
	udpfd = socket(domain, SOCK_DGRAM, protocol);
	if(udpfd < 0) {
		return udpfd;
	}
	// allocating space for unacknowledged message table and received message table
	unacknowledged_message_table = (unack_table *) malloc(sizeof(unack_table) * TABLE_SIZE);
	received_message_table = (recv_table *) malloc(sizeof(recv_table) * TABLE_SIZE);

	// setting the message id to be -1, indicating that it is a free space in table
	for(int i = 0; i < TABLE_SIZE; i++) {
		unacknowledged_message_table[i].msg_id = -1;
		received_message_table[i].msg_id = -1;
	}

	// intializing mutex lock
	pthread_mutex_init(&lock,NULL);
    pthread_attr_t attr; //Set of thread attributes
    pthread_attr_init(&attr);

    // creation of thread R and S
    int R = pthread_create(&tid_R, &attr, runnerR, NULL);
    int S = pthread_create(&tid_S, &attr, runnerS, NULL);

    if(R < 0 || S < 0) {
    	return -1;
    }

	return udpfd;
}

int r_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
	return bind(sockfd, addr, addrlen);
}

int r_sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen) {

	char *buffer = (char *)buf;

	// find empty place in unack_table
	pthread_mutex_lock(&lock);
	int empty_place = find_empty_place_unack_table();
	if(empty_place == -1) {
		return -1;
	}

	// filling the details of message to be sent in the unacknowledged_message_table at the first empty space found
	unacknowledged_message_table[empty_place].msg_id = empty_place;
    unacknowledged_message_table[empty_place].send_time = time(NULL);
    unacknowledged_message_table[empty_place].flags = flags;

    // prepending message ID with the message
    char *buffer_with_seq = get_msg_with_seq(buffer, empty_place);

    strcpy(unacknowledged_message_table[empty_place].msg, buffer_with_seq);
    unacknowledged_message_table[empty_place].msg_len = strlen(buffer_with_seq);
    unacknowledged_message_table[empty_place].dest_addr = *(struct sockaddr_in *)dest_addr;
    unacknowledged_message_table[empty_place].addrlen = addrlen;
    unacknowledged_message_table[empty_place].destination_port = ((struct sockaddr_in*)dest_addr)->sin_port;
    unacknowledged_message_table[empty_place].actual_id = cnt;
    pthread_mutex_unlock(&lock);
    cnt++;
    cnt_transmission++;
    // sending the message to the destination adress provided
	int bytes_sent = sendto(sockfd, buffer_with_seq, 100, flags, dest_addr, addrlen);
	free(buffer_with_seq);
	return bytes_sent;
}

int r_recvfrom(int sockfd, char *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen) {
	int bytes_recv = 0;
	while(1) {
		pthread_mutex_lock(&lock);
		int recv_id = get_top_msg_recv_table();
		// checking if there is anything in the received_message_table
		if(recv_id != -1) {
			// if any message is found then copying it to the buffer and removing it from the received_message_table
			strcpy(buf, received_message_table[recv_id].msg);
			len = strlen(buf);
			received_message_table[recv_id].msg_id = -1;
			*src_addr = *(struct sockaddr *)&recv_source_addr;
            *addrlen = recv_addr_len;
            pthread_mutex_unlock(&lock);
            return len;
		}
		else {
			pthread_mutex_unlock(&lock);
			sleep(1);
		}
	}

	return bytes_recv;
}

int r_close(int sockfd) {

	// This infinite while loop exits when there is no message present in the unacknowledged_message_table
	// while(1) {
	// 	int flag = 1;
	// 	pthread_mutex_lock(&lock);
	// 	for(int i = 0; i < TABLE_SIZE; i++) {
	// 		if(unacknowledged_message_table[i].msg_id != -1) {
	// 			flag = 0;
	// 			break;
	// 		}
	// 	}
	// 	pthread_mutex_unlock(&lock);
	// 	if(flag == 1) {
	// 		break;
	// 	}
	// } 

	printf("No of transmissions = %d\n", cnt_transmission);

	// if no message is present in the unacknowledged_message_table then killing thread S and R and freeing the memory for tables
    pthread_kill(tid_R, SIGTSTP);
    pthread_kill(tid_S, SIGTSTP);
	free(unacknowledged_message_table);
	free(received_message_table);
	return close(sockfd);
}


