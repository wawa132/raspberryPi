#include "server.h"

#define PORT 9090
#define SA struct sockaddr

int servfd = -1, connfd;
uint8_t request_data, ftp_chimney, upgrade, reboot_flag;
static uint8_t EOT = 0x04, ACK = 0x06, NAK = 0x15;
char ftp_type[2], ftp_ipDomain[41], ftp_port[6], ftp_path[51], ftp_id[11], ftp_passwd[11], ftp_servIP[16];

void *tcp_server()
{
    int total_len;
    int optval = 1;
    struct sockaddr_in servaddr, cli;

    while (RUNNING)
    {
        // Socket create and verification
        if ((servfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
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
        if (bind(servfd, (SA *)&servaddr, sizeof(servaddr)) != 0)
        {
            printf("server-socket bind failed...\n");
            exit_server_socket();

            if (RUNNING)
                continue;
            else
                break;
        }
        else
        {
            printf("server-socket successfully binded...\n");
        }

        // Reuse socket option
        if (setsockopt(servfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
        {
            perror("SO_REUSEADDR 설정 실패");
            exit_server_socket();

            if (RUNNING)
                continue;
            else
                break;
        }

        // Now server is ready to listen and verification
        if ((listen(servfd, 5)) != 0)
        {
            printf("server-socket listen failed...\n");
            exit_server_socket();

            if (RUNNING)
                continue;
            else
                break;
        }
        else
        {
            printf("server-socket listening...\n");
        }

        // Accept the data packet from client and verification
        total_len = sizeof(cli);
        connfd = accept(servfd, (SA *)&cli, (socklen_t *)&total_len);
        if (connfd < 0)
        {
            printf("server accept failed...\n");
            exit_server_socket();

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
        timeout.tv_sec = TIMEOUT;
        timeout.tv_usec = 0;

        if (setsockopt(connfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0)
        {
            printf("connected socket send timeout setup failed\n");
            exit_server_socket();
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
            exit_server_socket();

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
        exit_server_socket();

        sleep(1);
    }

    return NULL;
}

void exit_server_socket()
{
    close(servfd);
    servfd = -1;

    printf("server-socket closed...\n");
}

void process_client_message(int connfd)
{
    while (RUNNING)
    {
        unsigned char buff[BUFFER_SIZE];
        bzero(buff, BUFFER_SIZE); // set buffer to zero

        ssize_t byte_num = recv(connfd, buff, BUFFER_SIZE, 0);
        if (byte_num > 0)
        {
            if (byte_num == 1)
            {
                if (buff[0] == 0x04)
                    printf("server-socket recv EOT by client\n");
                else if (buff[0] == 0x06)
                    printf("server-socket recv ACK by client\n");
                else if (buff[0] == 0x15)
                    printf("server-socket recv NAK by client\n");
                else
                    printf("server-socket recv unknown data: %02X\n", buff[0]);

                break;
            }

            if (byte_num < 4)
            {
                printf("server-socket recv not enough bytes\n");
                break;
            }

            int result = recv_handle_data(buff, byte_num);
            if (result > 0)
            {
                send(connfd, &ACK, 1, 0);
                printf("server-socket send ACK to client\n");
            }
            else if (result == 0)
            {
                send(connfd, &EOT, 1, 0);
                printf("server-socket send EOT to client\n");
                break;
            }
            else
            {
                send(connfd, &NAK, 1, 0);
                printf("server-socket send NAK to client\n");
            }
        }
        else if (byte_num < 0 && errno == EWOULDBLOCK)
        {
            printf("server-socket recv timeout by client\n");
            break;
        }
        else
        {
            printf("server-socket recv failed %ld bytes\n", byte_num);
            break;
        }
    }
}

int recv_handle_data(const unsigned char *recvData, ssize_t byte_num)
{

    // check recv data
    check_recv_data(recvData, (size_t)byte_num);

    // CRC check
    uint16_t rcvCrc = recvData[byte_num - 2] << 8 | recvData[byte_num - 1];
    uint16_t chkCRC = crc16_ccitt_false(recvData, (size_t)(byte_num - 2));
    if (rcvCrc != chkCRC)
    {
        printf("server-socket crc mismatch: received(%d), calculated(%d)\n", rcvCrc, chkCRC);
        return -1;
    }

    // 명령어, 사업장코드, 굴뚝코드, 전체길이 추출
    char command[5], business_code[8], chimney_code[4], len[5];
    memcpy(command, recvData, 4);
    command[4] = '\0';
    memcpy(business_code, recvData + 4, 7);
    business_code[7] = '\0';
    memcpy(chimney_code, recvData + 11, 3);
    chimney_code[3] = '\0';
    memcpy(len, recvData + 14, 4);
    len[4] = '\0';

    // 전체길이 int형으로 변환
    int total_len = atoi(len);

    // 사업장 코드 확인, 굴뚝 코드 확인
    int check = 0;
    if (strncmp(chimney[0].business, business_code, 7) == 0)
    {
        for (int i = 0; i < NUM_CHIMNEY; i++)
        {
            if (strncmp(chimney[i].chimney, chimney_code, 3) == 0)
                break;

            check++;
        }

        if (check >= NUM_CHIMNEY)
        {
            printf("server-socket doesn't match chimney code %s\n", chimney_code);
            return -1;
        }
    }
    else
    {
        printf("server-socket doesn't match business code %s\n", business_code);
        return -1;
    }

    // 명령어 확인
    if (strncmp(command, "PDUH", 4) == 0 && total_len == 43)
    {
        return pduh_process(recvData, check);
    }
    else if (strncmp(command, "PFST", 4) == 0 && total_len == 24)
    {
        return pfst_process(recvData, check);
    }
    else if (strncmp(command, "PSEP", 4) == 0 && total_len == 36)
    {
        return psep_process(recvData, check);
    }
    else if (strncmp(command, "PUPG", 4) == 0 && total_len >= 164)
    {
        printf("recv number of byte for upgrade request: %ld\n", byte_num);
        return pupg_process(recvData, check);
    }
    else if (strncmp(command, "PVER", 4) == 0 && total_len == 20)
    {
        return pver_process(recvData, check);
    }
    else if (strncmp(command, "PSET", 4) == 0 && total_len == 32)
    {
        return pset_process(recvData, check);
    }
    else if (strncmp(command, "PFCC", 4) == 0 && total_len == 30)
    {
        return pfcc_process(recvData, check);
    }
    else if (strncmp(command, "PAST", 4) == 0 && total_len >= 46)
    {
        return past_process(recvData, check);
    }
    else if (strncmp(command, "PFCR", 4) == 0 && total_len == 20)
    {
        return pfcr_process(recvData, check);
    }
    else if (strncmp(command, "PFRS", 4) == 0 && total_len >= 32)
    {
        return pfrs_process(recvData, check);
    }
    else if (strncmp(command, "PRSI", 4) == 0 && total_len == 36)
    {
        return prsi_process(recvData, check);
    }
    else if (strncmp(command, "PDAT", 4) == 0 && total_len == 21)
    {
        return pdat_process(recvData, check);
    }
    else if (strncmp(command, "PODT", 4) == 0 && total_len == 26)
    {
        return podt_process(recvData, check);
    }
    else if (strncmp(command, "PCN2", 4) == 0 && total_len == 20)
    {
        return send_tcn2(check, 1);
    }
    else if (strncmp(command, "PRBT", 4) == 0 && total_len == 20)
    {
        return prbt_process(recvData, check);
    }

    printf("server-socket doesn't match command(%s) or total length(%d)\n", command, total_len);
    return -1;
}

int pduh_process(const uint8_t *recvData, int no_chimney) // n+1은 굴뚝번호
{
    char beginTime_str[11], endTime_str[11], dataType_str[4];

    memcpy(dataType_str, recvData + 18, 3);
    dataType_str[3] = '\0';
    memcpy(beginTime_str, recvData + 21, 10);
    beginTime_str[10] = '\0';
    memcpy(endTime_str, recvData + 31, 10);
    endTime_str[10] = '\0';

    // 자료 구분
    int data_type;
    if (strncmp(dataType_str, "HAF", 3) == 0)
        data_type = 0;
    else if (strncmp(dataType_str, "ALL", 3) == 0)
        data_type = 1;
    else if (strncmp(dataType_str, "FIV", 3) == 0)
        data_type = 2;

    // 시작- 종료 일시 설정
    struct tm begin_time, end_time;
    int year, mon, day, hour, min;
    if (sscanf(beginTime_str, "%02d%02d%02d%02d%02d",
               &year, &mon, &day, &hour, &min) == 5)
    {
        begin_time.tm_year = year + 100;
        begin_time.tm_mon = mon - 1;
        begin_time.tm_mday = day;
        begin_time.tm_hour = hour;
        begin_time.tm_min = min;
        begin_time.tm_sec = 0;
    }
    else
        return -1;

    printf("PDUH begin time: %4d-%02d-%02d %02d:%02d\n",
           begin_time.tm_year + 1900, begin_time.tm_mon + 1, begin_time.tm_mday, begin_time.tm_hour, begin_time.tm_min);

    if (sscanf(endTime_str, "%02d%02d%02d%02d%02d",
               &year, &mon, &day, &hour, &min) == 5)
    {
        end_time.tm_year = year + 100;
        end_time.tm_mon = mon - 1;
        end_time.tm_mday = day;
        end_time.tm_hour = hour;
        end_time.tm_min = min;
        end_time.tm_sec = 0;
    }
    else
        return -1;

    printf("PDUH end time: %4d-%02d-%02d %02d:%02d\n",
           end_time.tm_year + 1900, end_time.tm_mon + 1, end_time.tm_mday, end_time.tm_hour, end_time.tm_min);

    time_t startTime, endTime;
    startTime = mktime(&begin_time);
    endTime = mktime(&end_time);

    // 요청 기간 저장 데이터 큐에 담기
    enqueue_load_to_transmit(startTime, endTime, no_chimney, data_type);

    DATA_Q *q = &(remote_q[no_chimney][0]);
    while (!isEmpty(q))
    {
        SEND_Q *sq = dequeue(q);
        if (sq == NULL)
            break;

        send_response(sq->message, sq->message_len);
        free(sq);
        usleep(10000); // 10msec delay
    }

    request_data = no_chimney + 1;
    return 0;
}

int pfst_process(const uint8_t *recvData, int no_chimney)
{
    char resendTime_str[5], query_str[300];

    memcpy(resendTime_str, recvData + 18, 4);
    resendTime_str[4] = '\0';

    int resend_time = atoi(resendTime_str);
    if (resend_time > 2359)
        resend_time = 9999;

    chimney[no_chimney].time_resend = resend_time;
    snprintf(query_str, sizeof(query_str),
             "UPDATE t_chimney SET t_resend = %d WHERE chimney = \'%03d\';",
             chimney[no_chimney].time_resend, no_chimney + 1);

    if (chimney[no_chimney].time_resend == 9999)
        printf("miss data send by next data send time(9999)\n");
    else
        printf("change resend data time -> %02d:%02d\n",
               chimney[no_chimney].time_resend / 100, chimney[no_chimney].time_resend % 100);

    MYSQL *conn = get_conn();
    execute_query(conn, query_str);
    release_conn(conn);

    return send_tcn2(no_chimney, 0);
}

int psep_process(const uint8_t *recvData, int no_chimney)
{
    char decPassword_str[17];
    uint8_t encPassword_str[16], dec_buffer[16];

    memcpy(encPassword_str, recvData + 18, 16);
    printf("PSEP recved encPassword: ");
    for (int i = 0; i < 16; i++)
    {
        printf("%02X", encPassword_str[i]);
    }
    printf("\n");

    // Decrypt
    int decrypt_bytes = SEED_CBC_Decrypt(encPassword_str, sizeof(encPassword_str), dec_buffer);

    int i;
    for (i = 0; i < decrypt_bytes; i++)
    {
        decPassword_str[i] = (char)dec_buffer[i];
    }
    decPassword_str[i] = '\0';

    // 공백 제거
    int j = 0;
    for (i = 0; decPassword_str[i] != '\0'; i++)
    {
        if (decPassword_str[i] != ' ')
        {
            decPassword_str[j++] = decPassword_str[i];
        }
    }
    decPassword_str[j] = '\0';

    strcpy(info[no_chimney].passwd, decPassword_str); // change DB gw password column
    printf("PSEP recv decPassword: %s\n", info[no_chimney].passwd);

    char query_str[300];
    snprintf(query_str, sizeof(query_str), "UPDATE t_info SET encPassword_str = \'%s\'", info[no_chimney].passwd);

    MYSQL *conn = get_conn();
    execute_query(conn, query_str);
    release_conn(conn);

    return send_tcn2(no_chimney, 0);
}

int pupg_process(const uint8_t *recvData, int no_chimney)
{
    uint8_t enc_sftp[144], dec_sftp[144];

    ftp_chimney = no_chimney;

    memcpy(enc_sftp, recvData + 18, 144);
    printf("sftp connect information(hex): ");
    for (int i = 0; i < 144; i++)
    {
        printf("%02X", enc_sftp[i]);
    }
    printf("\n");

    int length = SEED_CBC_Decrypt(enc_sftp, 144, dec_sftp);
    if (length != 131)
    {
        printf("not enough decrypt bytes(131): %d\n", length);
        return -1;
    }

    memcpy(ftp_type, dec_sftp, 1);
    ftp_type[1] = '\0';
    memcpy(ftp_ipDomain, dec_sftp + 1, 40);
    ftp_ipDomain[40] = '\0';
    memcpy(ftp_port, dec_sftp + 41, 5);
    ftp_port[5] = '\0';
    memcpy(ftp_path, dec_sftp + 46, 50);
    ftp_path[50] = '\0';
    memcpy(ftp_id, dec_sftp + 96, 10);
    ftp_id[10] = '\0';
    memcpy(ftp_passwd, dec_sftp + 106, 10);
    ftp_passwd[10] = '\0';
    memcpy(ftp_servIP, dec_sftp + 116, 15);
    ftp_servIP[15] = '\0';

    printf("(FTP information) ftp_type: %s | ftp_IP/Domain: %s | ftp_port: %s | filePath: %s | ftp_ID: %s | ftp_passwd: %s | server_IP: %s\n",
           ftp_type, ftp_ipDomain, ftp_port, ftp_path, ftp_id, ftp_passwd, ftp_servIP);

    trim_space(ftp_ipDomain);
    trim_space(ftp_port);
    trim_space(ftp_path);
    trim_space(ftp_id);
    trim_space(ftp_passwd);
    trim_space(ftp_servIP);

    printf("(FTP information trim) ftp_type: %s | ftp_IP/Domain: %s | ftp_port: %s | filePath: %s | ftp_ID: %s | ftp_passwd: %s | server_IP: %s\n",
           ftp_type, ftp_ipDomain, ftp_port, ftp_path, ftp_id, ftp_passwd, ftp_servIP);

    // ftpIP/Domain 추출
    // "sftp://greenlink.or.kr 또는 unknown_prefix://119.269.53.2 인 경우 "//" 제거"
    // VPN 이어서, IP 주소를 사용하지 않을까?

    char *ftpIP = (char *)malloc(sizeof(char) * 200);
    if (ftpIP == NULL)
    {
        printf("failed to allocate memory for ftp ip\n");
        return -1;
    }

    memcpy(ftpIP, ftp_ipDomain, strlen(ftp_ipDomain) + 1);

    char *withoutPrefixFtpIP = ftpIP; // 원본 포인터는 그대로 두고 새로운 변수로 처리
    withoutPrefixFtpIP = strstr(withoutPrefixFtpIP, "//");
    if (withoutPrefixFtpIP != NULL)
    {
        withoutPrefixFtpIP += 2; // "//" 이후로 이동
        printf("delete \'//\' complete: %s\n", withoutPrefixFtpIP);

        memcpy(ftpIP, withoutPrefixFtpIP, strlen(withoutPrefixFtpIP));
    }
    else
    {
        // "//"를 찾지 못한 경우, 원본 문자열을 그대로 사용
        printf("there is no \'//\': %s\n", ftpIP);
    }

    // ftp 주소가 IP_Address인지 Domain인지 확인
    printf("SFTP IP/Domain address confirm\n");
    if (Is_IpAddress(ftpIP))
    {
        printf("SFTP IP address confirm\n");

        strcpy(ftp_ipDomain, ftpIP);
        printf("IpAddress type: %s\n", ftp_ipDomain);
    }
    else
    {
        printf("ip address extract from domain\n");

        struct hostent *host_info;
        host_info = gethostbyname(ftpIP);

        if (host_info == NULL)
        {
            fprintf(stderr, "failed to extract ip address from domain: %s\n", ftpIP);
            upgrade = false;

            free(ftpIP);
            return -1;
        }
        else
        {
            memcpy(ftp_ipDomain, host_info->h_addr_list[0], sizeof(struct in_addr));
            printf("ip address(%s) was extracted by %s\n", ftpIP, ftp_ipDomain);
        }
    }

    upgrade = true;

    free(ftpIP);
    return 1;
}

int pset_process(const uint8_t *recvData, int no_chimney)
{
    char setTime_str[13];
    struct tm set_time;

    memcpy(setTime_str, recvData + 18, 12);
    setTime_str[12] = '\0';

    int year, mon, day, hour, min, sec;
    if (sscanf(setTime_str, "%02d%02d%02d%02d%02d%02d",
               &year, &mon, &day, &hour, &min, &sec) == 6)
    {
        set_time.tm_year = (year + 2000) - 1900;
        set_time.tm_mon = mon - 1;
        set_time.tm_mday = day;
        set_time.tm_hour = hour;
        set_time.tm_min = min;
        if (sec == 0)
            sec = 1;
        set_time.tm_sec = sec;
        set_time.tm_isdst = -1;

        printf("PSET recv time: %4d-%02d-%02d %02d:%02d:%02d\n",
               set_time.tm_year + 1900, set_time.tm_mon + 1, set_time.tm_mday,
               set_time.tm_hour, set_time.tm_min, set_time.tm_sec);

        time_t system_t = time(NULL); // current system time
        struct tm system_time = *localtime(&system_t);

        printf("current system time: %4d-%02d-%02d %02d:%02d:%02d\n",
               system_time.tm_year + 1900, system_time.tm_mon + 1, system_time.tm_mday,
               system_time.tm_hour, system_time.tm_min, system_time.tm_sec);

        time_change = 1;

        // prevent time process overlapped
        pthread_mutex_lock(&time_mtx);

        while (processing)
            pthread_cond_wait(&time_cond, &time_mtx);

        // set system time
        set_system_time(set_time, 1);

        pthread_mutex_unlock(&time_mtx);
        sec_checker = true;
        min_checker = true;

        // time_t systemTime, setTime;
        time_t systemTime = mktime(&system_time);
        time_t setTime = mktime(&set_time);

        if (difftime(setTime, systemTime) > FIVSEC)
        {
            if (check_off(off_time + 1))
            {
                if (pthread_create(&thread[6], NULL, tofh_thread, (TOFH_TIME *)off_time + 1) < 0)
                    printf("Failed to create tofh thread: %d chimney\n", no_chimney + 1);
            }
        }
        return 1;
    }
    else
    {
        printf("PSET received time string failed to parsing time-date\n");
        return -1;
    }
}

int pver_process(const uint8_t *recvData, int no_chimney)
{
    return send_tver(recvData, no_chimney);
}

int pfcc_process(const uint8_t *recvData, int no_chimney)
{
    char old_facility[6], new_facility[6];

    memcpy(old_facility, recvData + 18, 5);
    old_facility[5] = '\0';
    memcpy(new_facility, recvData + 23, 5);
    new_facility[5] = '\0';

    CHIMNEY *c = &chimney[no_chimney];

    MYSQL *conn = get_conn();

    int num_old_fac = 0; // 변경하려는 시설코드를 가진 기존 시설의 갯수
    for (int i = 0; i < c->num_facility; i++)
    {
        FACILITY *f = &(c->facility[i]);

        if (strncmp(new_facility, f->name, 5) == 0)
        {
            printf("PFCC already exists facility code.\n");
            release_conn(conn);
            return -1; // NAK return
        }

        if (strncmp(old_facility, f->name, 5) == 0)
        {
            num_old_fac++;

            strcpy(f->name, new_facility);
            printf("change facility code %s -> %s\n", old_facility, f->name);

            char query_str[300];
            snprintf(query_str, sizeof(query_str),
                     "UPDATE t_facility SET fac = \'%s\' WHERE chim_id = %d AND fac = \'%5s\';",
                     f->name, no_chimney + 1, old_facility);

            execute_query(conn, query_str);
        }
    }

    if (num_old_fac < 1)
    {
        printf("PFCC there is no facility for changing code\n");
        release_conn(conn);
        return -1;
    }

    release_conn(conn);

    return send_tcn2(no_chimney, 0); // TCN2 전송
}

int past_process(const uint8_t *recvData, int no_chimney)
{
    char facilityItem_str[3], query_str[300];

    memcpy(facilityItem_str, recvData + 18, 2);
    facilityItem_str[2] = '\0';

    CHIMNEY *c = &chimney[no_chimney];

    int items = atoi(facilityItem_str);
    int next = 20;

    MYSQL *conn = get_conn(); // 데이터베이스 연결 획득

    for (int i = 0; i < items; i++)
    {
        char facility_code[6], item_code[2], min_str[7], max_str[7], std_str[7];

        memcpy(facility_code, recvData + (next), 5);
        facility_code[5] = '\0';
        memcpy(item_code, recvData + (next + 5), 1);
        item_code[1] = '\0';
        memcpy(min_str, recvData + (next + 6), 6);
        min_str[6] = '\0';
        memcpy(max_str, recvData + (next + 12), 6);
        max_str[6] = '\0';
        memcpy(std_str, recvData + (next + 18), 6);
        std_str[6] = '\0';

        int check = 0;
        for (int j = 0; j < c->num_facility; j++)
        {
            FACILITY *f = &(c->facility[j]);

            if (strncmp(facility_code, f->name, 5) == 0 && strncmp(item_code, f->item, 1) == 0)
            {
                int min = atoi(min_str);
                int max = atoi(max_str);
                int std = atoi(std_str);

                f->range.min = min * 100;
                f->range.max = max * 100;
                f->range.std = std * 100;

                snprintf(query_str, sizeof(query_str), "UPDATE t_facility SET min = %d, max = %d, std = %d WHERE id = %d AND fac = \'%5s\' AND item = \'%s\';",
                         f->range.min, f->range.max, f->range.std, j + 1, f->name, f->item);

                printf("%s\n", query_str);

                execute_query(conn, query_str);
                break;
            }

            check++;
        }

        if (check >= c->num_facility)
        {
            printf("there is no matched facility(%s) or item(%s)\n", facility_code, item_code);
            release_conn(conn);
            return -1;
        }

        next += 24;
    }

    release_conn(conn);
    return send_tcn2(no_chimney, 0);
}

int pfcr_process(const uint8_t *recvData, int no_chimney)
{
    uint8_t buffer[BUFFER_SIZE];
    char tempBuffer[BUFFER_SIZE], tempNewBuffer[300], query_str[300], business_code[8], chimney_code[4];

    memcpy(business_code, recvData + 4, 7);
    business_code[7] = '\0';
    memcpy(chimney_code, recvData + 11, 3);
    chimney_code[3] = '\0';

    snprintf(tempBuffer, sizeof(tempBuffer), "TFCR%7s%3s", business_code, chimney_code);

    snprintf(query_str, sizeof(query_str), "SELECT DISTINCT CONCAT(t1.fac, (SELECT t2.fac FROM t_facility AS t2 WHERE t2.id = t1.rel_fac AND t2.chim_id = %d)) AS combined_fac FROM t_facility AS t1 WHERE t1.chim_id = %d AND t1.rel_fac != 0;", no_chimney + 1, no_chimney + 1);

    MYSQL *conn = get_conn();
    execute_query(conn, query_str);

    MYSQL_RES *res = mysql_store_result(conn);
    int rows = mysql_num_rows(res);
    if (rows < 1)
        return -1;

    size_t length = 22 + (rows * 10);
    snprintf(tempNewBuffer, sizeof(tempNewBuffer), "%4ld%2d", length, rows);
    strcat(tempBuffer, tempNewBuffer);

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)))
    {
        strcpy(tempNewBuffer, row[0]);
        strcat(tempBuffer, tempNewBuffer);
    }

    mysql_free_result(res);
    release_conn(conn);

    printf("%s\n", tempBuffer);

    convert_ascii_to_hex(tempBuffer, buffer, 0, length - 2);

    uint16_t crc = crc16_ccitt_false((uint8_t *)tempBuffer, length - 2);
    buffer[length - 2] = (crc >> 8) & 0xff;
    buffer[length - 1] = crc & 0xff;

    return send_response(buffer, length);
}

int pfrs_process(const uint8_t *recvData, int no_chimney)
{
    char facilityItem_str[3], query_str[300], tempBuffer[BUFFER_SIZE], tempNewBuffer[300];
    uint8_t buffer[BUFFER_SIZE];

    memcpy(facilityItem_str, recvData + 18, 2);
    facilityItem_str[2] = '\0';

    CHIMNEY *c = &chimney[no_chimney];

    int items = atoi(facilityItem_str);
    int next = 20;

    MYSQL *conn = get_conn(); // 데이터베이스 연결 획득

    for (int i = 0; i < items; i++)
    {
        char emission_code[6], prevention_code[6];
        memcpy(emission_code, recvData + (next), 5);
        emission_code[5] = '\0';
        memcpy(prevention_code, recvData + (next + 5), 5);
        prevention_code[5] = '\0';

        int numEmission = 0, numPrevention = 0;

        for (int j = 0; j < c->num_facility; j++)
        {
            FACILITY *f = &(c->facility[j]);
            if (strncmp(emission_code, f->name, 5) == 0)
            {
                numEmission++;
            }
            else if (strncmp(prevention_code, f->name, 5) == 0)
            {
                numPrevention++;
            }
        }

        // 관계를 변경할 시설코드가 존재하지 않음.
        if (numEmission == 0 || numPrevention == 0)
        {
            printf("there is no facility for changing relations(emission: %d ea, prevention: %d ea)\n", numEmission, numPrevention);
            release_conn(conn);
            return -1;
        }

        for (int j = 0; j < c->num_facility; j++)
        {
            FACILITY *f = &(c->facility[j]);

            if (strncmp(emission_code, f->name, 5) == 0)
            {
                for (int k = 0; k < c->num_facility; k++)
                {
                    FACILITY *pf = &(c->facility[k]);

                    if (strncmp(prevention_code, pf->name, 5) == 0)
                    {
                        f->rel_facility = k + 1;

                        snprintf(query_str, sizeof(query_str), "UPDATE t_facility SET rel_fac = %d WHERE chim_id = %d AND fac = \'%5s\';", f->rel_facility, no_chimney + 1, f->name);

                        execute_query(conn, query_str);
                        numEmission--;

                        if (numEmission < 0)
                            break;
                    }
                }
            }
        }
        next += 10;
    }

    char business_code[8], chimney_code[4];

    memcpy(business_code, recvData + 4, 7);
    business_code[7] = '\0';
    memcpy(chimney_code, recvData + 11, 3);
    chimney_code[3] = '\0';

    snprintf(tempBuffer, sizeof(tempBuffer), "TFCR%7s%3s", business_code, chimney_code);

    snprintf(query_str, sizeof(query_str), "SELECT DISTINCT CONCAT(t1.fac, (SELECT t2.fac FROM t_facility AS t2 WHERE t2.id = t1.rel_fac AND t2.chim_id = %d)) AS combined_fac FROM t_facility AS t1 WHERE t1.chim_id = %d AND t1.rel_fac != 0;", no_chimney + 1, no_chimney + 1);

    execute_query(conn, query_str);
    MYSQL_RES *res = mysql_store_result(conn);

    int rows = mysql_num_rows(res);
    size_t length = 22 + (rows * 10);

    snprintf(tempNewBuffer, sizeof(tempNewBuffer), "%4ld%2d", length, rows);
    strcat(tempBuffer, tempNewBuffer);

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)))
    {
        strcpy(tempNewBuffer, row[0]);
        strcat(tempBuffer, tempNewBuffer);
    }

    mysql_free_result(res);
    release_conn(conn);

    convert_ascii_to_hex(tempBuffer, buffer, 0, length - 2);
    uint16_t crc = crc16_ccitt_false(buffer, length - 2);
    buffer[length - 2] = (crc >> 8) & 0xff;
    buffer[length - 1] = crc & 0xff;

    enqueue(&data_q[no_chimney], buffer, length, ";");
    return 1;
}

int prsi_process(const uint8_t *recvData, int no_chimney)
{
    uint8_t encIP[16];
    memcpy(encIP, recvData + 18, 16);

    uint8_t decIP[32];
    int decrypt_bytes = SEED_CBC_Decrypt(encIP, 16, decIP);

    char ipStr[16];
    int i = 0, j = 0;
    for (i = 0; i < decrypt_bytes; i++)
    {
        ipStr[i] = (char)decIP[i];
    }
    ipStr[i] = '\0';

    for (i = 0; ipStr[i] != '\0'; i++)
    {
        if (ipStr[i] != ' ')
            ipStr[j++] = ipStr[i];
    }
    ipStr[j] = '\0';

    INFORM *_i = &info[no_chimney];
    printf("PRSI ip address change %s -> %s\n", _i->sv_ip, ipStr);

    strcpy(_i->sv_ip, ipStr);

    char query_str[300];
    snprintf(query_str, sizeof(query_str), "UPDATE t_info SET sv_ip = \'%s\';", _i->sv_ip);

    MYSQL *conn = get_conn(); // 데이터베이스 연결 획득
    execute_query(conn, query_str);
    release_conn(conn); // 데이터베이스 연결 해제

    return send_tcn2(no_chimney, 0);
}

int pdat_process(const uint8_t *recvData, int no_chimney)
{
    char sendMode_str[2];
    memcpy(sendMode_str, recvData + 18, 1);
    sendMode_str[1] = '\0';

    CHIMNEY *c = &chimney[no_chimney];

    int send_mode = atoi(sendMode_str);
    printf("PDAT send mode change %d -> %d\n", c->send_mode, send_mode);

    c->send_mode = send_mode;
    char query_str[300];
    snprintf(query_str, sizeof(query_str), "UPDATE t_chimney SET send_mode = %d WHERE chimney = \'%3s\';", c->send_mode, c->chimney);

    MYSQL *conn = get_conn();
    execute_query(conn, query_str);
    release_conn(conn);

    return send_tcn2(no_chimney, 0);
}

int podt_process(const uint8_t *recvData, int no_chimney)
{
    char on_delay[4], of_delay[4];

    memcpy(on_delay, recvData + 18, 3);
    on_delay[3] = '\0';
    memcpy(of_delay, recvData + 21, 3);
    of_delay[3] = '\0';

    CHIMNEY *c = &chimney[no_chimney];

    int onDelay = atoi(on_delay);
    int ofDelay = atoi(of_delay);

    printf("PODT change ond %d -> %d | ofd %d -> %d\n", c->delay.ond, onDelay, c->delay.ofd, ofDelay);

    c->delay.ond = onDelay;
    c->delay.ofd = ofDelay;

    for (int i = 0; i < c->num_facility; i++)
    {
        FACILITY *f = &(c->facility[i]);
        f->delay.ond = onDelay / 5;
        f->delay.ofd = ofDelay / 5;
    }

    char query_str[300];
    snprintf(query_str, sizeof(query_str), "UPDATE t_chimney SET onDelay = %d, ofDelay = %d WHERE chimney = \'%3s\';", c->delay.ond, c->delay.ofd, c->chimney);

    MYSQL *conn = get_conn();
    execute_query(conn, query_str);
    release_conn(conn);

    return send_tcn2(no_chimney, 0);
}

int prbt_process(const uint8_t *recvData, int no_chimney)
{
    char business_code[8], chimney_code[4], query_str[300];

    memcpy(business_code, recvData + 4, 7);
    business_code[7] = '\0';
    memcpy(chimney_code, recvData + 11, 3);
    chimney_code[3] = '\0';

    CHIMNEY *c = &chimney[no_chimney];

    MYSQL *conn = get_conn();

    snprintf(query_str, sizeof(query_str), "UPDATE t_chimney SET reboot = 1 WHERE chimney = \'%3s\';", c->chimney);
    printf("PRBT request reboot %s(business) %s(chimney)): %s\n", business_code, chimney_code, query_str);

    execute_query(conn, query_str);
    release_conn(conn);

    reboot_flag = true;
    return 1;
}

/**
 * Type 0: Enqueue -> return 1
 * Type 1: Send
 * */
int send_tcn2(int no_chimney, int type)
{
    // n+1: 굴뚝번호, type: 접속상태에서 전송(0), 접속해제 후 나중 전송(1)
    uint8_t buffer[BUFFER_SIZE], enc_svIP[16], enc_gwIP[16], enc_pass[16], hashCode[32];
    uint8_t header[18], bodyPart1[42], bodyPart2[13];
    char tempBuffer[BUFFER_SIZE], tempNewBuffer[1500]; // ASCII 코드 담는 변수

    CHIMNEY *c = &chimney[no_chimney];
    INFORM *_i = &info[no_chimney];

    // 암호화
    snprintf(tempBuffer, sizeof(tempBuffer), "%15s", _i->sv_ip);
    int encByte = SEED_CBC_Encrypt((BYTE *)tempBuffer, (int)(strlen(tempBuffer)), enc_svIP);
    if (encByte != 16)
    {
        printf("Failed to enc_svIP\n");
        return -1;
    }

    snprintf(tempBuffer, sizeof(tempBuffer), "%15s", _i->gw_ip);
    encByte = SEED_CBC_Encrypt((BYTE *)tempBuffer, (int)(strlen(tempBuffer)), enc_gwIP);
    if (encByte != 16)
    {
        printf("Failed to enc_gwIP\n");
        return -1;
    }

    snprintf(tempBuffer, sizeof(tempBuffer), "%10s", _i->passwd);
    encByte = SEED_CBC_Encrypt((BYTE *)tempBuffer, (int)(strlen(tempBuffer)), enc_pass);
    if (encByte != 16)
    {
        printf("Failed to enc_pass\n");
        return -1;
    }

    // 해시코드
    size_t hash_length = strlen(_i->hash) / 2;
    if (hash_length % 2 != 0)
    {
        printf("hash string is odd.\n");
        return -1;
    }

    for (size_t i = 0; i < hash_length; i++)
    {
        sscanf(_i->hash + (2 * i), "%2hhx", &hashCode[i]);
    }

    size_t length = 155 + (24 * c->num_facility);

    // 헤더: 명령어(4), 사업장코드(7), 굴뚝코드(3), 전체길이(4)
    snprintf(tempBuffer, sizeof(tempBuffer), "TCN2%7s%3s%4ld",
             c->business, c->chimney, length);
    convert_ascii_to_hex(tempBuffer, header, 0, strlen(tempBuffer));

    // 바디 Part_1: 제조사코드(2), 모델명(20), 버전(20)
    snprintf(tempBuffer, sizeof(tempBuffer), "%2s%20s%20s",
             _i->manufacture_code, _i->model, _i->version);
    convert_ascii_to_hex(tempBuffer, bodyPart1, 0, strlen(tempBuffer));

    // 바디 Part_2: 미전송자료 전송시간(4), 자료전송모드(1), 가동유예시간(3), 중지유예시간(3), 항목수(2)
    snprintf(tempBuffer, sizeof(tempBuffer), "%04d%1d%3d%3d%2d",
             c->time_resend, c->send_mode, c->delay.ond, c->delay.ofd, c->num_facility);
    convert_ascii_to_hex(tempBuffer, bodyPart2, 0, strlen(tempBuffer));

    // 바디 part_3: 시설코드(5), 항목코드(1), 최소값(6), 최대값(6), 기준값(6)
    size_t bodyPart3_len = 24 * c->num_facility;
    uint8_t *bodyPart3 = malloc(bodyPart3_len);
    if (bodyPart3 == NULL)
    {
        printf("TCN2 bodyPart3 memory allocate failed\n");
        return -1;
    }
    memset(tempBuffer, '\0', BUFFER_SIZE);
    for (int i = 0; i < c->num_facility; i++)
    {
        FACILITY *fac = &(c->facility[i]);

        int raw_min, raw_max, raw_std;
        int abs_min, abs_max, abs_std;
        char fac_min[30], fac_max[30], fac_std[30];

        raw_min = fac->range.min / 100;
        raw_max = fac->range.max / 100;
        raw_std = fac->range.std / 100;

        if (raw_min < 0)
        {
            abs_min = abs(fac->range.min);

            if (raw_min < 10)
                snprintf(fac_min, sizeof(fac_min), "    -%1d", abs_min / 100);
            else if (raw_min < 100)
                snprintf(fac_min, sizeof(fac_min), "   -%2d", abs_min / 100);
            else
                snprintf(fac_min, sizeof(fac_min), "   -99");
        }
        else
            snprintf(fac_min, sizeof(fac_min), "%6d", fac->range.min / 100);

        if (raw_max < 0)
        {
            abs_max = abs(fac->range.max);

            if (raw_max < 10)
                snprintf(fac_max, sizeof(fac_max), "    -%1d", abs_max / 100);
            else if (raw_max < 100)
                snprintf(fac_max, sizeof(fac_max), "   -%2d", abs_max / 100);
            else
                snprintf(fac_max, sizeof(fac_max), "   -99");
        }
        else
            snprintf(fac_max, sizeof(fac_max), "%6d", fac->range.max / 100);

        if (raw_std < 0)
        {
            abs_std = abs(fac->range.std);

            if (raw_std < 10)
                snprintf(fac_std, sizeof(fac_std), "    -%1d", abs_std / 100);
            else if (raw_std < 100)
                snprintf(fac_std, sizeof(fac_std), "   -%2d", abs_std / 100);
            else
                snprintf(fac_std, sizeof(fac_std), "   -99");
        }
        else
            snprintf(fac_std, sizeof(fac_std), "%6d", fac->range.std / 100);

        snprintf(tempNewBuffer, sizeof(tempNewBuffer), "%5s%1s%6s%6s%6s", fac->name, fac->item, fac_min, fac_max, fac_std);
        strcat(tempBuffer, tempNewBuffer);
    }
    convert_ascii_to_hex(tempBuffer, bodyPart3, 0, strlen(tempBuffer));

    // 전송할 메세지 담기
    memcpy(buffer, header, sizeof(header));             // 18
    memcpy(buffer + 18, enc_svIP, sizeof(enc_svIP));    // 16
    memcpy(buffer + 34, enc_gwIP, sizeof(enc_gwIP));    // 16
    memcpy(buffer + 50, bodyPart1, sizeof(bodyPart1));  // 42
    memcpy(buffer + 92, hashCode, sizeof(hashCode));    // 32
    memcpy(buffer + 124, enc_pass, sizeof(enc_pass));   // 16
    memcpy(buffer + 140, bodyPart2, sizeof(bodyPart2)); // 13
    memcpy(buffer + 153, bodyPart3, bodyPart3_len);     // bodyPart3_len

    free(bodyPart3);

    printf("TCN2 check\n");
    for (size_t i = 0; i < length - 2; i++)
    {
        if (i < 18)
        {
            if (i == 0)
                printf("[header: ");
            printf("%c", buffer[i]);
        }
        else if (i < 34)
        {
            if (i == 18)
                printf("]\n[svIP: ");
            printf("%02x", buffer[i]);
        }
        else if (i < 50)
        {
            if (i == 34)
                printf("]\n[gwIP: ");
            printf("%02x", buffer[i]);
        }
        else if (i < 92)
        {
            if (i == 50)
                printf("]\n[manufacture Code: ");
            else if (i == 52)
                printf("]\n[gw Model: ");
            else if (i == 72)
                printf("]\n[fw Version: ");

            printf("%c", buffer[i]);
        }
        else if (i < 124)
        {
            if (i == 92)
                printf("]\n[hash: ");
            printf("%02x", buffer[i]);
        }
        else if (i < 140)
        {
            if (i == 124)
                printf("]\n[password: ");
            printf("%02x", buffer[i]);
        }
        else
        {
            if (i == 140)
                printf("]\n[pfst time: ");
            else if (i == 144)
                printf("]\n[send mode: ");
            else if (i == 145)
                printf("]\n[emission delay-time: ");
            else if (i == 148)
                printf("]\n[prevention delay-time: ");
            else if (i == 151)
                printf("]\n[item: ");
            else if (i == 153)
                printf("]\n[");

            printf("%c", buffer[i]);
        }
    }
    printf("]\n");

    // CRC
    uint16_t crc = crc16_ccitt_false(buffer, length - 2);
    buffer[length - 2] = (crc >> 8) & 0xff;
    buffer[length - 1] = crc & 0xff;

    if (type == 0)
    {
        enqueue(&data_q[no_chimney], buffer, length, ";");
        return 1;
    }
    else
        return send_response(buffer, length);
}

int send_tver(const uint8_t *recvData, int no_chimney)
{
    uint8_t buffer[BUFFER_SIZE], enc_svIP[16], enc_gwIP[16], hashCode[32];
    uint8_t header[18], bodyPart1[42];
    char tempBuffer[BUFFER_SIZE]; // ASCII 코드 담는 변수
    size_t length = 126;

    INFORM *_i = &info[no_chimney];

    // 암호화
    snprintf(tempBuffer, sizeof(tempBuffer), "%15s", _i->sv_ip);
    int encByte = SEED_CBC_Encrypt((BYTE *)tempBuffer, (int)(strlen(tempBuffer)), enc_svIP);
    if (encByte != 16)
    {
        printf("Failed to enc_svIP\n");
        return 0;
    }

    snprintf(tempBuffer, sizeof(tempBuffer), "%15s", _i->gw_ip);
    encByte = SEED_CBC_Encrypt((BYTE *)tempBuffer, (int)(strlen(tempBuffer)), enc_gwIP);
    if (encByte != 16)
    {
        printf("Failed to enc_gwIP\n");
        return 0;
    }

    // 해시코드
    size_t hash_length = strlen(_i->hash) / 2;
    if (hash_length % 2 != 0)
    {
        printf("hash string is odd\n");
        return 0;
    }

    for (size_t i = 0; i < hash_length; i++)
    {
        sscanf(_i->hash + (2 * i), "%2hhx", &hashCode[i]);
    }

    char business_code[8], chimney_code[4];
    memcpy(business_code, recvData + 4, 7);
    business_code[7] = '\0';
    memcpy(chimney_code, recvData + 11, 3);
    chimney_code[3] = '\0';

    // 헤더: 명령어(4), 사업장코드(7), 굴뚝코드(3), 전체길이(4)
    snprintf(tempBuffer, sizeof(tempBuffer), "TVER%7s%3s%4ld", business_code, chimney_code, length);
    convert_ascii_to_hex(tempBuffer, header, 0, strlen(tempBuffer));

    // 바디 Part_1: 제조사코드(2), 모델명(20), 버전(20)
    snprintf(tempBuffer, sizeof(tempBuffer), "%2s%20s%20s",
             _i->manufacture_code, _i->model, _i->version);
    convert_ascii_to_hex(tempBuffer, bodyPart1, 0, strlen(tempBuffer));

    // 전송할 메세지 담기
    memcpy(buffer, header, sizeof(header));            // 18
    memcpy(buffer + 18, enc_svIP, sizeof(enc_svIP));   // 16
    memcpy(buffer + 34, enc_gwIP, sizeof(enc_gwIP));   // 16
    memcpy(buffer + 50, bodyPart1, sizeof(bodyPart1)); // 42
    memcpy(buffer + 92, hashCode, sizeof(hashCode));   // 32

    // CRC 생성
    uint16_t crc = crc16_ccitt_false(buffer, length - 2);
    buffer[length - 2] = (crc >> 8) & 0xff;
    buffer[length - 1] = crc & 0xff;

    // 체크리스트용 변수
    printf("TVER check\n");
    for (size_t i = 0; i < length - 2; i++)
    {
        if (i < 18)
        {
            if (i == 0)
                printf("[header: ");
            printf("%c", buffer[i]);
        }
        else if (i < 34)
        {
            if (i == 18)
                printf("]\n[svIP: ");
            printf("%02x", buffer[i]);
        }
        else if (i < 50)
        {
            if (i == 34)
                printf("]\n[gwIP: ");
            printf("%02x", buffer[i]);
        }
        else if (i < 92)
        {
            if (i == 50)
                printf("]\n[manufacture Code: ");
            else if (i == 52)
                printf("]\n[gw Model: ");
            else if (i == 72)
                printf("]\n[fw Version: ");

            printf("%c", buffer[i]);
        }
        else
        {
            if (i == 92)
                printf("]\n[hash: ");
            printf("%02x", buffer[i]);
        }
    }
    printf("]\n");

    return send_response(buffer, length);
}

int send_response(const uint8_t *data, int data_len)
{
    for (int attempt = 0; attempt < 2; attempt++)
    {
        // response send
        ssize_t sent_byte = send(connfd, data, data_len, 0);
        if (sent_byte < 0 && errno == EWOULDBLOCK)
        {
            printf("server-socket %dsec send timeout(attempt %d)\n", TIMEOUT, attempt + 1);
            if (attempt == 1)
                return 0;
            else
                continue;
        }
        else if (sent_byte < 0)
        {
            printf("sever-socket failed to response(attempt %d)\n", attempt + 1);
            if (attempt == 1)
                return 0;
            else
                continue;
        }

        // check respose data
        check_sent_data(data, data_len);

        unsigned char recv_buffer[BUFFER_SIZE];
        memset(recv_buffer, 0, BUFFER_SIZE);

        ssize_t recv_bytes = recv(connfd, recv_buffer, BUFFER_SIZE, 0);
        if (recv_bytes > 0)
        {
            if (recv_bytes > 4)
                check_recv_data(recv_buffer, (size_t)recv_bytes);

            if (recv_bytes == 1)
            {
                if (recv_buffer[0] == ACK)
                {
                    printf("server-socket recv ACK by client(attempt %d)\n", attempt + 1);
                    return 0;
                }
                else if (recv_buffer[0] == NAK)
                    printf("(server-socket recv NAK by client(attempt %d)\n", attempt + 1);
                else
                    printf("(server-socket recv unknown data: %02X(attempt %d)\n", recv_buffer[0], attempt + 1);

                if (attempt == 1)
                    return 0;
            }
        }
        else if (recv_bytes < 0 || errno == EWOULDBLOCK)
        {
            printf("server-socket recv %dsec timeout(attempt %d)\n", TIMEOUT, attempt + 1);
            if (attempt == 1)
                return 0;
        }
        else if (recv_bytes <= 0)
        {
            printf("server-socket recv failed(attempt %d)\n", attempt + 1);
            if (attempt == 1)
                return 0;
        }
    }

    return 0;
}

void trim_space(char *buffer)
{
    int i = 0, j = 0;

    for (i = 0; buffer[i] != '\0'; i++)
    {
        if (buffer[i] != ' ')
        {
            buffer[j++] = buffer[i];
        }
    }

    buffer[j] = '\0';
}

int Is_IpAddress(const char *address)
{
    if (address == NULL || strlen(address) == 0)
    {
        printf("address is NULL or no string.\n");
        return 0;
    }

    struct sockaddr_in socket_addr;
    return inet_pton(AF_INET, address, &(socket_addr.sin_addr)) != 0;
}

void check_recv_data(const uint8_t *data, size_t data_len)
{
    char command[5];

    memcpy(command, data, 4);
    command[4] = '\0';

    printf("server-socket recv data(%ld): ", data_len);

    if (strncmp(command, "PSEP", 4) == 0 || strncmp(command, "PRSI", 4) == 0)
    {
        for (size_t i = 0; i < data_len - 2; i++)
        {
            if (i < 18)
                printf("%c", data[i]);
            else
                printf("%02x", data[i]);
        }
    }
    else if (strncmp(command, "PUPG", 4) == 0)
    {
        for (size_t i = 0; i < data_len - 2; i++)
        {
            if (i < 18)
                printf("%c", data[i]);
            else
                printf("%02x", data[i]);
        }
    }
    else
    {
        for (size_t i = 0; i < data_len - 2; i++)
        {
            printf("%c", data[i]);
        }
    }

    printf("\n");
}