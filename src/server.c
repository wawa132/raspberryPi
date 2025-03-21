#include "server.h"

#define PORT 9090
#define SA struct sockaddr

int sockfd;

void *tcp_server()
{
    int connfd, len;
    struct sockaddr_in servaddr, cli;

    while (RUNNING)
    {
        // Socket create and verification
        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            printf("server-socket creation failed...\n");
            sleep(1);

            if (RUNNING)
                continue;
            else
                break;
        }
        else
        {
            printf("server-socket successfully created...\n");
        }
        bzero(&servaddr, sizeof(servaddr));

        // assign IP, PORT
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        servaddr.sin_port = htons(PORT);

        // Binding newly created socket to given IP and verification
        if (bind(sockfd, (SA *)&servaddr, sizeof(servaddr)) != 0)
        {
            printf("server-socket bind failed...\n");
            close(sockfd);
            sleep(1);

            if (RUNNING)
                continue;
            else
                break;
        }
        else
        {
            printf("server-socket successfully binded...\n");
        }

        // Now server is ready to listen and verification
        if ((listen(sockfd, 5)) != 0)
        {
            printf("server-socket listen failed...\n");
            close(sockfd);
            sleep(1);

            if (RUNNING)
                continue;
            else
                break;
        }
        else
        {
            printf("server-socket listening...\n");
        }

        len = sizeof(cli);

        // Accept the data packet from client and verification
        connfd = accept(sockfd, (SA *)&cli, (socklen_t *)&len);
        if (connfd < 0)
        {
            printf("server accept failed...\n");
            close(sockfd);

            if (RUNNING)
                continue;
            else
                break;
        }
        else
        {
            printf("server accept the client...\n");
        }

        // Accepted client socket setup timeout
        struct timeval timeout;
        timeout.tv_sec = 30;
        timeout.tv_usec = 0;

        if (setsockopt(connfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0)
        {
            printf("connected socket send timeout setup failed\n");
            close(sockfd);

            if (RUNNING)
                continue;
            else
                break;
        }
        else
        {
            printf("connected socket send timeout setup success.\n");
        }

        if (setsockopt(connfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
        {
            printf("connected socket recv timeout setup failed\n");
            close(sockfd);

            if (RUNNING)
                continue;
            else
                break;
        }
        else
        {
            printf("connected socket recv timeout setup success.\n");
        }

        // Function for process between client and server
        process_client_message(connfd);

        // After process close the socket
        close(sockfd);
    }

    return NULL;
}

void process_client_message(int connfd)
{
    unsigned char buff[BUFFER_SIZE];

    // set buffer to zero
    bzero(buff, BUFFER_SIZE);

    // read the message from client and copy it in buffer
    size_t readBytes = read(connfd, buff, BUFFER_SIZE);

    // print buffer which contains the client contents
    printf("From client: ");
    for (size_t i = 0; i < readBytes; i++)
    {
        printf("%c", buff[i]);
    }
    printf("\n");
}