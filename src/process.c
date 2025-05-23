#include "process.h"

int32_t sum_cnt, sec_value[MAX_NUM], sum_value[MAX_NUM],
    fiv_value[MAX_NUM], haf_value[MAX_NUM];

time_t produce_time = 0;
const int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

void *make_data()
{
    while (RUNNING)
    {
        while (!isProcessEmpty(&planter))
        {
            MINER *miner = process_dequeue(&planter);
            if (miner == NULL)
            {
                if (isProcessEmpty(&planter))
                    continue;
                else
                    break;
            }

            produce_time = miner->datetime;
            process_5sec(&produce_time); // 5초 데이터 생성

            if (produce_time % FIVSEC == 0)
            {
                process_5min(&produce_time); // 5분 데이터 생성
                update_tdah(&produce_time, 1, 0);

                if (produce_time % HAFSEC == 0)
                {
                    process_30min(&produce_time, 0); // 30분 데이터 생성
                    update_tdah(&produce_time, 0, 0);
                    update_tnoh(&produce_time, 0);
                }

                enqueue_tdah_to_transmit(&produce_time, 0);
            }

            // 일일마감 데이터 확인
            struct tm day_time = *localtime(&produce_time);
            if (day_time.tm_hour == 0 && day_time.tm_min == 5 && day_time.tm_sec == 0)
            {
                // 매일 0시 2분 0초에 일일마감 데이터 생성
                process_day(&produce_time);
                update_tddh(&produce_time, 1); // 5분 일일마감 생성
                update_tddh(&produce_time, 0); // 30분 일일마감 생성

                enqueue_tddh_to_transmit(&produce_time);
            }

            // 시간 동기화 확인
            if (day_time.tm_hour == SYNC_TIME && day_time.tm_min == 0 && day_time.tm_sec == 0)
            {
                char buffer[300];
                uint8_t sendBuffer[300];

                size_t length = 20;

                CHIMNEY *c = &chimney[0];
                DATA_Q *q = &data_q[0];

                snprintf(buffer, sizeof(buffer), "TTIM%7s%3s  20", c->business, c->chimney);
                convert_ascii_to_hex(buffer, sendBuffer, 0, length - 2);

                uint16_t crc = crc16_ccitt_false((uint8_t *)buffer, length - 2);
                sendBuffer[length - 2] = (crc >> 8) & 0xff;
                sendBuffer[length - 1] = crc & 0xff;

                enqueue(q, sendBuffer, length, ";");
            }

            free(miner);   // free memory
            usleep(10000); // 10msec delay
        }
        sleep(1);
    }

    return NULL;
}

void process_5sec(time_t *datetime)
{
    update_5sec_data(); // 5초 데이터 업데이트

    update_5sec_state(); // 5초 상태정보 업데이트

    update_5sec_relation(datetime); // 5초 관계정보 업데이트
}

void update_5sec_data()
{
    for (int i = 0; i < MAX_NUM; i++)
    {
        sec_value[i] = sensor_data[i] * 100; // 0.00 단위로 x100
        sum_value[i] += sec_value[i];
    }
    sum_cnt++;
}

void update_5sec_state()
{
    for (int i = 0; i < NUM_CHIMNEY; i++)
    {
        CHIMNEY *c = &chimney[i];

        for (int j = 0; j < c->num_facility; j++)
        {
            FACILITY *f = &(c->facility[j]);
            int32_t data = sec_value[f->port - 1]; // 센서 1번 -> 센서[0]

            if (f->check == 1) // 점검중
            {
                f->SEC.stat = 8;
                f->SEC.value = data;
            }
            else
            {
                if (data >= 66600) // 통신불량
                {
                    f->SEC.stat = 2;
                    f->SEC.value = 0;
                }
                else
                {
                    if (data < f->range.min) // 비정상범위
                    {
                        f->SEC.stat = 1;
                        f->SEC.value = data;
                    }
                    else // 정상, 정상범위 이상인 경우 최대값 표기
                    {
                        f->SEC.stat = 0;

                        if (f->SEC.value >= f->range.max)
                            f->SEC.value = f->range.max;
                        else
                            f->SEC.value = data;
                    }
                }
            }

            if (f->SEC.stat == 0 && f->SEC.value >= f->range.std) // 가동-미가동
                f->SEC.run = 1;
            else
                f->SEC.run = 0;
        }
    }
}

void update_5sec_relation(time_t *datetime)
{
    char query_str[BUFFER_SIZE], insertTime_str[300];

    struct tm *insert_time = localtime(datetime);

    snprintf(insertTime_str, sizeof(insertTime_str), "%4d-%02d-%02d %02d:%02d:%02d",
             insert_time->tm_year + 1900, insert_time->tm_mon + 1, insert_time->tm_mday, insert_time->tm_hour, insert_time->tm_min, insert_time->tm_sec);

    MYSQL *conn = get_conn(); // 데이터베이스 연결 획득

    for (int i = 0; i < NUM_CHIMNEY; i++)
    {
        CHIMNEY *c = &chimney[i];

        for (int j = 0; j < c->num_facility; j++)
        {
            FACILITY *f = &(c->facility[j]);
            FACILITY *rf = &(c->facility[c->facility[j].rel_facility - 1]);

            if (f->type != 0) // 해당없음(3)
            {
                f->SEC.rel = 3;
            }
            else
            {
                if (f->SEC.run == 0 || f->SEC.stat != 0 || rf->SEC.run == 1 || rf->SEC.stat != 0)
                {
                    f->SEC.rel = 1; // 정상(1)

                    if (rf->SEC.run == 1)
                        f->delay.bsec_run = 1;
                    else
                        f->delay.bsec_run = 0;
                }
                else
                {
                    f->SEC.rel = 0; // 비정상(0)

                    if (f->delay.bsec_run == 1)
                    {
                        if (f->delay.ofd > 0)
                        {
                            f->SEC.rel = 8; // 중지유예(8)
                        }
                    }
                    else
                    {
                        if (f->delay.ond > 0)
                        {
                            f->SEC.rel = 9; // 가동유예(9)
                        }
                    }
                }

                if (f->delay.lock == 9 && f->delay.ond > 0)
                    f->SEC.rel = 9;
                else if (f->delay.lock == 8 && f->delay.ofd > 0)
                    f->SEC.rel = 8;
            }

            snprintf(query_str, sizeof(query_str), "INSERT IGNORE INTO t_05sec (insert_time, fac_id, fac, _data, stat, run, rel, chim_id) VALUES(\'%s\', %d, \'%s\', %d, %d, %d, %d, %d);",
                     insertTime_str, j, f->name, f->SEC.value, f->SEC.stat, f->SEC.run, f->SEC.rel, i + 1);
            execute_query(conn, query_str);

            int32_t data = f->SEC.value;
            int16_t upper, lower;

            if (data < 0)
            {
                data = abs(data);
                upper = data / 100;
                lower = data % 100;
                if (upper < 10)
                    printf("%s (5초) %5s(%1s):  -%1d.%02u | 자료상태 %1d | 가동상태 %1d | 방지정상 %1d | 가동유예 %2d | 중지유예 %2d\n",
                           insertTime_str, f->name, f->item, upper, lower, f->SEC.stat, f->SEC.run, f->SEC.rel, f->delay.ond, f->delay.ofd);
                else
                    printf("%s (5초) %5s(%1s): -%2d.%02u | 자료상태 %1d | 가동상태 %1d | 방지정상 %1d | 가동유예 %2d | 중지유예 %2d\n",
                           insertTime_str, f->name, f->item, upper, lower, f->SEC.stat, f->SEC.run, f->SEC.rel, f->delay.ond, f->delay.ofd);
            }
            else
                printf("%s (5초) %5s(%1s): %3d.%02u | 자료상태 %1d | 가동상태 %1d | 방지정상 %1d | 가동유예 %2d | 중지유예 %2d\n",
                       insertTime_str, f->name, f->item, f->SEC.value / 100, f->SEC.value % 100, f->SEC.stat, f->SEC.run, f->SEC.rel, f->delay.ond, f->delay.ofd);
        }
        printf("\n");
    }

    release_conn(conn); // 데이터베이스 연결 해제
}

void process_5min(time_t *datetime)
{
    update_5min_data();

    update_5min_state(datetime, 0);

    update_5min_relation(datetime, 0);
}

void update_5min_data()
{
    for (int i = 0; i < MAX_NUM; i++)
    {
        fiv_value[i] = sum_value[i] / sum_cnt;
        sum_value[i] = 0;
    }

    sum_cnt = 0; // 합계 카운터 초기화
}

