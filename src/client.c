#include "client.h"

static uint8_t EOT = 0x04, ACK = 0x06, NAK = 0x15;

int clntfd = -1;

void *tcp_client()
{
    while (RUNNING)
    {
        // request data by server command 'pduh'
        if (request_data)
        {
            int no_chimney = request_data;
            for (int data_arr = 1; data_arr < 4; data_arr++)
            {
                DATA_Q *q = &(remote_q[no_chimney - 1][data_arr]);
                send_process(q, no_chimney);
            }

            request_data = 0; // clear
        }

        // normally send routine
        for (int no_chimney = 0; no_chimney < NUM_CHIMNEY; no_chimney++)
        {
            DATA_Q *q = &data_q[no_chimney];
            send_process(q, no_chimney);
        }

        sleep(1);
    }

    return NULL;
}

void send_process(DATA_Q *q, int no_chimney)
{
    INFORM *ipPort = &info[no_chimney];

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

        int send_result = send_data_to_server(sq, ipPort->sv_ip, ipPort->sv_port);
        if (send_result == 0 || send_result == -1)
        {
            if (send_result == 0)
                printf("Succeccfully send the message to the server...\n");
            else
                printf("Failed to send the message to the server...\n");

            // comlete send, update database send column = 1(send complete)
            if (!(strncmp(sq->query_str, ";", 1) == 0))
                execute_query(conn, sq->query_str);
        }
        else if (send_result == -2)
        {
            printf("Failed to send the message to server cause no response...\n");
        }
        else if (send_result == -3)
        {
            printf("Failed to setup the client-socket...\n");
        }

        // free mem allocate and release db connection
        free(sq);
        release_conn(conn);
    }

    if (clntfd != -1)
    {
        if (send(clntfd, &EOT, 1, 0) < 0)
            printf("client-socket send EOT fail...\n");

        exit_client_socket();
    }
}

