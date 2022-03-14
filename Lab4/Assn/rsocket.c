#include "rsocket.h"

/* TODO:
Test mutex locks
How to close socket on receiver end

storing dest ip instead of dest port
not able to attach seq_no at beginning of sendto buffer
linear impleentation of table rather than circular
*/

// Global variables

// Receive message table - list of all message received till now
recv_msg *received_msg_table;
int msg_count = 0; // Number of msgs received - index for received_msg_id_table
int msg_to_print = 0;

// Unacknowledged message table
unack_msg *unack_msg_table;
int unack_msg_last = -1; // Index of last entered location in unack_msg_table

// Thread related variables
pthread_t rid, sid;
pthread_mutex_t mutex;
pthread_mutexattr_t mutex_ra, mutex_sa;

// Total transmissions done through the socket - Used to calculate metric
int num_tranmissions = 0;

int min(int a, int b)
{
	if (a < b)
		return a;
	else
		return b;
}

int dropMessage(float p)
{
	float rand_num = (float)rand() / ((float)RAND_MAX + 1);
	return rand_num < p;
}

// Function to create socket and initialise data structures
int r_socket(int domain, int type, int protocol)
{
	if (type != SOCK_MRP)
		return -1;

	int udp_fd;

	// Create a UDP socket
	if ((udp_fd = socket(domain, SOCK_DGRAM, protocol)) < 0)
	{
		perror("socket creation failed");
		return -1;
	}
	else
	{
		received_msg_table = (recv_msg *)calloc(TABLE_SIZE, sizeof(recv_msg));
		unack_msg_table = (unack_msg *)calloc(TABLE_SIZE, sizeof(unack_msg));
		for (int i = 0; i < TABLE_SIZE; i++)
		{
			received_msg_table[i].seq_no = -1;
			unack_msg_table[i].seq_no = -1; // -1 means id not presesnt
		}

		// Used for dropMessage function to generate random numbers
		srand(time(NULL));

		// Initialise data structures and thread R,S
		pthread_mutexattr_init(&mutex_ra);
		pthread_mutex_init(&mutex, &mutex_ra);
		int *param = (int *)malloc(sizeof(int));
		*param = udp_fd;

		int val = pthread_create(&rid, NULL, thread_R, param); // add attributes
		if (val < 0)
		{
			perror("Thread creation error");
			return -1;
		}

		// Initialise data structures and thread-X
		pthread_mutexattr_init(&mutex_sa);
		pthread_mutex_init(&mutex, &mutex_sa);
		*param = udp_fd;

		val = pthread_create(&sid, NULL, thread_S, param); // add attributes
		if (val < 0)
		{
			perror("Thread creation error");
			return -1;
		}

		return udp_fd;
	}
}
// Function to bind the socket to the port - similar to bind call
int r_bind(int udp_fd, const struct sockaddr *addr, socklen_t addrlen)
{
	return bind(udp_fd, addr, addrlen);
}

// Function to send messages.
ssize_t r_sendto(int udp_fd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen)
{
	// Message ids - taken as a static variable and updated for each message
	static unsigned int id_count = 0;
	id_count++; // increment message id
	// Initialise new buffer for appending id to the user message
	char *buffer = (char *)malloc(sizeof(char) * MESSAGE_SIZE);
	bzero(buffer, MESSAGE_SIZE);
	buffer[0] = 'M'; // indicate app msg, 'A' is used for ACK

	// Convert and append the msgid in network byte order
	int id_htonl = htonl(id_count);
	// printf("id=%d,=>%d\n", id_count, id_htonl);
	// strcat(buffer, id_htonl);
	// strcat(buffer, buf);

	// memcpy(&buffer[1], &id_htonl, sizeof(id_htonl));
	// memcpy(&buffer[sizeof(id_count) + 1], buf, len);
	memcpy(&buffer[1], buf, len);

	printf("Transmit %s,%s\n", buffer, buf);

	// -----------------lock
	pthread_mutex_lock(&mutex);
	unack_msg_last++; // increment index of unacknowledge message table
	unack_msg_table[unack_msg_last].seq_no = id_count;
	memcpy(unack_msg_table[unack_msg_last].message, buffer, len + sizeof(id_count) + 1);
	unack_msg_table[unack_msg_last].msg_len = len + sizeof(id_count) + 1;
	unack_msg_table[unack_msg_last].dest_addr = *(struct sockaddr_in *)dest_addr;
	gettimeofday(&unack_msg_table[unack_msg_last].tv, NULL);
	printf("Transmit\n");
	// Make the udp send call
	int nbytes_sent = sendto(udp_fd, buffer, len + sizeof(id_count) + 1, flags, dest_addr, addrlen);
	num_tranmissions++; // Increment number of sendto call count
	pthread_mutex_unlock(&mutex);
	// ------------------unlock

	if (nbytes_sent < 0)
	{
		perror("Unable to send");
	}
	// Return the number of bytes send -> similar to sendto call
	return nbytes_sent;
}

// Function to read data from the receive buffer
ssize_t r_recvfrom(int udp_fd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen)
{
	char *buffer = (char *)buf;
	//------------lock
	pthread_mutex_lock(&mutex);
	// If the buffer is empty
	while (msg_to_print == msg_count)
	{
		// ----------- temp unlock
		pthread_mutex_unlock(&mutex);
		sleep(1); // Sleep for 1 sec and then check again
		//------------ relock
		pthread_mutex_lock(&mutex);
	}
	// If buffer is non-empty
	// Deteremine the size of message to read - need to read either asked size len, or the message
	// length actually present, whichever is minimum
	int size = min(len, received_msg_table[msg_to_print].msg_len);
	// Copy the message into the buf present, passed by the user
	memcpy(buffer, received_msg_table[msg_to_print].message, size);

	msg_to_print = (msg_to_print + 1) % TABLE_SIZE; // Update the value of the buffer index.
	printf("r_recvfrom -> %s\n", buffer);
	// ------------unlock
	pthread_mutex_unlock(&mutex);
	// Return the size of message returned
	return size;
}