void update_5min_state(time_t *datetime, int power_off)
{
    char query_str[BUFFER_SIZE], beginTime_str[300], endTime_str[300], insertTime_str[300];

    time_t begin_t = *datetime;
    begin_t -= FIVSEC;
    struct tm begin_time = *localtime(&begin_t);
    struct tm *end_time = localtime(datetime);

    snprintf(beginTime_str, sizeof(beginTime_str), "%4d-%02d-%02d %02d:%02d",
             begin_time.tm_year + 1900, begin_time.tm_mon + 1, begin_time.tm_mday, begin_time.tm_hour, begin_time.tm_min);
    snprintf(endTime_str, sizeof(endTime_str), "%4d-%02d-%02d %02d:%02d",
             end_time->tm_year + 1900, end_time->tm_mon + 1, end_time->tm_mday, end_time->tm_hour, end_time->tm_min);
    snprintf(insertTime_str, sizeof(insertTime_str), "%4d-%02d-%02d %02d:%02d:%02d",
             end_time->tm_year + 1900, end_time->tm_mon + 1, end_time->tm_mday, end_time->tm_hour, end_time->tm_min, end_time->tm_sec);

    MYSQL *conn = get_conn(); // 데이터베이스 연결 획득

    for (int i = 0; i < NUM_CHIMNEY; i++)
    {
        CHIMNEY *c = &chimney[i];

        for (int j = 0; j < c->num_facility; j++)
        {
            snprintf(query_str, sizeof(query_str),
                     "SELECT SUM(CASE WHEN stat = 0 THEN _data END), SUM(_data), "
                     "COUNT(CASE WHEN stat = 0 THEN 1 END), COUNT(CASE WHEN stat = 1 THEN 1 END), "
                     "COUNT(CASE WHEN stat = 2 THEN 1 END), COUNT(CASE WHEN stat = 4 THEN 1 END), "
                     "COUNT(CASE WHEN stat = 8 THEN 1 END), "
                     "COUNT(CASE WHEN rel = 9 THEN 1 END), COUNT(CASE WHEN rel = 8 THEN 1 END), "
                     "COUNT(CASE WHEN rel = 0 THEN 1 END), COUNT(CASE WHEN rel = 1 THEN 1 END), "
                     "COUNT(CASE WHEN rel = 3 THEN 1 END) "
                     "FROM t_05sec WHERE insert_time > \'%s\' AND insert_time <= \'%s\' "
                     "AND fac_id = %d AND chim_id = %d FOR UPDATE;",
                     beginTime_str, endTime_str, j, i + 1);

            execute_query(conn, query_str);

            MYSQL_RES *res = mysql_store_result(conn);
            if (res == NULL)
            {
                release_conn(conn);
                return;
            }

            FACILITY *f = &(c->facility[j]);

            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)))
            {
                // 정상데이터 합계
                if (row[0] == NULL)
                    f->FIV.value = 0;
                else
                    f->FIV.value = atoi(row[0]);

                // 전체데이터 합계
                int32_t total_value;
                if (row[1] == NULL)
                    total_value = 0;
                else
                    total_value = atoi(row[1]);

                // 상태정보
                int nrm = atoi(row[2]); // 정상(0)
                int low = atoi(row[3]); // 비정상(1)
                int com = atoi(row[4]); // 통신불량(2)
                int off = atoi(row[5]); // 전원단절(4)
                int chk = atoi(row[6]); // 점검중(8)

                // 관계정보
                int ond = atoi(row[7]);  // 가동유예(9)
                int ofd = atoi(row[8]);  // 중지유예(8)
                int err = atoi(row[9]);  // 비정상(0)
                int okk = atoi(row[10]); // 정상(1)
                int naa = atoi(row[11]); // 해당없음(3)

                // 수집되지 않은 자료는 전원단절로 처리
                int total = nrm + low + com + off + chk;
                if (total < FIVNUM)
                    off += (FIVNUM - total);

                total = 0;

                // 전원단절 시 관계정보는 정상으로 처리
                total = ond + ofd + err + okk + naa;
                if (total < FIVNUM)
                {
                    if (f->type == 0)
                        okk += (FIVNUM - total);
                    else
                        naa += (FIVNUM - total);
                }

                // 정상자료가 50% 이상인 경우 정상자료만 평균, 그외는 전체평균
                if (nrm >= (FIVNUM / 2))
                {
                    f->FIV.value /= nrm;
                }
                else
                {
                    total_value /= FIVNUM;
                    f->FIV.value = total_value;
                }

                if (nrm > (FIVNUM / 2)) // 전체 갯수에서 정상항목 절반 이상
                    f->FIV.stat = 0;
                else
                {
                    if (chk > 0) // 점검중
                        f->FIV.stat = 8;
                    else if (off > 0) // 전원단절
                        f->FIV.stat = 4;
                    else if (com > 0) // 통신불량
                        f->FIV.stat = 2;
                    else if (low > 0) // 비정상범위
                        f->FIV.stat = 1;
                }

                if (f->FIV.stat == 0 && f->FIV.value >= f->range.std)
                { // 자료상태 정상이며, 측정값 범위가 기준값 이상일 경우
                    if (f->FIV.value >= f->range.max)
                        f->FIV.value = f->range.max; // 측정값이 최대값 이상일 경우, 최대값으로 설정

                    f->FIV.run = 1;
                }
                else
                    f->FIV.run = 0;

                if (!power_off)
                    printf("[%s](5분) %s(%s) (상태정보)정상 %2d | 비정상 %2d | 통신불량 %2d | 전원단절 %2d | 점검중 %2d\n",
                           insertTime_str, f->name, f->item, nrm, low, com, off, chk);
            }
            mysql_free_result(res);
        }
    }
    release_conn(conn); // 데이터베이스 연결 해제
}

void update_5min_relation(time_t *datetime, int power_off)
{
    char query_str[BUFFER_SIZE], insertTime_str[300], deleteTime_str[300];

    time_t delete_t = *datetime;
    delete_t -= FIVSEC;
    struct tm delete_time = *localtime(&delete_t);
    struct tm *insert_time = localtime(datetime);

    snprintf(deleteTime_str, sizeof(deleteTime_str), "%4d-%02d-%02d %02d:%02d",
             delete_time.tm_year + 1900, delete_time.tm_mon + 1, delete_time.tm_mday, delete_time.tm_hour, delete_time.tm_min);
    snprintf(insertTime_str, sizeof(insertTime_str), "%4d-%02d-%02d %02d:%02d:%02d",
             insert_time->tm_year + 1900, insert_time->tm_mon + 1, insert_time->tm_mday, insert_time->tm_hour, insert_time->tm_min, insert_time->tm_sec);

    MYSQL *conn = get_conn(); // 데이터베이스 연결 획득

    for (int i = 0; i < NUM_CHIMNEY; i++)
    {
        CHIMNEY *c = &chimney[i];

        for (int j = 0; j < c->num_facility; j++)
        {
            FACILITY *f = &(c->facility[j]);
            FACILITY *rf = &(c->facility[c->facility[j].rel_facility - 1]);

            if (f->type != 0)
            {
                f->FIV.rel = 3; // 배출시설이 아닌 경우, 관계정보는 해당없음(3)
            }
            else
            {
                if (f->FIV.run == 0 || f->FIV.stat != 0 || rf->FIV.run == 1 || rf->FIV.stat != 0)
                {
                    f->FIV.rel = 1; // 관계정보 정상(1)

                    if (f->delay.lock == 9 && f->delay.ond > 0)
                    {
                        f->FIV.rel = 9; // 정상이나, 가동유예 중이었으면 가동유예
                        f->delay.ond--;
                    }
                    else if (f->delay.lock == 8 && f->delay.ofd > 0)
                    {
                        f->FIV.rel = 8; // 정상이나, 중지유예 중이었으면 중지유예
                        f->delay.ofd--;
                    }
                    else
                    {
                        f->FIV.rel = 1; // 유예시간 초기화 및 정상

                        f->delay.ond = (c->delay.ond / 5);
                        f->delay.ofd = (c->delay.ofd / 5);

                        f->delay.lock = 1;
                    }
                }
                else
                {
                    f->FIV.rel = 0; // 관계정보 비정상(0)

                    if (f->delay.bfiv_run == 1) // 중지유예 시작
                    {
                        if (f->delay.lock == 1) // 중지유예 설정
                        {
                            if (f->delay.ofd > 0)
                            {
                                f->FIV.rel = 8;
                                f->delay.ofd--;

                                f->delay.lock = 8;
                            }
                        }
                        else if (f->delay.lock == 9) // 가동유예와 겹치면, 가동유예 설정
                        {
                            if (f->delay.ond > 0)
                            {
                                f->FIV.rel = 9;
                                f->delay.ond--;

                                if (f->delay.ofd > 0)
                                    f->delay.ofd--;

                                if (f->delay.ofd > 0 && f->delay.ond == 0)
                                    f->delay.lock = 8;
                            }
                            else if (f->delay.ofd > 0)
                            {
                                f->FIV.rel = 8; // 가동유예 시간이 지나면, 중지유예로 설정해야 함
                                f->delay.ofd--;

                                f->delay.lock = 8;
                            }
                            else // 25-01-13 추가
                            {
                                f->delay.lock = 0;
                                f->FIV.rel = 0;
                            }
                        }
                        else
                        {
                            f->delay.lock = 0;
                            f->FIV.rel = 0;
                        }
                    }
                    else
                    {
                        if (f->delay.lock == 1 || f->delay.lock == 9) // 가동유예 시작
                        {
                            if (f->delay.ond > 0)
                            {
                                f->FIV.rel = 9;
                                f->delay.ond--;

                                f->delay.lock = 9;
                            }
                        }
                        else if (f->delay.lock == 8)
                        {
                            if (f->delay.ofd > 0) // 중지유예가 진행중이면, 중지유예 설정
                            {
                                f->FIV.rel = 8;
                                f->delay.ofd--;
                            }
                            else // 25-01-13 추가
                            {
                                f->delay.lock = 0;
                                f->FIV.rel = 0;
                            }
                        }
                        else
                        {
                            f->delay.lock = 0;
                            f->FIV.rel = 0;
                        }
                    }
                }
            }

            if (rf->FIV.run)
                f->delay.bfiv_run = 1;
            else
                f->delay.bfiv_run = 0;

            if (!power_off)
                printf("[%s](5분) %s(%s) (방지정상)%2d (9: 가동유예, 8: 중지유예, 3: 해당없음, 1: 정상, 0: 비정상)\n",
                       insertTime_str, f->name, f->item, f->FIV.rel);

            // 5분 상태 정보 데이터 베이스 저장
            snprintf(query_str, sizeof(query_str), "INSERT IGNORE INTO t_05stat (insert_time, fac_id, fac, _data, stat, run, rel, chim_id) VALUES (\"%s\", %d, \"%s\", %d, %d, %d, %d, %d);",
                     insertTime_str, j, f->name, f->FIV.value, f->FIV.stat, f->FIV.run, f->FIV.rel, i + 1);
            execute_query(conn, query_str);
        }
        printf("\n");

        snprintf(query_str, sizeof(query_str), "DELETE FROM t_05sec WHERE insert_time <= \'%s\' and chim_id = %d;", deleteTime_str, i + 1);
        execute_query(conn, query_str);
    }

    release_conn(conn); // 데이터베이스 연결 해제

    if (!power_off)
    {
        for (int i = 0; i < NUM_CHIMNEY; i++)
        {
            CHIMNEY *c = &chimney[i];

            for (int j = 0; j < c->num_facility; j++)
            {
                FACILITY *f = &(c->facility[j]);
                int32_t data = f->FIV.value;
                int16_t upper, lower;

                if (data < 0)
                {
                    data = abs(data);
                    upper = data / 100;
                    lower = data % 100;
                    if (upper < 10)
                        printf("%s (5분) %5s(%1s):  -%1d.%02u | 상태정보 %1d | 가동유무 %1d | 방지정상 %1d | 가동유예 %2d | 중지유예 %2d\n",
                               insertTime_str, f->name, f->item, upper, lower, f->FIV.stat, f->FIV.run, f->FIV.rel, f->delay.ond, f->delay.ofd);
                    else
                        printf("%s (5분) %5s(%1s): -%2d.%02u | 상태정보 %1d | 가동유무 %1d | 방지정상 %1d | 가동유예 %2d | 중지유예 %2d\n",
                               insertTime_str, f->name, f->item, upper, lower, f->FIV.stat, f->FIV.run, f->FIV.rel, f->delay.ond, f->delay.ofd);
                }
                else
                    printf("%s (5분) %5s(%1s): %3d.%02u | 상태정보 %1d | 가동유무 %1d | 방지정상 %1d | 가동유예 %2d | 중지유예 %2d\n",
                           insertTime_str, f->name, f->item, f->FIV.value / 100, f->FIV.value % 100, f->FIV.stat, f->FIV.run, f->FIV.rel, f->delay.ond, f->delay.ofd);
            }
            printf("\n");
        }
    }
}

