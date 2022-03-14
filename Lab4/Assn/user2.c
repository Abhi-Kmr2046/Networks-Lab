#include "rsocket.h"

#define PORT1 50080
#define PORT2 50081

int main()
{
	int sockfd;
	struct sockaddr_in src_addr, dest_addr;
	sockfd = r_socket(AF_INET, SOCK_MRP, 0);
	if (sockfd < 0)
	{
		perror("Error in socket creation");
		exit(1);
	}

	memset(&src_addr, 0, sizeof(src_addr));

	src_addr.sin_family = AF_INET;
	src_addr.sin_addr.s_addr = INADDR_ANY;
	src_addr.sin_port = htons(PORT1);

	dest_addr.sin_family = AF_INET;
	dest_addr.sin_addr.s_addr = INADDR_ANY;
	dest_addr.sin_port = htons(PORT2);
	if (r_bind(sockfd, (const struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0)
	{
		perror("Bind error");
		exit(1);
	}
	printf("Receiver running\n");

	char input;
	int len;

	// char buffer[100] = {0};
	// len = sizeof(src_addr);
	// int valread = recvfrom(sockfd, (char *)buffer, 100, 0, (struct sockaddr *)&src_addr, &len);
	// buffer[valread] = '\0';
	// printf("Recieved: %s\n", buffer);

	while(1){
		len = sizeof(src_addr);
		r_recvfrom(sockfd,&input,1,0,(struct sockaddr *)&src_addr,&len);
		printf("---------------------------\n");
		printf("Received %c\n",input);
		fflush(stdout);
		printf("---------------------------\n");
	}
	r_close(sockfd);
	return 0;
=======
#include "rsocket.h"

#define PORT1 50080
#define PORT2 50081

int main()
{
	int sockfd;
	struct sockaddr_in src_addr, dest_addr;
	sockfd = r_socket(AF_INET, SOCK_MRP, 0);
	if (sockfd < 0)
	{
		perror("Error in socket creation");
		exit(1);
	}

	memset(&src_addr, 0, sizeof(src_addr));

	src_addr.sin_family = AF_INET;
	src_addr.sin_addr.s_addr = INADDR_ANY;
	src_addr.sin_port = htons(PORT1);

	dest_addr.sin_family = AF_INET;
	dest_addr.sin_addr.s_addr = INADDR_ANY;
	dest_addr.sin_port = htons(PORT2);
	if (r_bind(sockfd, (const struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0)
	{
		perror("Bind error");
		exit(1);
	}
	printf("Receiver running\n");

	char input;
	int len;

	char buffer[100] = {0};
	len = sizeof(src_addr);
	int valread = recvfrom(sockfd, (char *)buffer, 100, 0, (struct sockaddr *)&src_addr, &len);
	buffer[valread] = '\0';
	printf("Recieved: %s\n", buffer);

	// while(1){
	// 	len = sizeof(src_addr);
	// 	r_recvfrom(sockfd,&input,1,0,(struct sockaddr *)&src_addr,&len);
	// 	printf("---------------------------\n");
	// 	printf("Received %c\n",input);
	// 	fflush(stdout);
	// 	printf("---------------------------\n");
	// }
	r_close(sockfd);
	int hi;
	return 0;
}