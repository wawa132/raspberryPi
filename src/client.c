#include "client.h"

static uint8_t EOT = 0x04, ACK = 0x06, NAK = 0x15;

int clntfd;

void *tcp_client()
{
    while (RUNNING)
    {
        for (int i = 0; i < NUM_CHIMNEY; i++)
        {
            isMessageToSend(i);
        }

        sleep(1);
    }

    return NULL;
}

void isMessageToSend(int i)
{
    DATA_Q *q = &data_q[i];
    INFORM *ci = &info[i];

    while (!isEmpty(q))
    {
        if (q->cnt == 0)
        {
            return;
        }
        else
        {
            MYSQL *conn = get_conn();
            SEND_Q *sq = dequeue(q);

            // if queue mem allocate failed, goto first code
            if (sq == NULL)
            {
                release_conn(conn);
                continue;
            }

            int send_result = send_message_to_server(sq, ci->sv_ip, ci->sv_port);
            if (send_result == 0 || send_result == -1)
            {
                if (send_result == 0)
                    printf("Succeccfully send the message to the server...\n");
                else
                    printf("Failed to send the message to the server...\n");

                // update database send column = 1(send complete)
                execute_query(conn, sq->query_str);
            }
            else if (send_result == -2)
            {
                printf("Failed to send the message to server cause no response...\n");
            }
            else if (send_result == -3)
            {
                printf("Failed to send the message to server cause Wrong socket setup...\n");
            }

            // free mem allocate and release db connection
            free(sq);
            release_conn(conn);
        }
    }
}

int send_message_to_server(DATA_Q *q, char *IP, uint16_t PORT)
{
    struct sockaddr_in servaddr;

    // socket create and verification
    if ((clntfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("client-socket creation failed...\n");
        return -3;
    }
    else
    {
        printf("client-socket successfully created...\n");
    }
    bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, IP, &servaddr.sin_addr) <= 0)
    {
        printf("client-socket assign IP failed...(Invalid IP)\n");
        close(clntfd);

        return -3;
    }

    // connect the client socket to server socket
    if (connect(clntfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0)
    {
        printf("client-socket connection with the server failed...\n");
        close(clntfd);

        return -1;
    }
    else
    {
        printf("client-socket to the server...\n");
    }

    // set timeout var
    struct timeval timeout;
    timeout.tv_sec = 30;
    timeout.tv_usec = 0;

    // 30s timeout setup
    if (setsockopt(clntfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0)
    {
        printf("client-socket send timeout setup failed...\n");
        close(clntfd);

        return -3;
    }

    if (setsockopt(clntfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
    {
        printf("client-socket receive timeout setup failed...\n");
        close(clntfd);

        return -3;
    }

    // send the message to server, if failed send try one more...
    for (int attempt = 0; attempt < 2; attempt++)
    {
        /* code */
    }

    // close the socket
    close(clntfd);

    return 0;
}