void process_30min(time_t *datetime, int power_off)
{
    char query_str[BUFFER_SIZE], beginTime_str[300], endTime_str[300], insertTime_str[300];

    time_t begin_t = *datetime;
    begin_t -= HAFSEC;
    struct tm begin_time = *localtime(&begin_t);
    struct tm *end_time = localtime(datetime);

    snprintf(beginTime_str, sizeof(beginTime_str), "%4d-%02d-%02d %02d:%02d",
             begin_time.tm_year + 1900, begin_time.tm_mon + 1, begin_time.tm_mday, begin_time.tm_hour, begin_time.tm_min);
    snprintf(endTime_str, sizeof(endTime_str), "%4d-%02d-%02d %02d:%02d",
             end_time->tm_year + 1900, end_time->tm_mon + 1, end_time->tm_mday, end_time->tm_hour, end_time->tm_min);
    snprintf(insertTime_str, sizeof(insertTime_str), "%4d-%02d-%02d %02d:%02d:%02d",
             end_time->tm_year + 1900, end_time->tm_mon + 1, end_time->tm_mday, end_time->tm_hour, end_time->tm_min, end_time->tm_sec);

    MYSQL *conn = get_conn(); // 데이터베이스 연결 획득

    for (int i = 0; i < NUM_CHIMNEY; i++)
    {
        CHIMNEY *c = &chimney[i];

        for (int j = 0; j < c->num_facility; j++)
        { // 30분전 ~ 현재시간까지의 5분 데이터 수집자료 데이터베이스에서 읽어오기

            snprintf(query_str, sizeof(query_str),
                     "SELECT SUM(CASE WHEN stat = 0 THEN _data END), SUM(run), "
                     "COUNT(CASE WHEN stat = 0 THEN 1 END), COUNT(CASE WHEN stat = 1 THEN 1 END), "
                     "COUNT(CASE WHEN stat = 2 THEN 1 END), COUNT(CASE WHEN stat = 4 THEN 1 END), "
                     "COUNT(CASE WHEN stat = 8 THEN 1 END), "
                     "COUNT(CASE WHEN rel = 9 THEN 1 END), COUNT(CASE WHEN rel = 8 THEN 1 END), "
                     "COUNT(CASE WHEN rel = 0 THEN 1 END), COUNT(CASE WHEN rel = 1 THEN 1 END), "
                     "COUNT(CASE WHEN rel = 3 THEN 1 END), SUM(_data) "
                     "FROM t_05stat WHERE insert_time > \'%s\' AND insert_time <= \'%s\' "
                     "AND fac_id = %d AND chim_id = %d;",
                     beginTime_str, endTime_str, j, i + 1);

            execute_query(conn, query_str);
            MYSQL_RES *res = mysql_store_result(conn); // 쿼리 결과 담기
            if (res == NULL)
            {
                release_conn(conn); // 데이터베이스 연결 해제
                return;
            }

            FACILITY *f = &(c->facility[j]);
            // facility *rf = &(c->facility[c->facility[i].rel_facility - 1]); 방지시설정상여부 관계정보 해당없음으로 TNOH 배제 -> 25-02-25

            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)))
            {
                // 측정 데이터 합계(정상적인 값)
                if (row[0] == NULL)
                    f->HAF.value = 0;
                else
                    f->HAF.value = atoi(row[0]);

                if (row[1] == NULL)
                    f->HAF.run = 0;
                else
                    f->HAF.run = atoi(row[1]);

                // 상태정보
                int nrm = atoi(row[2]); // 정상(0)
                int low = atoi(row[3]); // 비정상(1)
                int com = atoi(row[4]); // 통신불량(2)
                int off = atoi(row[5]); // 전원단절(4)
                int chk = atoi(row[6]); // 점검중(8)

                // 관계정보
                int ond = atoi(row[7]);  // 가동유예(9)
                int ofd = atoi(row[8]);  // 중지유예(8)
                int err = atoi(row[9]);  // 비정상(0)
                int okk = atoi(row[10]); // 정상(1)
                int naa = atoi(row[11]); // 해당없음(3)

                // 전체 합계(비정상 값도 포함)
                int32_t total_value;

                if (row[12] == NULL)
                    total_value = 0;
                else
                    total_value = atoi(row[12]);

                // 수집되지 않은 자료는 전원단절로 처리
                int total = nrm + low + com + off + chk;
                if (total < HAFNUM)
                    off = (HAFNUM - total) + off;

                total = 0;

                // 전원단절 시 관계정보는 정상으로 처리
                total = ond + ofd + err + okk + naa;
                if (total <= HAFNUM)
                {
                    if (f->type == 0)
                        okk = (HAFNUM - total) + okk;
                    else
                        naa = (HAFNUM - total) + naa;
                }

                if (total == 0)
                    total = HAFNUM;

                // 유효한 값은 평균을 내야 하므로 계산(가동에 해당하는 값만 평균)
                if (nrm >= (HAFNUM / 2))
                {
                    f->HAF.value /= nrm;
                }
                else // 전체평균
                {
                    total_value /= total;
                    f->HAF.value = total_value;
                }

                if (nrm >= (HAFNUM / 2)) // 정상
                    f->HAF.stat = 0;
                else
                {
                    if (chk > 0) // 점검중
                        f->HAF.stat = 8;
                    else if (off > 0) // 전원단절
                        f->HAF.stat = 4;
                    else if (com > 0) // 통신불량
                        f->HAF.stat = 2;
                    else if (low > 0) // 비정상범위
                        f->HAF.stat = 1;
                }

                if (naa > 0)
                    f->HAF.rel = 3;
                else if (ond > 0)
                    f->HAF.rel = 9;
                else if (ofd > 0)
                    f->HAF.rel = 8;
                else if (err > 0)
                    f->HAF.rel = 0;
                else
                    f->HAF.rel = 1;

                if ((f->type == 0 && f->HAF.run < 6 && f->HAF.run >= 1) || (f->type == 0 && f->HAF.rel != 1))
                {
                    // 배출시설 가동횟수가 1 ~ 5회 이거나 방지시설 정상여부가 정상(1)이 아닌 경우, TNOH를 생성한다.
                    c->TNOH++;   // 굴뚝내 TNOH 생성 필요 시설 갯수 증가
                    f->TNOH = 1; // TNOH 생성 배출시설
                    // rf->TNOH = 1; // TNOH 생성 방지-송풍 시설 -> 25-02-25 관계시설 정상여부에 해당하는 시설에 한해서.
                }

                if (!power_off)
                    printf("[%s](30분) %s(%s) (상태정보)정상 %2d | 비정상 %2d | 통신불량 %2d | 전원단절 %2d | 점검중 %2d\n (방지정상)가동유예 %2d | 중지유예 %2d | 비정상 %2d | 정상 %2d | 해당없음 %2d\n", insertTime_str, f->name, f->item, nrm, low, com, off, chk, ond, ofd, err, okk, naa);

                // 30분 상태 정보 데이터 베이스 저장
                snprintf(query_str, sizeof(query_str), "INSERT IGNORE INTO t_30stat (insert_time, fac_id, fac, _data, stat, run, rel, chim_id) VALUES (\"%s\", %d, \"%s\", %d, %d, %d, %d, %d);", insertTime_str, j, f->name, f->HAF.value, f->HAF.stat, f->HAF.run, f->HAF.rel, i + 1);
                execute_query(conn, query_str);
            }

            mysql_free_result(res);
        }
        printf("\n");
    }

    release_conn(conn); // 데이터베이스 연결 해제
    if (!power_off)
    {
        for (int i = 0; i < NUM_CHIMNEY; i++)
        {
            CHIMNEY *c = &chimney[i];

            for (int j = 0; j < c->num_facility; j++)
            {

                FACILITY *f = &(c->facility[j]);
                int32_t data = f->HAF.value;
                int16_t upper, lower;

                if (data < 0)
                {
                    data = abs(data);
                    upper = data / 100;
                    lower = data % 100;
                    if (upper < 10)
                        printf("%s (30분) %5s(%1s):  -%1d.%02u | 상태정보 %1d | 가동유무 %1d | 방지정상 %1d\n",
                               insertTime_str, f->name, f->item, upper, lower, f->HAF.stat, f->HAF.run, f->HAF.rel);
                    else
                        printf("%s (30분) %5s(%1s): -%2d.%02u | 상태정보 %1d | 가동유무 %1d | 방지정상 %1d\n",
                               insertTime_str, f->name, f->item, upper, lower, f->HAF.stat, f->HAF.run, f->HAF.rel);
                }
                else
                    printf("%s (30분) %5s(%1s): %3d.%02u | 상태정보 %1d | 가동유무 %1d | 방지정상 %1d\n",
                           insertTime_str, f->name, f->item, f->HAF.value / 100, f->HAF.value % 100, f->HAF.stat, f->HAF.run, f->HAF.rel);
            }
            printf("\n");
        }
    }
}

