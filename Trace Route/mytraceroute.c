// Group - 5
// Suryam Arnav Kalra - 19CS30050
// Kunal Singh - 19CS30025

// Trace Route

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>

#define SOURCE_PORT 8080
#define DESTINATION_PORT 32164
#define MAX_HOP 16
#define N 52
#define PCKT_LEN 8192
#define MAX_CHAR 100
#define TIMEOUT 1
#define MSG_SIZE 2048

int verified = 0;
char prev_buffer[PCKT_LEN];
char prev_received[MAX_CHAR];
int glb_timeout_left;
int glb_is_send;
int glb_number_of_times_left;
int glb_ttl;
int debug_print_on = 0;

unsigned short csum(unsigned short *buf, int nwords) {
    unsigned long sum;
    for (sum = 0; nwords > 0; nwords--)
        sum += *buf++;
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    return (unsigned short)(~sum);
}

int create_raw_socket(int domain, int type, int protocol) {
    int sock = socket(domain, type, protocol);
    return sock;
}

int bind_raw_socket(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    return bind(sockfd, addr, addrlen);
}

void set_ip_header(struct iphdr *ip, struct udphdr *udp, u_int32_t dst_addr, int ttl){
    ip->ihl = 5;
    ip->version = 4;
    ip->tos = 0; // low delay
    ip->tot_len = sizeof(struct iphdr) + sizeof(struct udphdr) + N; 
    ip->id = htons(54322);
    ip->ttl = ttl;     // hops
    ip->protocol = 17; // UDP
    ip->saddr = 0;     //src_addr;
    ip->daddr = dst_addr;

    // fabricate the UDP header
    udp->source = htons(SOURCE_PORT);
    // destination port number
    udp->dest = htons(DESTINATION_PORT);
    udp->len = htons(sizeof(struct udphdr)+N);
}

void generate_random_payload(char *payload) {
    for(int i = 0; i < N; i++) {
        payload[i] = rand()%26 + 'a';
    }
    payload[N-1] = '\0';
}

void my_print_data(int ttl, char *ip_addr, float time_diff) {
    printf("%d\t%s\t%.3f ms\n", ttl, ip_addr, time_diff);
}

void my_print_star(int ttl){
    printf("%d\t*\t*\t*\t*\n", ttl);
}

int verification(u_int32_t a, u_int32_t b) {
    if(a == b) {
        return 1;
    }
    else {
        return 0;
    }
}

void store_prev_buffer(char *buffer, char *payload){
    for (int i=0; i < PCKT_LEN; i++) {
        prev_buffer[i] = '\0';
    }

    for (int i=0; i < PCKT_LEN; i++) {
        prev_buffer[i] = buffer[i];
    }
}

void store_prev_received(char *msg){
    for (int i=0; i < MAX_CHAR; i++) {
        prev_received[i] = '\0';
    }

    for (int i=0; i < MAX_CHAR; i++) {
        prev_received[i] = msg[i];
    }
}

void store_and_print_status(int timeout, int is_send, int times, int ttl, int position) {
    if(debug_print_on) {
        printf("Debug printing called from: %d", position);
        printf("%d\t%d\t%d\t%d ms\n", timeout, is_send, times, ttl);
    }

    glb_timeout_left = 1 - timeout;
    glb_is_send = is_send;
    glb_number_of_times_left = 3 - times;
    glb_ttl = ttl;
}