// Function for thread-X, takes the udp_fd value as parameter.
void *thread_R(void *param)
{
	// Typecase the provided parameter into udp_fd and free the pointer
	int udp_fd = *((int *)param);
	free(param);

	fd_set readfds;
	int nfds = udp_fd + 1;
	// Populate the timeout value
	struct timeval tv;
	tv.tv_sec = 10;
	tv.tv_usec = TIMEOUT_T_USEC;
	int ret_val;
	while (1)
	{
		// Wait on select call
		FD_ZERO(&readfds);
		FD_SET(udp_fd, &readfds);
		ret_val = select(nfds, &readfds, NULL, NULL, &tv);
		// If select comes out due to timeout
		if (ret_val != 0)
		{
			if (FD_ISSET(udp_fd, &readfds))
			{

				char buffer[110];
				bzero(buffer, 110);
				struct sockaddr_in src_addr;
				int len;
				len = sizeof(src_addr);

				// Receive message
				int msg_len = recvfrom(udp_fd, buffer, 110, 0, (struct sockaddr *)&src_addr, &len);

				// Compute probability for dropping
				if (dropMessage(DROP_PROB) != 0)
				{
					// Else do nothing
					printf("Dropping message\n");
					continue;
				}

				if (buffer[0] == 'M')
				{
					// HandleMessage(udp_fd,)
					int id;
					// Copy the id recieved
					memcpy(&id, &buffer[1], sizeof(id));
					id = ntohl(id);

					//---------------------lock
					pthread_mutex_lock(&mutex);
					// Copy the message to the receive msg table
					received_msg_table[msg_count].seq_no = id;
					received_msg_table[msg_count].msg_len = msg_len;
					memcpy(received_msg_table[msg_count].message, &buffer[sizeof(id) + 1], msg_len);
					pthread_mutex_unlock(&mutex);
					//------------------------------unlock

					msg_count++;
					// Send ack for the received message
					sendAck(id, udp_fd, (const struct sockaddr *)&src_addr);
				}
				else
				{
					int i;
					int id;
					memcpy(&id, &buffer[1], sizeof(id));
					id = ntohl(id);

					//----------------------lock
					pthread_mutex_lock(&mutex);
					for (i = 0; i <= unack_msg_last; i++)
					{
						if (unack_msg_table[i].seq_no == id)
						{
							printf("Received ack for message %d\n", id);
							// Swap the message to be deleted with the last message
							unack_msg_table[i] = unack_msg_table[unack_msg_last];
							// Decrement the last message, effectively deleting the acknowledged message
							unack_msg_last--;
							break;
						}
					}
					//----------------------unlock

					pthread_mutex_unlock(&mutex);
				}
			}
		}
	}
}

void *thread_S(void *param)
{
	int udp_fd = *((int *)param);
	free(param);

	while (1)
	{

		int i;
		struct timeval curr_time;
		gettimeofday(&curr_time, NULL);
		//----------------------lock
		pthread_mutex_lock(&mutex);
		// Loop over the unacknowledged messages.
		for (i = 0; i <= unack_msg_last; i++)
		{
			// If the message has timed out,
			if (curr_time.tv_sec - unack_msg_table[i].tv.tv_sec >= 2 * T_HALF_TIMEOUT)
			{
				// Retransmit the message
				if (sendto(udp_fd, unack_msg_table[i].message, unack_msg_table[i].msg_len, 0, (const struct sockaddr *)&unack_msg_table[i].dest_addr, sizeof(unack_msg_table[i].dest_addr)) < 0)
				{
					perror("Unable to send");
					exit(1);
				}
				// Update the time for the message
				unack_msg_table[i].tv.tv_sec = curr_time.tv_sec;
				unack_msg_table[i].tv.tv_usec = curr_time.tv_usec;
				num_tranmissions++;
			}
		}
		pthread_mutex_unlock(&mutex);
		//----------------------unlock
		sleep(T_HALF_TIMEOUT);
	}
}

void sendAck(int id, int udp_fd, const struct sockaddr *dest_addr)
{
	char buffer[sizeof(int) + 1];
	buffer[0] = 'A'; // Indicate message as an ACK
	int id_htonl = htonl(id);
	memcpy(&buffer[1], &id_htonl, sizeof(id_htonl));
	// call to udp send for sending the ack
	if (sendto(udp_fd, buffer, sizeof(id_htonl) + 1, 0, dest_addr, sizeof(*dest_addr)) < 0)
	{
		perror("Unable to send");
		exit(1);
	}
}

// Function to close the socket
int r_close(int udp_fd)
{
	printf("Here1\n");
	//----------------------lock
	pthread_mutex_lock(&mutex);
	// Check if there is any unacknowledged message left
	while (unack_msg_last >= 0)
	{
		pthread_mutex_unlock(&mutex);
		sleep(1);
		pthread_mutex_lock(&mutex);
	}
	// Print the number of transmissions before closing the socket
	printf("Total transmissions happened: %d\n", num_tranmissions);
	//------------------------------unlock
	pthread_mutex_unlock(&mutex);
	pthread_mutex_destroy(&mutex);
	free(received_msg_table);
	free(unack_msg_table);
	pthread_cancel(rid);
	pthread_cancel(sid);
	printf("Here2\n");
	return close(udp_fd);
}