void update_tdah(time_t *datetime, int seg, int off)
{
    char data_segment[4], dateTime_str[100], dataTime_str[50], insertTime_str[100], buffer[BUFFER_SIZE], newBuffer[BUFFER_SIZE], crcBuffer[5], query_str[BUFFER_SIZE];
    time_t data_t = *datetime;

    if (seg == 1) // 5min
    {
        data_t -= FIVSEC;
        strcpy(data_segment, "FIV");
    }
    else
    {
        data_t -= HAFSEC;
        strcpy(data_segment, "HAF");
    }

    struct tm data_time = *localtime(&data_t);
    struct tm *insert_time = localtime(datetime);

    snprintf(dateTime_str, sizeof(dateTime_str), "%02d%02d%02d%02d%02d",
             (data_time.tm_year + 1900) % 100, data_time.tm_mon + 1, data_time.tm_mday, data_time.tm_hour, data_time.tm_min);

    snprintf(insertTime_str, sizeof(insertTime_str), "%4d-%02d-%02d %02d:%02d:%02d",
             insert_time->tm_year + 1900, insert_time->tm_mon + 1, insert_time->tm_mday, insert_time->tm_hour, insert_time->tm_min, insert_time->tm_sec);

    snprintf(dataTime_str, sizeof(dataTime_str), "%4d-%02d-%02d %02d:%02d:%02d",
             data_time.tm_year + 1900, data_time.tm_mon + 1, data_time.tm_mday, data_time.tm_hour, data_time.tm_min, data_time.tm_sec);

    MYSQL *conn = get_conn(); // 데이터베이스 연결 획득

    for (int i = 0; i < NUM_CHIMNEY; i++)
    {
        CHIMNEY *c = &chimney[i];

        snprintf(buffer, sizeof(buffer), "TDAH%7s%3s", c->business, c->chimney);

        uint16_t total_len = 35 + (c->num_facility * 15);
        snprintf(newBuffer, sizeof(newBuffer), "%4d%3s%10s%2d", total_len, data_segment, dateTime_str, c->num_facility);
        strcat(buffer, newBuffer);

        int TOFH = 0;

        for (int j = 0; j < c->num_facility; j++)
        {
            FACILITY *f = &(c->facility[j]);

            uint16_t upper = 0;
            uint8_t lower = 0;

            if (seg == 1)
            {
                if (f->FIV.value < 0)
                {
                    int32_t data = abs(f->FIV.value);
                    upper = data / 100;
                    lower = data % 100;

                    if (upper >= 100)
                        upper = 99;

                    if (upper >= 10)
                        snprintf(newBuffer, sizeof(newBuffer), "%5s%1s-%2u.%02u%1d%1d%1d", f->name, f->item, upper, lower, f->FIV.stat, f->FIV.run, f->FIV.rel);
                    else
                        snprintf(newBuffer, sizeof(newBuffer), "%5s%1s -%1u.%02u%1d%1d%1d", f->name, f->item, upper, lower, f->FIV.stat, f->FIV.run, f->FIV.rel);
                }
                else
                {
                    upper = f->FIV.value / 100;
                    lower = f->FIV.value % 100;

                    snprintf(newBuffer, sizeof(newBuffer), "%5s%1s%3u.%02u%1d%1d%1d", f->name, f->item, upper, lower, f->FIV.stat, f->FIV.run, f->FIV.rel);
                }

                if (f->FIV.stat == 4)
                {
                    TOFH++;
                }
            }
            else
            {
                if (f->HAF.value < 0)
                {
                    int32_t data = abs(f->HAF.value);
                    upper = data / 100;
                    lower = data % 100;

                    if (upper >= 100)
                        upper = 99;

                    if (upper >= 10)
                        snprintf(newBuffer, sizeof(newBuffer), "%5s%1s-%2u.%02u%1d%1d%1d", f->name, f->item, upper, lower, f->HAF.stat, f->HAF.run, f->HAF.rel);
                    else
                        snprintf(newBuffer, sizeof(newBuffer), "%5s%1s -%1u.%02u%1d%1d%1d", f->name, f->item, upper, lower, f->HAF.stat, f->HAF.run, f->HAF.rel);
                }
                else
                {
                    upper = f->HAF.value / 100;
                    lower = f->HAF.value % 100;

                    snprintf(newBuffer, sizeof(newBuffer), "%5s%1s%3u.%02u%1d%1d%1d", f->name, f->item, upper, lower, f->HAF.stat, f->HAF.run, f->HAF.rel);
                }

                if (f->HAF.stat == 4)
                {
                    TOFH++;
                }
            }

            strcat(buffer, newBuffer);
        }

        uint16_t crc = crc16_ccitt_false((uint8_t *)buffer, total_len - 2);
        snprintf(crcBuffer, sizeof(crcBuffer), "%04X", crc);

        // 확인용
        if (seg == 1)
            printf("%s(5분TDAH) %s%s\n", insertTime_str, buffer, crcBuffer);
        else
            printf("%s(30분TDAH) %s%s\n", insertTime_str, buffer, crcBuffer);

        // TDAH 테이블에 저장
        int send = 0;
        if (seg == 1)
        {
            if (c->send_mode == 0 || (TOFH > 0 && off))
                send = 1;
            else
                send = 0;

            if (TOFH > 0)
                snprintf(query_str, sizeof(query_str), "INSERT IGNORE INTO t_05tdah (insert_time, tim_date, off, cmd, _data, crc, chim_id, send) VALUES (\"%s\", \"%s\", 1, \"%s\", \"%s\", \"%s\", %d, %d);", insertTime_str, dataTime_str, "TDAH", buffer + 4, crcBuffer, i + 1, send);
            else
                snprintf(query_str, sizeof(query_str), "INSERT IGNORE INTO t_05tdah (insert_time, tim_date, off, cmd, _data, crc, chim_id, send) VALUES (\"%s\", \"%s\", 0, \"%s\", \"%s\", \"%s\", %d, %d);", insertTime_str, dataTime_str, "TDAH", buffer + 4, crcBuffer, i + 1, send);
        }
        else
        {
            if (c->send_mode == 2 || (TOFH > 0 && off))
                send = 1;
            else
                send = 0;

            if (TOFH > 0)
                snprintf(query_str, sizeof(query_str), "INSERT IGNORE INTO t_30tdah (insert_time, tim_date, off, cmd, _data, crc, chim_id, send) VALUES (\"%s\", \"%s\", 1, \"%s\", \"%s\", \"%s\", %d, %d);", insertTime_str, dataTime_str, "TDAH", buffer + 4, crcBuffer, i + 1, send);
            else
                snprintf(query_str, sizeof(query_str), "INSERT IGNORE INTO t_30tdah (insert_time, tim_date, off, cmd, _data, crc, chim_id, send) VALUES (\"%s\", \"%s\", 0, \"%s\", \"%s\", \"%s\", %d, %d);", insertTime_str, dataTime_str, "TDAH", buffer + 4, crcBuffer, i + 1, send);
        }

        execute_query(conn, query_str);

        if (TOFH == c->num_facility && off == 0) // 해당 구간은 전원단절 데이터도 생성해줘야 함.
        {
            snprintf(buffer, sizeof(buffer), "TOFH%7s%3s", c->business, c->chimney);
            uint16_t total_len = 34 + (1 * 4);

            snprintf(dateTime_str, sizeof(dateTime_str), "%04d%02d%02d",
                     data_time.tm_year + 1900, data_time.tm_mon + 1, data_time.tm_mday);

            if (seg == 1)
                snprintf(newBuffer, sizeof(newBuffer), "%4dFIV%8s%3d", total_len, dateTime_str, 1);
            else
                snprintf(newBuffer, sizeof(newBuffer), "%4dHAF%8s%3d", total_len, dateTime_str, 1);
            strcat(buffer, newBuffer);

            snprintf(newBuffer, sizeof(newBuffer), "%02d%02d", data_time.tm_hour, data_time.tm_min);
            strcat(buffer, newBuffer);

            uint16_t crc = crc16_ccitt_false((uint8_t *)buffer, total_len - 2);
            snprintf(crcBuffer, sizeof(crcBuffer), "%04X", crc);

            if (seg == 1)
                printf("[%s](5분TOFH) %s%s\n", insertTime_str, buffer, crcBuffer);
            else
                printf("[%s](30분TOFH) %s%s\n", insertTime_str, buffer, crcBuffer);

            if (seg == 1)
                snprintf(query_str, sizeof(query_str), "INSERT IGNORE INTO t_05tofh (insert_time, cmd, _data, crc, chim_id, send) VALUE (\"%s\", \"%s\", \"%s\", \"%s\", %d, 1);", insertTime_str, "TOFH", buffer + 4, crcBuffer, i + 1);
            else
                snprintf(query_str, sizeof(query_str), "INSERT IGNORE INTO t_30tofh (insert_time, cmd, _data, crc, chim_id, send) VALUE (\"%s\", \"%s\", \"%s\", \"%s\", %d, 1);", insertTime_str, "TOFH", buffer + 4, crcBuffer, i + 1);

            execute_query(conn, query_str);
        }
    }
    release_conn(conn);
}