int send_data_to_server(SEND_Q *q, char *IP, uint16_t PORT)
{
    struct sockaddr_in servaddr;

    if (clntfd == -1)
    {
        // 2 try for connection
        for (int con_attempt = 0; con_attempt < 2; con_attempt++)
        {
            // socket create and verification
            if ((clntfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            {
                printf("client-socket creation failed...\n");
                if (con_attempt == 1)
                    return -3;
                else
                {
                    sleep(1);
                    continue;
                }
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
                exit_client_socket();
                if (con_attempt == 1)
                    return -3;
                else
                {
                    sleep(1);
                    continue;
                }
            }

            // connect the client socket to server socket
            if (connect(clntfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0)
            {
                printf("client-socket connection with the server failed...\n");
                exit_client_socket();
                if (con_attempt == 1)
                    return -3;
                else
                {
                    sleep(1);
                    continue;
                }
            }
            else
            {
                printf("client-socket connect to the server...\n");
            }
        }

        // set timeout var
        struct timeval timeout;
        timeout.tv_sec = TIMEOUT;
        timeout.tv_usec = 0;

        // 30s timeout setup
        if (setsockopt(clntfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0)
        {
            printf("client-socket send timeout setup failed...\n");
            exit_client_socket();
            return -3;
        }
        if (setsockopt(clntfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
        {
            printf("client-socket receive timeout setup failed...\n");
            exit_client_socket();
            return -3;
        }
    }

    // send the message to server, if failed send try one more...
    for (int attempt = 0; attempt < 2; attempt++)
    {
        ssize_t send_byte = send(clntfd, q->message, q->message_len, 0);
        if (send_byte < 0 && errno == EWOULDBLOCK) // send timeout
        {
            printf("client-socket data transmit timeout(attempt %d)\n", attempt + 1);

            if (attempt == 1)
            {
                exit_client_socket();
                return -1;
            }
            else
                continue;
        }
        else if (send_byte < 0) // failed to send
        {
            printf("client-socket data transmit failed...(attempt %d)\n", attempt + 1);
            if (attempt == 1)
            {
                exit_client_socket();
                return -2;
            }
            else
                continue;
        }

        // check sent data
        check_sent_data(q->message, q->message_len);

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
                        printf("client-socket received ACK(attempt %d)\n", attempt + 1);
                        return 0;
                    }
                    else if (recv_buffer[0] == NAK)
                    {
                        printf("client-socket received NAK(attempt %d)\n", attempt + 1);
                        if (attempt == 1)
                        {
                            exit_client_socket();
                            return -1;
                        }

                        break;
                    }
                    else
                    {
                        printf("client-socket received unknown data: %02X(attempt %d)\n", recv_buffer[0], attempt + 1);
                        if (attempt == 1)
                        {
                            exit_client_socket();
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
                    exit_client_socket();
                    return -2;
                }

                break;
            }
            else if (recv_bytes <= 0)
            {
                printf("client-socket data recieved failed...(attempt %d)\n", attempt + 1);
                if (attempt == 1)
                {
                    exit_client_socket();
                    return -2;
                }

                break;
            }
        }
    }

    // close the socket
    exit_client_socket();
    return -1;
}

void exit_client_socket()
{
    close(clntfd);
    clntfd = -1;

    printf("client-socket closed...\n");
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

    if (byte_num > 2)
    {
        uint16_t rcvCrc = data[byte_num - 2] << 8 | data[byte_num - 1];
        uint16_t chkCRC = crc16_ccitt_false(data, (size_t)(byte_num - 2));

        if (rcvCrc != chkCRC)
        {
            printf("client-socket crc mismatch: received(%d), calculated(%d)\n", rcvCrc, chkCRC);
            printf("client-socket received message(%ld): ", byte_num);
            for (ssize_t i = 0; i < byte_num - 2; i++)
            {
                printf("%c", data[i]);
            }
            printf("\n");

            return -1;
        }
        else
        {
            printf("client-socket crc : received(%d), calculated(%d)\n", rcvCrc, chkCRC);

            char cmd[5], business_code[8], chimney_code[4], total_len[5], server_time[13];

            memcpy(cmd, data, 4);
            cmd[4] = '\0';
            memcpy(business_code, data + 4, 7);
            business_code[7] = '\0';
            memcpy(chimney_code, data + 11, 3);
            chimney_code[3] = '\0';
            memcpy(total_len, data + 14, 4);
            total_len[4] = '\0';
            memcpy(server_time, data + 18, 12);
            server_time[12] = '\0';

            int length = atoi(total_len);

            if (length == 32 && strncmp(cmd, "PTIM", 4) == 0)
            {
                int check = 0;
                for (int i = 0; i < NUM_CHIMNEY; i++)
                {
                    CHIMNEY *c = &chimney[i];
                    if (strncmp(business_code, c->business, 7) == 0 && strncmp(chimney_code, c->chimney, 3) == 0)
                    {
                        struct tm received_time;
                        int year, mon, day, hour, min, sec;
                        if (sscanf(server_time, "%02d%02d%02d%02d%02d%02d",
                                   &year, &mon, &day, &hour, &min, &sec) == 6)
                        {
                            received_time.tm_year = (year + 2000) - 1900;
                            received_time.tm_mon = mon - 1;
                            received_time.tm_mday = day;
                            received_time.tm_hour = hour;
                            received_time.tm_min = min;
                            received_time.tm_sec = sec;
                            received_time.tm_isdst = -1;

                            printf("received server time: %4d-%02d-%02d %02d:%02d:%02d\n",
                                   received_time.tm_year + 1900, received_time.tm_mon + 1, received_time.tm_mday,
                                   received_time.tm_hour, received_time.tm_min, received_time.tm_sec);

                            time_t system_t = time(NULL);
                            struct tm system_time = *localtime(&system_t);
                            printf("current system time: %4d-%02d-%02d %02d:%02d:%02d\n",
                                   system_time.tm_year + 1900, system_time.tm_mon + 1, system_time.tm_mday,
                                   system_time.tm_hour, system_time.tm_min, system_time.tm_sec);

                            time_t systemTime = mktime(&system_time);
                            time_t serverTime = mktime(&received_time);

                            if (difftime(serverTime, systemTime) > FIVSEC || difftime(systemTime, serverTime) > FIVSEC)
                            {
                                // set system time

                                // 서버 시간이 5분이상 빠른 경우 tofh 데이터 생성
                                if (difftime(serverTime, systemTime) > FIVSEC)
                                {
                                    if (check_off(off_time + 1))
                                    {
                                        if (pthread_create(&thread[5], NULL, tofh_thread, (TOFH_TIME *)off_time + 1) < 0)
                                            printf("Failed to create tofh thread: %s 굴뚝\n", chimney_code);
                                    }
                                }
                            }
                            break;
                        }
                        else
                            printf("server time sscanf 실패: %s\n", server_time);
                    }
                    else
                    {
                        printf("code mismatch: recv business-code(%s) | business-code(%s), recv chimney-code(%s)|chimney-code(%s)\n",
                               business_code, c->business, chimney_code, c->chimney);
                        check++;
                    }
                }

                if (check >= NUM_CHIMNEY)
                {
                    printf("PTIM recv error: business-code(%s), chimney-code(%s)\n", business_code, chimney_code);
                    return -1;
                }

                return 0;
            }
            else
            {
                printf("PTIM recv error: command(%s/PTIM) or length(%d/32)\n", cmd, length);
                return -1;
            }
        }
    }

    printf("client-socket received wrong protocol(%ld): ", byte_num);
    for (ssize_t i = 0; i < byte_num; i++)
    {
        printf("%c", (char)data[i]);
    }
    printf("\n");
    return -1;
}