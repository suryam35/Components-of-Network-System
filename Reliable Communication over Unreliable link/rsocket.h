/*
	Group Number - 5
	Group Member 1 - Suryam Arnav Kalra (19CS30050)
	Group Member 2 - Kunal Singh (19CS30025)
*/

#ifndef _RSOCKET_H
#define _RSOCKET_H

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>

#define SOCK_MRP 5
#define T 2
#define p 0.20
#define MSG_SIZE 100
#define TABLE_SIZE 100


int r_socket(int, int, int);
int r_bind(int, const struct sockaddr *, socklen_t);
int r_sendto(int, const void *, size_t, int, const struct sockaddr *, socklen_t);
int r_recvfrom(int, char *, size_t, int, struct sockaddr *, socklen_t *);
int r_close(int);
void *runnerR(void *);
void *runnerS(void *);
int dropMessage(float);

#endif