void update_tnoh(time_t *datetime, int off)
{
    char dataTime_str[100], insertTime_str[100], buffer[BUFFER_SIZE], newBuffer[BUFFER_SIZE], crcBuffer[5], query_str[BUFFER_SIZE], tnohBuffer[BUFFER_SIZE];

    time_t data_t = *datetime;
    data_t -= HAFSEC;
    struct tm data_time = *localtime(&data_t);    // 데이터 시간(00:30 -> 00:00분 자료)
    struct tm *insert_time = localtime(datetime); // 데이터 생성 시점

    snprintf(insertTime_str, sizeof(insertTime_str), "%4d-%02d-%02d %02d:%02d:%02d",
             insert_time->tm_year + 1900, insert_time->tm_mon + 1, insert_time->tm_mday, insert_time->tm_hour, insert_time->tm_min, insert_time->tm_sec);
    snprintf(dataTime_str, sizeof(dataTime_str), "%02d%02d%02d%02d%02d",
             (data_time.tm_year + 1900) % 100, data_time.tm_mon + 1, data_time.tm_mday, data_time.tm_hour, data_time.tm_min);

    MYSQL *conn = get_conn(); // 데이터베이스 연결 획득

    for (int i = 0; i < NUM_CHIMNEY; i++)
    {
        CHIMNEY *c = &chimney[i];

        // TNOH 시설 갯수 확인 25-02-25 수정코드
        uint8_t numTNOHfac = 0;

        // 전원단절 데이터로 생성되면, TDAH를 전송하지 않기에 TNOH도 전송하지 않는다
        int TOFH = 0;

        if (c->TNOH > 0)
        {
            memset(tnohBuffer, '\0', BUFFER_SIZE);

            for (int i = 0; i < c->num_facility; i++)
            {
                FACILITY *f = &(c->facility[i]);

                if (f->TNOH == 1) // 해당 시설이 TNOH 전송 대상에 해당되는 경우
                {
                    // 수정 25-02-25
                    snprintf(newBuffer, sizeof(newBuffer), "%5s%1s%1d%1d", f->name, f->item, f->HAF.run, f->HAF.rel);
                    strcat(tnohBuffer, newBuffer);

                    numTNOHfac++; // 항목 수 증감

                    if (f->HAF.stat == 4)
                        TOFH++;
                }

                f->TNOH = 0; // 시설 TNOH 플래그 해제
            }

            snprintf(buffer, sizeof(buffer), "TNOH%7s%3s", c->business, c->chimney);
            uint16_t total_len = 35 + (numTNOHfac * 8);

            snprintf(newBuffer, sizeof(newBuffer), "%4dHAF%10s%2d", total_len, dataTime_str, numTNOHfac);
            strcat(buffer, newBuffer);

            // 반복문에서 생성한 TNOH 데이터 붙이기
            strcat(buffer, tnohBuffer);

            numTNOHfac = 0;
            c->TNOH = 0; // 굴뚝 TNOH 플래그 해제

            uint16_t crc = crc16_ccitt_false((uint8_t *)buffer, total_len - 2);
            snprintf(crcBuffer, sizeof(crcBuffer), "%04X", crc);

            // 확인용
            printf("%s %s%s\n", insertTime_str, buffer, crcBuffer);

            // TNOH 테이블에 저장
            if (TOFH > 0 && off)
                snprintf(query_str, sizeof(query_str), "INSERT IGNORE INTO t_05tnoh (insert_time, cmd, _data, crc, chim_id, send) VALUE (\"%s\", \"%s\", \"%s\", \"%s\", %d, %d);",
                         insertTime_str, "TNOH", buffer + 4, crcBuffer, i + 1, 1);
            else
                snprintf(query_str, sizeof(query_str), "INSERT IGNORE INTO t_05tnoh (insert_time, cmd, _data, crc, chim_id, send) VALUE (\"%s\", \"%s\", \"%s\", \"%s\", %d, %d);",
                         insertTime_str, "TNOH", buffer + 4, crcBuffer, i + 1, 0);

            execute_query(conn, query_str);
        }
    }

    release_conn(conn); // 데이터베이스 연결 해제
}

void process_day(time_t *datetime)
{
    // 일일 데이터 생성
    char query_str[BUFFER_SIZE], insertTime_str[50], endTime_str[50], beginTime_str[50], beginHafTime_str[50], deleteTime_str[50];

    struct tm *insert_time = localtime(datetime); // 현재 시간(아마도 다음날 0시 0분 0초)
    snprintf(insertTime_str, sizeof(insertTime_str), "%4d-%02d-%02d %02d:%02d:%02d", insert_time->tm_year + 1900, insert_time->tm_mon + 1, insert_time->tm_mday, insert_time->tm_hour, insert_time->tm_min, insert_time->tm_sec);
    snprintf(endTime_str, sizeof(endTime_str), "%4d-%02d-%02d 00:00:00", insert_time->tm_year + 1900, insert_time->tm_mon + 1, insert_time->tm_mday);

    time_t begin_t = *datetime;
    begin_t -= DAYSEC; // 하루전 시간
    struct tm begin_time = *localtime(&begin_t);
    snprintf(beginTime_str, sizeof(beginTime_str), "%4d-%02d-%02d 00:05:00", begin_time.tm_year + 1900, begin_time.tm_mon + 1, begin_time.tm_mday);
    snprintf(beginHafTime_str, sizeof(beginHafTime_str), "%4d-%02d-%02d 00:30:00", begin_time.tm_year + 1900, begin_time.tm_mon + 1, begin_time.tm_mday);

    time_t delete_t = *datetime;
    delete_t -= (DAYSEC * 3); // 3일 전 시간
    struct tm delete_time = *localtime(&delete_t);
    snprintf(deleteTime_str, sizeof(deleteTime_str), "%4d-%02d-%02d", delete_time.tm_year + 1900, delete_time.tm_mon + 1, delete_time.tm_mday);

    printf("%s 일일마감(5분) 생성 시점: %s\n", insertTime_str, beginTime_str);

    MYSQL *conn = get_conn(); // 데이터베이스 연결 획득

    for (int i = 0; i < NUM_CHIMNEY; i++)
    {
        CHIMNEY *c = &chimney[i];

        for (int j = 0; j < c->num_facility; j++)
        {
            // 전일 5분 데이터 데이터베이스에서 읽어오기
            snprintf(query_str, sizeof(query_str),
                     "SELECT COUNT(CASE WHEN stat = 0 THEN 1 END),"
                     "COUNT(CASE WHEN stat = 1 THEN 1 END), "
                     "COUNT(CASE WHEN stat = 2 THEN 1 END), "
                     "COUNT(CASE WHEN stat = 4 THEN 1 END), "
                     "COUNT(CASE WHEN stat = 8 THEN 1 END) "
                     "FROM t_05stat "
                     "WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' "
                     "AND fac_id = %d AND chim_id = %d;",
                     beginTime_str, endTime_str, j, i + 1);

            printf("check fiv_day_process query: %s\n", query_str);

            execute_query(conn, query_str);
            MYSQL_RES *res = mysql_store_result(conn); // 쿼리 결과 담기
            if (res == NULL)
            {
                release_conn(conn); // 데이터베이스 연결 해제
                return;
            }

            FACILITY *f = &(c->facility[j]);

            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)))
            {
                // 상태정보
                f->SFIV.nrm = atoi(row[0]); // 정상(0)
                f->SFIV.low = atoi(row[1]); // 비정상(1)
                f->SFIV.com = atoi(row[2]); // 통신불량(2)
                f->SFIV.off = atoi(row[3]); // 전원단절(4)
                f->SFIV.chk = atoi(row[4]); // 점검중(8)

                // 수집되지 않은 자료는 전원단절로 처리
                f->SFIV.total = f->SFIV.nrm + f->SFIV.low + f->SFIV.com + f->SFIV.off + f->SFIV.chk;
                /*if (total <= FIVTDDH)
                    f->SFIV.off = (FIVTDDH - total) + f->SFIV.off;*/

                // j번 시설 3일 전 5분 상태 정보 클리어
                snprintf(query_str, sizeof(query_str), "DELETE FROM t_05stat WHERE fac_id = %d AND chim_id = %d AND insert_time < \'%s%%\';",
                         j, i + 1, deleteTime_str);
                execute_query(conn, query_str);
            }

            mysql_free_result(res);
        }
    }

    for (int i = 0; i < NUM_CHIMNEY; i++)
    { // 확인용
        CHIMNEY *c = &chimney[i];
        for (int j = 0; j < c->num_facility; j++)
        {
            FACILITY *f = &(c->facility[j]);
            printf("%s (일일 5분) %s(%1s) | 정상 %3d | 비정상 %3d | 통신불량 %3d | 전원단절 %3d | 점검중 %3d\n",
                   insertTime_str, f->name, f->item, f->SFIV.nrm, f->SFIV.low, f->SFIV.com, f->SFIV.off, f->SFIV.chk);
        }
    }

    printf("%s 일일마감(30분) 생성 시점: %s\n", insertTime_str, beginTime_str);

    for (int i = 0; i < NUM_CHIMNEY; i++)
    {
        CHIMNEY *c = &chimney[i];

        for (int j = 0; j < c->num_facility; j++)
        {
            // 전일 30분 데이터 데이터베이스에서 읽어오기
            snprintf(query_str, sizeof(query_str), "SELECT COUNT(CASE WHEN stat = 0 THEN 1 END), COUNT(CASE WHEN stat = 1 THEN 1 END), COUNT(CASE WHEN stat = 2 THEN 1 END), COUNT(CASE WHEN stat = 4 THEN 1 END), COUNT(CASE WHEN stat = 8 THEN 1 END) FROM t_30stat WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND fac_id = %d AND chim_id = %d;", beginHafTime_str, endTime_str, j, i + 1);

            printf("check haf_day_process query: %s\n", query_str);

            execute_query(conn, query_str);
            MYSQL_RES *res = mysql_store_result(conn); // 쿼리 결과 담기
            if (res == NULL)
            {
                release_conn(conn); // 데이터베이스 연결 해제
                return;
            }

            FACILITY *f = &(c->facility[j]);

            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)))
            {
                // 상태정보
                f->SHAF.nrm = atoi(row[0]); // 정상(0)
                f->SHAF.low = atoi(row[1]); // 비정상(1)
                f->SHAF.com = atoi(row[2]); // 통신불량(2)
                f->SHAF.off = atoi(row[3]); // 전원단절(4)
                f->SHAF.chk = atoi(row[4]); // 점검중(8)

                // 수집되지 않은 자료는 전원단절로 처리
                f->SHAF.total = f->SHAF.nrm + f->SHAF.low + f->SHAF.com + f->SHAF.off + f->SHAF.chk;
                /*if (total <= HAFTDDH)
                    f->SHAF.off = (HAFTDDH - total) + f->SHAF.off;*/

                // j번 시설 3일 전 30분 상태 정보 클리어
                snprintf(query_str, sizeof(query_str), "DELETE FROM t_30stat WHERE fac_id = %d AND chim_id = %d AND insert_time < \'%s%%\';",
                         j, i + 1, deleteTime_str);
                execute_query(conn, query_str);
            }
            mysql_free_result(res);
        }
    }

    release_conn(conn); // 데이터베이스 연결 해제

    for (int i = 0; i < NUM_CHIMNEY; i++)
    { // 확인용
        CHIMNEY *c = &chimney[i];
        for (int j = 0; j < c->num_facility; j++)
        {
            FACILITY *f = &(c->facility[j]);
            printf("%s (일일 30분) %s(%1s) 정상 %3d | 비정상 %3d | 통신불량 %3d | 전원단절 %3d | 점검중 %3d\n",
                   insertTime_str, f->name, f->item, f->SHAF.nrm, f->SHAF.low, f->SHAF.com, f->SHAF.off, f->SHAF.chk);
        }
    }
}

