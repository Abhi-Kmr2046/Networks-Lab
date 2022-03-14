#ifndef RSOCKET_H
#define RSOCKET_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// Macros required
#define SOCK_MRP 1
#define TABLE_SIZE 100
#define MESSAGE_SIZE 70
#define DROP_PROB 0.5
#define T_HALF_TIMEOUT 2
#define TIMEOUT_T_USEC 0

// Datatype for unacknowledged message
typedef struct
{
	int seq_no;
	char message[MESSAGE_SIZE];
	int msg_len;
	struct sockaddr_in dest_port;
	struct sockaddr_in dest_addr;
	struct timeval tv;
} unack_msg;

// Datatype for recieved message
typedef struct
{
	int seq_no;
	int msg_len;
	char message[MESSAGE_SIZE];
} recv_msg;

// Function prototypes
int r_socket(int domain, int type, int protocol);
int r_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
ssize_t r_sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
ssize_t r_recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
void *thread_R(void *param);
void *thread_S(void *param);
void sendAck(int id, int sockfd, const struct sockaddr *dest_addr);
int min(int a, int b);
int dropMessage(float p);
int r_close(int sfd);

#endif
