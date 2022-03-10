#include "rsocket.h"

#define PORT1 50080
#define PORT2 50081

int main()
{
    int sockfd;
    char inp[101];
    printf("Enter the input string\n");
    scanf("%s", inp);

    sockfd = r_socket(AF_INET, SOCK_MRP, 0);
    struct sockaddr_in src_addr, dest_addr;
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

    if (r_bind(sockfd, (const struct sockaddr *)&src_addr, sizeof(src_addr)) < 0)
    {
        perror("Bind error");
        exit(1);
    }

    printf("Sender running\n");
    int i, len = strlen(inp);
    for (i = 0; i < len; i++)
    {
        int ret = r_sendto(sockfd, &inp[i], 1, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (ret < 0)
        {
            perror("Send error");
            exit(1);
        }
    }

    r_close(sockfd);
}