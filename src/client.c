#include "client.h"

static uint8_t EOT = 0x04, ACK = 0x06, NAK = 0x15;

int clntfd;

void *tcp_client()
{
    while (RUNNING)
    {
        for (int i = 0; i < NUM_CHIMNEY; i++)
        {
            isData_to_send(i);
        }
        sleep(1);
    }

    return NULL;
}

void isData_to_send(int i)
{
    DATA_Q *q = &data_q[i];
    INFORM *ci = &info[i];

    while (!isEmpty(q))
    {
        MYSQL *conn = get_conn();
        SEND_Q *sq = dequeue(q);

        // if queue mem allocate failed, break
        if (sq == NULL)
        {
            release_conn(conn);
            break;
        }

        int send_result = send_data_to_server(sq, ci->sv_ip, ci->sv_port);
        if (send_result == 0 || send_result == -1)
        {
            if (send_result == 0)
                printf("Succeccfully send the message to the server...\n");
            else
                printf("Failed to send the message to the server...\n");

            // update database send column = 1(send complete)
            if (!(strncmp(sq->query_str, ";", 1) == 0))
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

int send_data_to_server(SEND_Q *q, char *IP, uint16_t PORT)
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
        ssize_t send_byte = send(clntfd, q->message, q->message_len, 0);
        if (send_byte < 0 && errno == EWOULDBLOCK) // 데이터 전송 타임아웃
        {
            printf("client-socket data transmit timeout(attempt %d)\n", attempt + 1);

            if (attempt == 1)
            {
                close(clntfd);
                return -1;
            }
            else
                continue;
        }
        else if (send_byte < 0) // 데이터 전송 실패
        {
            printf("client-socket data transmit failed...(attempt %d)\n", attempt + 1);
            if (attempt == 1)
            {
                close(clntfd);
                return -1;
            }
            else
                continue;
        }

        while (RUNNING)
        {
            uint8_t recv_buffer[BUFFER_SIZE];
            ssize_t recv_bytes = recv(clntfd, recv_buffer, BUFFER_SIZE, 0);
            if (recv_bytes > 0) // 데이터 수신 처리
            {
                if (recv_bytes == 1)
                {
                    if (recv_buffer[0] == ACK)
                    {
                        printf("client-socket received ACK\n");
                        send(clntfd, &EOT, 1, 0);
                        close(clntfd);

                        return 0;
                    }
                    else if (recv_buffer[0] == NAK)
                    {
                        printf("client-socket received NAK\n");

                        if (attempt == 1)
                        {
                            close(clntfd);
                            return -1;
                        }

                        break;
                    }
                    else
                    {
                        printf("client-socket received unknown data: %02X\n", recv_buffer[0]);

                        if (attempt == 1)
                        {
                            close(clntfd);
                            return -1;
                        }

                        break;
                    }
                }
                else
                {
                    printf("client-socket received byte: %ld\n", recv_bytes);
                }
            }
            else if (recv_bytes < 0 && errno == EWOULDBLOCK)
            {
                printf("client-socket data received timeout(attempt %d)\n", attempt + 1);
                if (attempt == 1)
                {
                    close(clntfd);
                    return -2;
                }

                break;
            }
            else if (recv_bytes <= 0)
            {
                printf("client-socket data recieved failed...(attempt %d)\n", attempt + 1);
                if (attempt == 1)
                {
                    close(clntfd);
                    return -2;
                }

                break;
            }
        }
    }

    // close the socket
    close(clntfd);
    return -1;
}

int handle_response(uint8_t *data, ssize_t byte_num)
{
    if (byte_num == 1)
    {
        if (data[0] == ACK)
        {
            printf("client-socket received ACK\n");
            return 0;
        }
        else if (data[0] == NAK)
        {
            printf("client-socket received NAK\n");
            return -1;
        }
        else
        {
            printf("client-socket received unknown data: %02X\n", data[0]);
            return -1;
        }
    }

    printf("client-socket received large byte: %ld\n", byte_num);
    return -1;
}