void update_tddh(time_t *datetime, int seg)
{
    char dataTime_str[100], insertTime_str[100], buffer[BUFFER_SIZE], newBuffer[BUFFER_SIZE], crcBuffer[5], query_str[BUFFER_SIZE], endTime_str[50], beginTime_str[50], endHafTime_str[50], beginHafTime_str[50], deleteTime_str[50];

    time_t data_t = *datetime;
    data_t -= DAYSEC;
    struct tm data_time = *localtime(&data_t);    // 데이터 시간(전일)
    struct tm *insert_time = localtime(datetime); // 데이터 생성 시점

    snprintf(insertTime_str, sizeof(insertTime_str), "%4d-%02d-%02d %02d:%02d:%02d",
             insert_time->tm_year + 1900, insert_time->tm_mon + 1, insert_time->tm_mday, insert_time->tm_hour, insert_time->tm_min, insert_time->tm_sec);
    snprintf(dataTime_str, sizeof(dataTime_str), "%4d%02d%02d", data_time.tm_year + 1900, data_time.tm_mon + 1, data_time.tm_mday);

    snprintf(beginTime_str, sizeof(beginTime_str), "%4d-%02d-%02d 00:05:00", data_time.tm_year + 1900, data_time.tm_mon + 1, data_time.tm_mday);
    snprintf(endTime_str, sizeof(endTime_str), "%4d-%02d-%02d 00:00:00", insert_time->tm_year + 1900, insert_time->tm_mon + 1, insert_time->tm_mday);
    snprintf(beginHafTime_str, sizeof(beginHafTime_str), "%4d-%02d-%02d 00:30:00", data_time.tm_year + 1900, data_time.tm_mon + 1, data_time.tm_mday);
    snprintf(endHafTime_str, sizeof(endHafTime_str), "%4d-%02d-%02d 00:00:00", insert_time->tm_year + 1900, insert_time->tm_mon + 1, insert_time->tm_mday);

    MYSQL *conn = get_conn(); // 데이터베이스 연결 획득

    for (int i = 0; i < NUM_CHIMNEY; i++)
    {
        CHIMNEY *c = &chimney[i];

        snprintf(buffer, sizeof(buffer), "TDDH%7s%3s", c->business, c->chimney);
        uint16_t total_len = 42 + (c->num_facility * 21);

        if (seg == 1)
            snprintf(newBuffer, sizeof(newBuffer), "%4dFIV%8s%3d%3d%3d%2d", total_len, dataTime_str, c->facility[0].SFIV.total, c->facility[0].SFIV.total - c->facility[0].SFIV.off, c->facility[0].SFIV.off, c->num_facility);
        else
            snprintf(newBuffer, sizeof(newBuffer), "%4dHAF%8s%3d%3d%3d%2d", total_len, dataTime_str, c->facility[0].SHAF.total, c->facility[0].SHAF.total - c->facility[0].SHAF.off, c->facility[0].SHAF.off, c->num_facility);
        strcat(buffer, newBuffer);

        for (int j = 0; j < c->num_facility; j++)
        {
            FACILITY *f = &(c->facility[j]);

            if (seg == 1)
                snprintf(newBuffer, sizeof(newBuffer), "%5s%1s%3d%3d%3d%3d%3d", f->name, f->item, f->SFIV.nrm, f->SFIV.low, f->SFIV.com, f->SFIV.off, f->SFIV.chk);
            else
                snprintf(newBuffer, sizeof(newBuffer), "%5s%1s%3d%3d%3d%3d%3d", f->name, f->item, f->SHAF.nrm, f->SHAF.low, f->SHAF.com, f->SHAF.off, f->SHAF.chk);
            strcat(buffer, newBuffer);
        }

        uint16_t crc = crc16_ccitt_false((uint8_t *)buffer, total_len - 2);
        snprintf(crcBuffer, sizeof(crcBuffer), "%04X", crc);

        // 확인용
        printf("%s %s%s\n", insertTime_str, buffer, crcBuffer);

        // TDDH 테이블에 저장
        if (seg == 1)
        {
            if (c->send_mode == 0)
                snprintf(query_str, sizeof(query_str), "INSERT IGNORE INTO t_05tddh (insert_time, cmd, _data, crc, chim_id, send) VALUE (\"%s\", \"%s\", \"%s\", \"%s\", %d, 1);",
                         insertTime_str, "TDDH", buffer + 4, crcBuffer, i + 1);
            else
                snprintf(query_str, sizeof(query_str), "INSERT IGNORE INTO t_05tddh (insert_time, cmd, _data, crc, chim_id, send) VALUE (\"%s\", \"%s\", \"%s\", \"%s\", %d, 0);",
                         insertTime_str, "TDDH", buffer + 4, crcBuffer, i + 1);
        }
        else
        {
            if (c->send_mode == 2)
                snprintf(query_str, sizeof(query_str), "INSERT IGNORE INTO t_30tddh (insert_time, cmd, _data, crc, chim_id, send) VALUE (\"%s\", \"%s\", \"%s\", \"%s\", %d, 1);",
                         insertTime_str, "TDDH", buffer + 4, crcBuffer, i + 1);
            else
                snprintf(query_str, sizeof(query_str), "INSERT IGNORE INTO t_30tddh (insert_time, cmd, _data, crc, chim_id, send) VALUE (\"%s\", \"%s\", \"%s\", \"%s\", %d, 0);",
                         insertTime_str, "TDDH", buffer + 4, crcBuffer, i + 1);
        }
        execute_query(conn, query_str);

        /*일일마감 전원단절 데이터 생성*/
        if (seg == 1)
            snprintf(query_str, sizeof(query_str), "SELECT tim_date FROM t_05tdah WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND off = 1 AND chim_id = %d;", beginTime_str, endTime_str, i + 1);
        else
            snprintf(query_str, sizeof(query_str), "SELECT tim_date FROM t_30tdah WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND off = 1 AND chim_id = %d;", beginHafTime_str, endHafTime_str, i + 1);

        execute_query(conn, query_str);
        MYSQL_RES *res = mysql_store_result(conn); // 쿼리 결과 담기

        int tofh_num = 0;
        if ((tofh_num = mysql_num_rows(res)) > 0)
        {
            char tofhTime[tofh_num][20];
            for (int j = 0; j < tofh_num; j++)
            {
                MYSQL_ROW row = mysql_fetch_row(res);

                if (row)
                {
                    strcpy(tofhTime[j], row[0]);

                    if (seg == 1)
                        printf("(5분)전원단절구간 시간: %s\n", tofhTime[j]);
                    else
                        printf("(30분)전원단절구간 시간: %s\n", tofhTime[j]);
                }
            }
            mysql_free_result(res);

            snprintf(buffer, sizeof(buffer), "TOFH%7s%3s", c->business, c->chimney);
            uint16_t total_len = 34 + (tofh_num * 4);

            if (seg == 1)
                snprintf(newBuffer, sizeof(newBuffer), "%4dFIV%8s%3d", total_len, dataTime_str, tofh_num);
            else
                snprintf(newBuffer, sizeof(newBuffer), "%4dHAF%8s%3d", total_len, dataTime_str, tofh_num);
            strcat(buffer, newBuffer);

            for (int j = 0; j < tofh_num; j++)
            {
                snprintf(newBuffer, sizeof(newBuffer), "%c%c%c%c", tofhTime[j][11], tofhTime[j][12], tofhTime[j][14], tofhTime[j][15]);
                strcat(buffer, newBuffer);
            }

            uint16_t crc = crc16_ccitt_false((uint8_t *)buffer, total_len - 2);
            snprintf(crcBuffer, sizeof(crcBuffer), "%04X", crc);

            // 확인용
            if (seg == 1)
                printf("일일마감 FIV TOFH[%s] %s%s\n", insertTime_str, buffer, crcBuffer);
            else
                printf("일일마감 HAF TOFH[%s] %s%s\n", insertTime_str, buffer, crcBuffer);

            // TOFH 테이블에 저장
            if (seg == 1)
            {
                if (c->send_mode == 0) // 30분 전송 모드인 경우, 5분 전원단절 데이터 전송하지 않음.
                    snprintf(query_str, sizeof(query_str), "INSERT IGNORE INTO t_05tofh_day (insert_time, cmd, _data, crc, chim_id, send) VALUE (\"%s\", \"%s\", \"%s\", \"%s\", %d, 1);", insertTime_str, "TOFH", buffer + 4, crcBuffer, i + 1);
                else
                    snprintf(query_str, sizeof(query_str), "INSERT IGNORE INTO t_05tofh_day (insert_time, cmd, _data, crc, chim_id, send) VALUE (\"%s\", \"%s\", \"%s\", \"%s\", %d, 0);", insertTime_str, "TOFH", buffer + 4, crcBuffer, i + 1);
            }
            else
            {
                if (c->send_mode == 2) // 5분 전송 모드인 경우, 30분 전원단절 데이터 전송하지 않음.
                    snprintf(query_str, sizeof(query_str), "INSERT IGNORE INTO t_30tofh_day (insert_time, cmd, _data, crc, chim_id, send) VALUE (\"%s\", \"%s\", \"%s\", \"%s\", %d, 1);", insertTime_str, "TOFH", buffer + 4, crcBuffer, i + 1);
                else
                    snprintf(query_str, sizeof(query_str), "INSERT IGNORE INTO t_30tofh_day (insert_time, cmd, _data, crc, chim_id, send) VALUE (\"%s\", \"%s\", \"%s\", \"%s\", %d, 0);", insertTime_str, "TOFH", buffer + 4, crcBuffer, i + 1);
            }

            execute_query(conn, query_str);
        }
    }

    // 30일 전 데이터 삭제
    time_t delete_t = *datetime;
    delete_t -= (DAYSEC * 30);
    struct tm delete_time = *localtime(&delete_t);

    snprintf(deleteTime_str, sizeof(deleteTime_str), "%4d-%02d-%02d %02d:%02d",
             delete_time.tm_year + 1900, delete_time.tm_mon + 1, delete_time.tm_mday, delete_time.tm_hour, delete_time.tm_min);

    if (seg == 1)
    {
        snprintf(query_str, sizeof(query_str), "DELETE FROM t_05tdah WHERE insert_time <= \'%s%%\';", deleteTime_str);
        execute_query(conn, query_str);
        snprintf(query_str, sizeof(query_str), "DELETE FROM t_05tddh WHERE insert_time <= \'%s%%\';", deleteTime_str);
        execute_query(conn, query_str);
        snprintf(query_str, sizeof(query_str), "DELETE FROM t_05tofh WHERE insert_time <= \'%s%%\';", deleteTime_str);
        execute_query(conn, query_str);
        snprintf(query_str, sizeof(query_str), "DELETE FROM t_05tofh_day WHERE insert_time <= \'%s%%\';", deleteTime_str);
        execute_query(conn, query_str);
    }
    else
    {
        snprintf(query_str, sizeof(query_str), "DELETE FROM t_30tdah WHERE insert_time <= \'%s%%\';", deleteTime_str);
        execute_query(conn, query_str);
        snprintf(query_str, sizeof(query_str), "DELETE FROM t_05tnoh WHERE insert_time <= \'%s%%\';", deleteTime_str);
        execute_query(conn, query_str);
        snprintf(query_str, sizeof(query_str), "DELETE FROM t_30tddh WHERE insert_time <= \'%s%%\';", deleteTime_str);
        execute_query(conn, query_str);
        snprintf(query_str, sizeof(query_str), "DELETE FROM t_30tofh WHERE insert_time <= \'%s%%\';", deleteTime_str);
        execute_query(conn, query_str);
        snprintf(query_str, sizeof(query_str), "DELETE FROM t_30tofh_day WHERE insert_time <= \'%s%%\';", deleteTime_str);
        execute_query(conn, query_str);
    }

    release_conn(conn); // 데이터베이스 연결 해제
}