int main(int argc, char *argv[]) {

	// 1. Get the IP address from the domain name
	char *domain_name = argv[1];
	struct hostent *h;
	h = gethostbyname(domain_name);
	char *ip_address;

	if(h == NULL) {
		// printf("IP : %s\n", inet_ntoa(*(struct in_addr*)h->h_addr));
		printf("Invalid hostname from client\n");
		exit(0);
	}
	else {
		struct in_addr **addr_list;
		addr_list = (struct in_addr **)h->h_addr_list;
		if (addr_list[0] == NULL) {
			printf("Error in getting ip address\n");
        	exit(0);
		}
		ip_address = inet_ntoa(*((struct in_addr*) h->h_addr_list[0]));  // get the current ip address
	}

	// 2. Create two raw sockets and bind them to local IP
	int S1, S2;
    struct sockaddr_in saddr_raw, cli_addr;
    socklen_t saddr_raw_len;

    S1 = create_raw_socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
    if(S1 < 0) {
    	printf("[-] Error in creating raw socket S1\n");
    	exit(1);
    }

    S2 = create_raw_socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if(S2 < 0) {
    	close(S1);
    	printf("[-] Error in creating raw socket S2\n");
    	exit(1);
    }

    u_int32_t dst_addr;
    dst_addr = inet_addr(ip_address);
    saddr_raw.sin_family = AF_INET;
    saddr_raw.sin_port = htons(SOURCE_PORT);
    saddr_raw.sin_addr.s_addr = INADDR_ANY; //inet_addr(LISTEN_IP);
    saddr_raw_len = sizeof(saddr_raw);

    cli_addr.sin_family = AF_INET;
    cli_addr.sin_port = htons(DESTINATION_PORT);
    cli_addr.sin_addr.s_addr = dst_addr;

    int bind_return = bind_raw_socket(S1, (struct sockaddr *)&saddr_raw, saddr_raw_len);

    if(bind_return < 0) {
        perror("[-] Error in raw bind\n");
        close(S1);
        close(S2);
        exit(1);
    }

    printf("mytraceroute to %s (%s), %d hops max, %d byte packets\n", argv[1], ip_address, MAX_HOP, N);

    // 3. socket option on S1 to include IPHDR_INCL

    int opt = 1;
    if(setsockopt(S1, IPPROTO_IP, IP_HDRINCL, &opt, sizeof(opt)) < 0) {
        printf("[-] Error: setsockopt. You need to run this program as root\n");
        close(S1);
        close(S2);
        exit(1);
    }

    int ttl = 1, is_send = 1;
    int timeout = TIMEOUT;
    int times = 0;
    int position_val = 0;
    store_and_print_status(timeout, is_send, times, ttl, position_val);
    char payload[N];
    clock_t start_time;
    fd_set readSockSet;
    while(ttl <= MAX_HOP) {
    	char buffer[PCKT_LEN];
        struct iphdr *ip = (struct iphdr *)buffer;
        struct udphdr *udp = (struct udphdr *)(buffer + sizeof(struct iphdr));
        // 4. Send a UDP packet to traceroute using S1
        if(is_send) {
        	times++;
            int position_val = 1;
            store_and_print_status(timeout, is_send, times, ttl, position_val);
        	// generate random payload
            generate_random_payload(payload);

            bzero(buffer, PCKT_LEN);

            set_ip_header(ip, udp, dst_addr, ttl);

            ip->check = csum((unsigned short *)buffer, sizeof(struct iphdr) + sizeof(struct udphdr));
                
            strcpy(buffer + sizeof(struct iphdr) + sizeof(struct udphdr), payload);
            store_prev_buffer(buffer, payload);
            if (sendto(S1, buffer, ip->tot_len, 0, (struct sockaddr *)&cli_addr, sizeof(cli_addr)) < 0) {
                perror("[-] Error in sendto()");
                close(S1);
                close(S2);
                exit(1);
            }
            start_time = clock();
        }

        // 5. Wait on the select call
        FD_ZERO(&readSockSet);
        FD_SET(S2, &readSockSet);
        struct timeval tv;
        tv.tv_sec = timeout;
        tv.tv_usec = 0;
        int max_val = S2 + 1;
        int position_val = 2;
        store_and_print_status(timeout, is_send, times, ttl, position_val);
        int select_return = select(max_val, &readSockSet, 0, 0, &tv);
        if (select_return) {
        	if (FD_ISSET(S2, &readSockSet)) {
                // 6. Receival of ICMP message
                char msg[MAX_CHAR];
                bzero(msg, MAX_CHAR);
                int msglen;
                socklen_t raddr_len = sizeof(saddr_raw);
                msglen = recvfrom(S2, msg, MSG_SIZE, 0, (struct sockaddr *)&saddr_raw, &raddr_len);
                store_prev_received(msg);
                clock_t end_time = clock();
                if (msglen <= 0) {
                    timeout = TIMEOUT;
                    is_send = 1;
                    for (int i=0; i < PCKT_LEN; i++) {
                        prev_buffer[i] = '\0';
                    }
                    for (int i=0; i < MAX_CHAR; i++) {
                        prev_received[i] = '\0';
                    }
                    int position_val = 3;
                    store_and_print_status(timeout, is_send, times, ttl, position_val);
                    continue;
                }
                int position_val = 4;
                store_and_print_status(timeout, is_send, times, ttl, position_val);
                struct iphdr hdrip = *((struct iphdr *)msg);
                int iphdrlen = sizeof(hdrip);
                struct icmphdr hdricmp = *((struct icmphdr *)(msg + iphdrlen));
                struct in_addr saddr_ip;
                saddr_ip.s_addr = hdrip.saddr;
                if (hdrip.protocol == 1) {
                    verified = 0;
                    if (hdricmp.type == 11){
                        //time exceed
                        float time_diff = (float)(end_time - start_time) / CLOCKS_PER_SEC * 1000;
                        my_print_data(ttl,inet_ntoa(saddr_ip), time_diff);
                        ttl++;
                        times = 1;
                        timeout = TIMEOUT;
                        is_send = 1;
                        int position_val = 5;
                        store_and_print_status(timeout, is_send, times, ttl, position_val);
                        continue;
                    }
                    else if (hdricmp.type == 3) {
                        // verify
                        int res = verification(hdrip.saddr, ip->daddr);
                        if(res == 1){
                            verified = 1;
                        }
                        else {
                            verified = 0;
                        }
                        float time_diff = (float)(end_time - start_time) / CLOCKS_PER_SEC * 1000;
                        if (hdrip.saddr == ip->daddr) {
                            my_print_data(ttl,inet_ntoa(saddr_ip), time_diff);
                        }
                        close(S1);
                        close(S2);
                        return 0;
                    }
                }
                else {
                    is_send = 0;
                    timeout = end_time - start_time;
                    float comparator = 0.01;
                    if (timeout >= comparator){
                        continue;
                    }
                    else {
                        if (times > 3) {
                            my_print_star(ttl);
                            times = 1;
                            ttl++;
                        }
                        timeout = TIMEOUT;
                        is_send = 1;
                        int position_val = 6;
                        store_and_print_status(timeout, is_send, times, ttl, position_val);
                        continue;
                    }
                }
            }
        }
        else if (select_return == 0){
            if (times > 3) {
                my_print_star(ttl);
                times = 1;
                ttl++;
            }
            timeout = TIMEOUT;
            is_send = 1;
            int position_val = 7;
            store_and_print_status(timeout, is_send, times, ttl, position_val);
            continue;
        }
        else {
            perror("[-] Error in select()\n");
            close(S1);
            close(S2);
            exit(1);
        }
    }
    close(S1);
    close(S2);
	return 0;
}