int check_off(TOFH_TIME *t)
{
    char query_str[BUFFER_SIZE], time_buff[100];
    int year, mon, day, hour, min, sec, result;

    // 데이터베이스내 5분 데이터 마지막으로 저장 시간 확인
    snprintf(query_str, sizeof(query_str), "SELECT insert_time FROM t_05tdah ORDER BY insert_time DESC LIMIT 1;");

    MYSQL *conn = get_conn(); // 데이터베이스 연결 획득
    execute_query(conn, query_str);

    MYSQL_RES *res = mysql_store_result(conn); // 쿼리 결과 담기

    if (res == NULL || !mysql_num_rows(res))
    {
        release_conn(conn);
        return 0;
    }

    // 데이터베이스 저장 내용 불러오기
    MYSQL_ROW row = mysql_fetch_row(res);
    strcpy(time_buff, row[0]);

    // 마지막 시간 데이터 저장(시작 범위)
    if (sscanf(time_buff, "%4d-%02d-%02d %02d:%02d:%02d",
               &year, &mon, &day, &hour, &min, &sec) == 6)
    {
        t->off_fiv.tm_year = year - 1900;
        t->off_fiv.tm_mon = mon - 1;
        t->off_fiv.tm_mday = day;
        t->off_fiv.tm_hour = hour;
        t->off_fiv.tm_min = min;
        t->off_fiv.tm_sec = sec;
    }

    // 마지막 시간 정보 확인
    printf("(5분)데이터베이스 마지막 저장 시간: %4d-%02d-%02d %02d:%02d:%02d\n",
           t->off_fiv.tm_year + 1900, t->off_fiv.tm_mon + 1, t->off_fiv.tm_mday,
           t->off_fiv.tm_hour, t->off_fiv.tm_min, t->off_fiv.tm_sec);

    // 쿼리 결과 메모리 할당 해제
    mysql_free_result(res);

    // 마지막으로 저장된 5분 데이터 확인
    result = 1;

    // 데이터베이스내 30분 데이터 마지막으로 저장 시간 확인
    snprintf(query_str, sizeof(query_str), "SELECT insert_time FROM t_30tdah ORDER BY insert_time DESC LIMIT 1;");
    execute_query(conn, query_str);

    res = mysql_store_result(conn); // 쿼리 결과 담기

    if (res == NULL || !mysql_num_rows(res))
    {
        release_conn(conn);

        struct tm corr_time = t->off_fiv;
        time_t corr_t = mktime(&corr_time);
        corr_t -= (corr_t % HAFSEC);
        corr_time = *localtime(&corr_t);
        t->off_haf = corr_time;

        return result;
    }

    // 데이터베이스 저장 내용 불러오기
    row = mysql_fetch_row(res);
    strcpy(time_buff, row[0]);

    // 마지막 시간 데이터 저장(시작 범위)
    if (sscanf(time_buff, "%4d-%02d-%02d %02d:%02d:%02d",
               &year, &mon, &day, &hour, &min, &sec) == 6)
    {

        t->off_haf.tm_year = year - 1900;
        t->off_haf.tm_mon = mon - 1;
        t->off_haf.tm_mday = day;
        t->off_haf.tm_hour = hour;
        t->off_haf.tm_min = min;
        t->off_haf.tm_sec = sec;
    }

    // 마지막 시간 정보 확인
    printf("(30분)데이터베이스 마지막 저장 시간: %4d-%02d-%02d %02d:%02d:%02d\n",
           t->off_haf.tm_year + 1900, t->off_haf.tm_mon + 1, t->off_haf.tm_mday,
           t->off_haf.tm_hour, t->off_haf.tm_min, t->off_haf.tm_sec);

    // 쿼리 결과 메모리 할당 해제
    mysql_free_result(res);

    // 쿼리 접속 해제
    release_conn(conn);

    return result;
}

void *tofh_thread(void *arg)
{
    // 전원단절 구간 생성 스레드
    TOFH_TIME t = *(TOFH_TIME *)arg;

    // 현재 시간 업데이트
    time_t current_t = time(NULL);
    struct tm current_time = *localtime(&current_t);
    t.off_now = current_time;

    printf("check Time(FIV): %4d-%02d-%02d %02d:%02d:%02d\n",
           t.off_fiv.tm_year + 1900, t.off_fiv.tm_mon + 1, t.off_fiv.tm_mday,
           t.off_fiv.tm_hour, t.off_fiv.tm_min, t.off_fiv.tm_sec);

    printf("check Time(HAF): %4d-%02d-%02d %02d:%02d:%02d\n",
           t.off_haf.tm_year + 1900, t.off_haf.tm_mon + 1, t.off_haf.tm_mday,
           t.off_haf.tm_hour, t.off_haf.tm_min, t.off_haf.tm_sec);

    printf("check Time(NOW): %4d-%02d-%02d %02d:%02d:%02d\n",
           t.off_now.tm_year + 1900, t.off_now.tm_mon + 1, t.off_now.tm_mday,
           t.off_now.tm_hour, t.off_now.tm_min, t.off_now.tm_sec);

    // 전원단절 구간 데이터 생성
    process_off(t);

    // 스레드 종료
    pthread_exit(NULL);
}

void process_off(TOFH_TIME t)
{
    int day_over = 0;

    time_t begin_t = mktime(&t.off_fiv);
    time_t end_t = mktime(&t.off_now);

    printf("전원단절구간 데이터 생성 시작: %4d-%02d-%02d %02d:%02d:%02d\n",
           t.off_fiv.tm_year + 1900, t.off_fiv.tm_mon + 1, t.off_fiv.tm_mday,
           t.off_fiv.tm_hour, t.off_fiv.tm_min, t.off_fiv.tm_sec);
    printf("전원단절구간 데이터 생성 종료: %4d-%02d-%02d %02d:%02d:%02d\n",
           t.off_now.tm_year + 1900, t.off_now.tm_mon + 1, t.off_now.tm_mday,
           t.off_now.tm_hour, t.off_now.tm_min, t.off_now.tm_sec);

    if (t.off_fiv.tm_mday != t.off_now.tm_mday)
        day_over = 1;
    else
        day_over = 0;

    while (begin_t <= end_t)
    {
        // 시작시간과 종료시간(시작일 다음날의 00:00:00) 설정
        struct tm begin_fiv_time = t.off_fiv;
        struct tm begin_haf_time = t.off_haf;
        time_t begin_fiv = mktime(&begin_fiv_time);
        time_t begin_haf = mktime(&begin_haf_time);

        struct tm end_time = t.off_fiv;
        end_time.tm_hour = 0;
        end_time.tm_min = 0;
        end_time.tm_sec = 0;

        // 시간 비교를 위한 타임 변수 변환
        time_t end = mktime(&end_time);
        end += DAYSEC;

        // 시작시간 종료시간 확인
        printf("전원단절 생성(FIV) 시작: %4d-%02d-%02d %02d:%02d:%02d\n",
               begin_fiv_time.tm_year + 1900, begin_fiv_time.tm_mon + 1, begin_fiv_time.tm_mday,
               begin_fiv_time.tm_hour, begin_fiv_time.tm_min, begin_fiv_time.tm_sec);
        printf("전원단절 생성(HAF) 시작: %4d-%02d-%02d %02d:%02d:%02d\n",
               begin_haf_time.tm_year + 1900, begin_haf_time.tm_mon + 1, begin_haf_time.tm_mday,
               begin_haf_time.tm_hour, begin_haf_time.tm_min, begin_haf_time.tm_sec);

        end_time = *localtime(&end);
        printf("전원단절 생성(ALL) 종료: %4d-%02d-%02d %02d:%02d:%02d\n",
               end_time.tm_year + 1900, end_time.tm_mon + 1, end_time.tm_mday,
               end_time.tm_hour, end_time.tm_min, end_time.tm_sec);

        // 종료시간이 전원단절생성 스레드에서 받은 현재시간보다 큰 경우,
        // 종료시간을 생성했던 현재시간으로 수정
        if (end_t < end)
        {
            end_time = t.off_now;
            end = end_t;

            printf("전원단절 생성(ALL) 종료 수정: %4d-%02d-%02d %02d:%02d:%02d\n",
                   end_time.tm_year + 1900, end_time.tm_mon + 1, end_time.tm_mday,
                   end_time.tm_hour, end_time.tm_min, end_time.tm_sec);
        }

        // 첫째날의 경우, 읽어온 데이터 시간이 기존 존재하는 데이터를 포함하고 있으므로,
        // 다음 시간부터 생성할 수 있도록 시간을 보정해준다.
        begin_fiv += FIVSEC;
        begin_haf += HAFSEC;

        int off_data_fiv_num = 0; // 전원단절 구간 5분 데이터 갯수
        int off_data_haf_num = 0; // 전원단절 구간 30분 데이터 갯수
        if (begin_fiv <= end)
            off_data_fiv_num = ((end - begin_fiv) / FIVSEC) + 1; // '+1'은 시작 시점 데이터 갯수 포함
        if (begin_haf <= end)
            off_data_haf_num = ((end - begin_haf) / HAFSEC) + 1; // '+1'은 시작 시점 데이터 갯수 포함

        if (off_data_fiv_num > 0)
            update_tofh(&begin_fiv, &end, 1, day_over);
        if (off_data_haf_num > 0)
            update_tofh(&begin_haf, &end, 0, day_over);

        // 0시 5분 보다 빠른 시점에 전원단절 발생 시,
        // 일일마감 데이터 생성
        if (begin_fiv_time.tm_hour < 1 && begin_fiv_time.tm_min < 5)
        {
            process_day(&begin_fiv);
            update_tddh(&begin_fiv, 1);
            update_tddh(&begin_fiv, 0);

            enqueue_tddh_to_transmit(&begin_fiv);
        }

        // 다음날 00시 00분 부터 시작으로 변경
        for (int i = 0; i < 2; i++)
        {
            struct tm *nextTime;
            if (i == 0)
                nextTime = &t.off_fiv;
            else
                nextTime = &t.off_haf;

            nextTime->tm_mday += 1;
            if (nextTime->tm_mday > days_in_month[nextTime->tm_mon])
            {
                if (nextTime->tm_mon == 1 && (nextTime->tm_year % 4 == 0 && (nextTime->tm_year % 100 != 0 || nextTime->tm_year % 400 == 0)))
                {
                    if (nextTime->tm_mday > 29) // 윤달
                    {
                        nextTime->tm_mday = 1;
                        nextTime->tm_mon++;
                    }
                    else
                    {
                        nextTime->tm_mday = 1;
                        nextTime->tm_mon++;
                    }

                    if (nextTime->tm_mon > 11)
                    {
                        nextTime->tm_mon = 0;
                        nextTime->tm_year++;
                    }
                }
            }

            nextTime->tm_hour = 0;
            nextTime->tm_min = 0;
            nextTime->tm_sec = 0;

            begin_t = mktime(nextTime);
        }
    }
}

void update_tofh(time_t *begin, time_t *end, int seg, int day_over)
{
    char dataTime_str[50], buffer[BUFFER_SIZE], newBuffer[BUFFER_SIZE], crcBuffer[5], query_str[BUFFER_SIZE], beginTime_str[50], endTime_str[50], insertTime_str[50];

    time_t datetime;

    if (seg == 1)
    {
        for (datetime = *begin; datetime <= *end; datetime += FIVSEC)
        {
            update_5min_state(&datetime, 1);
            update_5min_relation(&datetime, 1);
            update_tdah(&datetime, 1, 1);

            enqueue_tdah_to_transmit(&datetime, 1);
            usleep(10000); // 10msec delay
        }
    }
    else
    {
        for (datetime = *begin; datetime <= *end; datetime += HAFSEC)
        {
            process_30min(&datetime, 1);
            update_tdah(&datetime, 0, 1);
            update_tnoh(&datetime, 1);

            enqueue_tdah_to_transmit(&datetime, 1);
            usleep(10000); // 10msec delay
        }
    }

    if (seg == 1)
        datetime -= FIVSEC;
    else
        datetime -= HAFSEC;

    // TOFH 데이터 생성
    struct tm begin_time = *localtime(begin);
    struct tm end_time = *localtime(&datetime);

    snprintf(beginTime_str, sizeof(beginTime_str), "%4d-%02d-%02d %02d:%02d:%02d",
             begin_time.tm_year + 1900, begin_time.tm_mon + 1, begin_time.tm_mday,
             begin_time.tm_hour, begin_time.tm_min, begin_time.tm_sec);

    snprintf(endTime_str, sizeof(endTime_str), "%4d-%02d-%02d %02d:%02d:%02d",
             end_time.tm_year + 1900, end_time.tm_mon + 1, end_time.tm_mday,
             end_time.tm_hour, end_time.tm_min, end_time.tm_sec);

    snprintf(insertTime_str, sizeof(insertTime_str), "%4d-%02d-%02d %02d:%02d:%02d",
             end_time.tm_year + 1900, end_time.tm_mon + 1, end_time.tm_mday,
             end_time.tm_hour, end_time.tm_min, end_time.tm_sec);

    snprintf(dataTime_str, sizeof(dataTime_str), "%04d%02d%02d",
             begin_time.tm_year + 1900, begin_time.tm_mon + 1, begin_time.tm_mday);

    MYSQL *conn = get_conn(); // 데이터베이스 연결 획득

    for (int i = 0; i < NUM_CHIMNEY; i++)
    {
        if (seg == 1)
        {
            snprintf(query_str, sizeof(query_str), "SELECT tim_date FROM t_05tdah WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND off = 1 AND chim_id = %d FOR UPDATE;", beginTime_str, endTime_str, i + 1);
        }
        else
        {
            snprintf(query_str, sizeof(query_str), "SELECT tim_date FROM t_30tdah WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND off = 1 AND chim_id = %d FOR UPDATE", beginTime_str, endTime_str, i + 1);
        }

        execute_query(conn, query_str);
        MYSQL_RES *res = mysql_store_result(conn); // 쿼리 결과 담기

        int num = 0;
        if ((num = mysql_num_rows(res)) > 0)
        {
            char timeBuff[num][20];

            for (int j = 0; j < num; j++)
            {
                MYSQL_ROW row = mysql_fetch_row(res);
                if (row)
                {
                    strcpy(timeBuff[j], row[0]);
                    if (seg == 1)
                        printf("(5분)전원단절구간 시간: %s\n", timeBuff[j]);
                    else
                        printf("(30분)전원단절구간 시간: %s\n", timeBuff[j]);
                }
            }
            mysql_free_result(res);

            CHIMNEY *c = &chimney[i];

            snprintf(buffer, sizeof(buffer), "TOFH%7s%3s", c->business, c->chimney);
            uint16_t total_len = 34 + (num * 4);

            if (seg == 1)
                snprintf(newBuffer, sizeof(newBuffer), "%4dFIV%8s%3d", total_len, dataTime_str, num);
            else
                snprintf(newBuffer, sizeof(newBuffer), "%4dHAF%8s%3d", total_len, dataTime_str, num);
            strcat(buffer, newBuffer);

            for (int j = 0; j < num; j++)
            {
                snprintf(newBuffer, sizeof(newBuffer), "%c%c%c%c", timeBuff[j][11], timeBuff[j][12], timeBuff[j][14], timeBuff[j][15]);
                strcat(buffer, newBuffer);
            }

            uint16_t crc = crc16_ccitt_false((uint8_t *)buffer, total_len - 2);
            snprintf(crcBuffer, sizeof(crcBuffer), "%04X", crc);

            // 확인용
            if (seg == 1)
                printf("전원단절 5분[%s]: %s%s\n", insertTime_str, buffer, crcBuffer);
            else
                printf("전원단절 30분[%s]: %s%s\n", insertTime_str, buffer, crcBuffer);

            // TOFH 테이블에 저장
            if (seg == 1)
            {
                if (c->send_mode == 0 || day_over) // 30분 전송 모드인 경우, 5분 전원단절 데이터 전송하지 않음.
                    snprintf(query_str, sizeof(query_str), "INSERT IGNORE INTO t_05tofh (insert_time, cmd, _data, crc, chim_id, send) VALUE (\"%s\", \"%s\", \"%s\", \"%s\", %d, 1);", insertTime_str, "TOFH", buffer + 4, crcBuffer, i + 1);
                else
                    snprintf(query_str, sizeof(query_str), "INSERT IGNORE INTO t_05tofh (insert_time, cmd, _data, crc, chim_id, send) VALUE (\"%s\", \"%s\", \"%s\", \"%s\", %d, 0);", insertTime_str, "TOFH", buffer + 4, crcBuffer, i + 1);
            }
            else
            {
                if (c->send_mode == 2 || day_over) // 5분 전송 모드인 경우, 30분 전원단절 데이터 전송하지 않음.
                    snprintf(query_str, sizeof(query_str), "INSERT IGNORE INTO t_30tofh (insert_time, cmd, _data, crc, chim_id, send) VALUE (\"%s\", \"%s\", \"%s\", \"%s\", %d, 1);", insertTime_str, "TOFH", buffer + 4, crcBuffer, i + 1);
                else
                    snprintf(query_str, sizeof(query_str), "INSERT IGNORE INTO t_30tofh (insert_time, cmd, _data, crc, chim_id, send) VALUE (\"%s\", \"%s\", \"%s\", \"%s\", %d, 0);", insertTime_str, "TOFH", buffer + 4, crcBuffer, i + 1);
            }

            execute_query(conn, query_str);
        }
    }
    release_conn(conn); // 데이터베이스 연결 해제

    // TOFH 전원단절 생성 데이터 전송할 큐에 담기
    enqueue_tofh_to_transmit(&datetime);
}