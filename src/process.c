#include "process.h"

int32_t sum_cnt, sec_value[MAX_NUM], sum_value[MAX_NUM],
    fiv_value[MAX_NUM], haf_value[MAX_NUM];

const int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

void process_5sec()
{
    update_5sec_data(); // 5초 데이터 업데이트

    update_5sec_state(); // 5초 상태정보 업데이트

    update_5sec_relation(); // 5초 관계정보 업데이트
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

void update_5sec_relation()
{
    char query_str[BUFFER_SIZE], time_str[300];

    struct tm *t = &current_time;

    snprintf(time_str, sizeof(time_str), "%4d-%02d-%02d %02d:%02d:%02d",
             t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);

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

            snprintf(query_str, sizeof(query_str), "INSERT IGNORE INTO t_05sec (tim_date, fac_id, fac, _data, stat, run, rel, chim_id) VALUES(\'%s\', %d, \'%s\', %d, %d, %d, %d, %d);",
                     time_str, j, f->name, f->SEC.value, f->SEC.stat, f->SEC.run, f->SEC.rel, i + 1);
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
                           time_str, f->name, f->item, upper, lower, f->SEC.stat, f->SEC.run, f->SEC.rel, f->delay.ond, f->delay.ofd);
                else
                    printf("%s (5초) %5s(%1s): -%2d.%02u | 자료상태 %1d | 가동상태 %1d | 방지정상 %1d | 가동유예 %2d | 중지유예 %2d\n",
                           time_str, f->name, f->item, upper, lower, f->SEC.stat, f->SEC.run, f->SEC.rel, f->delay.ond, f->delay.ofd);
            }
            else
                printf("%s (5초) %5s(%1s): %3d.%02u | 자료상태 %1d | 가동상태 %1d | 방지정상 %1d | 가동유예 %2d | 중지유예 %2d\n",
                       time_str, f->name, f->item, f->SEC.value / 100, f->SEC.value % 100, f->SEC.stat, f->SEC.run, f->SEC.rel, f->delay.ond, f->delay.ofd);
        }
        printf("\n");
    }

    release_conn(conn); // 데이터베이스 연결 해제
}

void process_5min()
{
    update_5min_data();

    update_5min_state();

    update_5min_relation();

    update_tdah(1, 0);
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

void update_5min_state()
{
    char query_str[BUFFER_SIZE], stime_str[300], etime_str[300];

    time_t stime = mktime(&current_time);
    stime -= FIVSEC;
    struct tm *st = localtime(&stime);
    struct tm *et = &current_time;

    snprintf(stime_str, sizeof(stime_str), "%4d-%02d-%02d %02d:%02d",
             st->tm_year + 1900, st->tm_mon + 1, st->tm_mday, st->tm_hour, st->tm_min);
    snprintf(etime_str, sizeof(etime_str), "%4d-%02d-%02d %02d:%02d",
             et->tm_year + 1900, et->tm_mon + 1, et->tm_mday, et->tm_hour, et->tm_min);

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
                     "FROM t_05sec WHERE tim_date >= \'%s\' AND tim_date < \'%s\' "
                     "AND fac_id = %d AND chim_id = %d FOR UPDATE;",
                     stime_str, etime_str, j, i + 1);

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

                printf("[%s](5분) %s(%s) (상태정보)정상 %2d | 비정상 %2d | 통신불량 %2d | 전원단절 %2d | 점검중 %2d\n",
                       etime_str, f->name, f->item, nrm, low, com, off, chk);
            }
            mysql_free_result(res);
        }
    }
    release_conn(conn); // 데이터베이스 연결 해제
}

void update_5min_relation()
{
    char query_str[BUFFER_SIZE], etime_str[300], delete_str[300];

    time_t dtime = mktime(&current_time);
    dtime -= FIVSEC;
    struct tm *dt = localtime(&dtime);
    struct tm *et = &current_time;

    snprintf(delete_str, sizeof(delete_str), "%4d-%02d-%02d %02d:%02d",
             dt->tm_year + 1900, dt->tm_mon + 1, dt->tm_mday, dt->tm_hour, dt->tm_min);
    snprintf(etime_str, sizeof(etime_str), "%4d-%02d-%02d %02d:%02d",
             et->tm_year + 1900, et->tm_mon + 1, et->tm_mday, et->tm_hour, et->tm_min);

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

            printf("[%s](5분) %s(%s) (방지정상)%2d (9: 가동유예, 8: 중지유예, 3: 해당없음, 1: 정상, 0: 비정상)\n",
                   etime_str, f->name, f->item, f->FIV.rel);

            // 5분 상태 정보 데이터 베이스 저장
            snprintf(query_str, sizeof(query_str), "INSERT IGNORE INTO t_05stat (tim_date, fac_id, fac, _data, stat, run, rel, chim_id) VALUES (\"%s\", %d, \"%s\", %d, %d, %d, %d, %d);",
                     etime_str, j, f->name, f->FIV.value, f->FIV.stat, f->FIV.run, f->FIV.rel, i + 1);
            execute_query(conn, query_str);
        }
        printf("\n");

        snprintf(query_str, sizeof(query_str), "DELETE FROM t_05sec WHERE tim_date <= \'%s\' and chim_id = %d;", delete_str, i + 1);
        execute_query(conn, query_str);
    }

    release_conn(conn); // 데이터베이스 연결 해제

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
                           etime_str, f->name, f->item, upper, lower, f->FIV.stat, f->FIV.run, f->FIV.rel, f->delay.ond, f->delay.ofd);
                else
                    printf("%s (5분) %5s(%1s): -%2d.%02u | 상태정보 %1d | 가동유무 %1d | 방지정상 %1d | 가동유예 %2d | 중지유예 %2d\n",
                           etime_str, f->name, f->item, upper, lower, f->FIV.stat, f->FIV.run, f->FIV.rel, f->delay.ond, f->delay.ofd);
            }
            else
                printf("%s (5분) %5s(%1s): %3d.%02u | 상태정보 %1d | 가동유무 %1d | 방지정상 %1d | 가동유예 %2d | 중지유예 %2d\n",
                       etime_str, f->name, f->item, f->FIV.value / 100, f->FIV.value % 100, f->FIV.stat, f->FIV.run, f->FIV.rel, f->delay.ond, f->delay.ofd);
        }
        printf("\n");
    }
}

void process_30min()
{
    char query_str[BUFFER_SIZE], stime_str[300], etime_str[300];

    time_t stime = mktime(&current_time);
    stime -= HAFSEC;
    struct tm *st = localtime(&stime);
    struct tm *et = &current_time;

    snprintf(stime_str, sizeof(stime_str), "%4d-%02d-%02d %02d:%02d",
             st->tm_year + 1900, st->tm_mon + 1, st->tm_mday, st->tm_hour, st->tm_min);
    snprintf(etime_str, sizeof(etime_str), "%4d-%02d-%02d %02d:%02d",
             et->tm_year + 1900, et->tm_mon + 1, et->tm_mday, et->tm_hour, et->tm_min);

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
                     "FROM t_05stat WHERE tim_date >= \'%s\' AND tim_date < \'%s\' "
                     "AND fac_id = %d AND chim_id = %d;",
                     stime_str, etime_str, j, i + 1);

            execute_query(conn, query_str);
            MYSQL_RES *res = mysql_store_result(conn); // 쿼리 결과 담기
            if (res == NULL)
            {
                release_conn(conn); // 데이터베이스 연결 해제
                return;
            }

            FACILITY *f = &(c->facility[i]);
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

                if (nrm > (HAFNUM / 2)) // 정상
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

                printf("[%s](30분) %s(%s) (상태정보)정상 %2d | 비정상 %2d | 통신불량 %2d | 전원단절 %2d | 점검중 %2d\n (방지정상)가동유예 %2d | 중지유예 %2d | 비정상 %2d | 정상 %2d | 해당없음 %2d\n", etime_str, f->name, f->item, nrm, low, com, off, chk, ond, ofd, err, okk, naa);

                // 30분 상태 정보 데이터 베이스 저장
                snprintf(query_str, sizeof(query_str), "INSERT IGNORE INTO t_30stat (tim_date, fac_id, fac, _data, stat, run, rel, chim_id) VALUES (\"%s\", %d, \"%s\", %d, %d, %d, %d, %d);", etime_str, i, f->name, f->HAF.value, f->HAF.stat, f->HAF.run, f->HAF.rel, i + 1);
                execute_query(conn, query_str);
            }

            mysql_free_result(res);
        }
        printf("\n");
    }

    release_conn(conn); // 데이터베이스 연결 해제

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
                           etime_str, f->name, f->item, upper, lower, f->HAF.stat, f->HAF.run, f->HAF.rel);
                else
                    printf("%s (30분) %5s(%1s): -%2d.%02u | 상태정보 %1d | 가동유무 %1d | 방지정상 %1d\n",
                           etime_str, f->name, f->item, upper, lower, f->HAF.stat, f->HAF.run, f->HAF.rel);
            }
            else
                printf("%s (30분) %5s(%1s): %3d.%02u | 상태정보 %1d | 가동유무 %1d | 방지정상 %1d\n",
                       etime_str, f->name, f->item, f->HAF.value / 100, f->HAF.value % 100, f->HAF.stat, f->HAF.run, f->HAF.rel);
        }
        printf("\n");
    }

    update_tdah(0, 0);
}

void update_tdah(int seg, int off)
{
    char data_segment[4], data_time[30], insert_time[100], buffer[BUFFER_SIZE], newBuffer[BUFFER_SIZE], crcBuffer[5], query_str[BUFFER_SIZE];

    time_t stime = mktime(&current_time);

    if (seg == 1) // 5min
    {
        stime -= FIVSEC;
        strcpy(data_segment, "FIV");
    }
    else
    {
        stime -= HAFSEC;
        strcpy(data_segment, "HAF");
    }

    struct tm *it = &current_time;
    struct tm *dt = localtime(&stime);

    snprintf(insert_time, sizeof(insert_time), "%4d-%02d-%02d %02d:%02d",
             it->tm_year + 1900, it->tm_mon + 1, it->tm_mday, it->tm_hour, it->tm_min);
    snprintf(data_time, sizeof(data_time), "%02d%02d%02d%02d%02d",
             (dt->tm_year + 1900) % 100, dt->tm_mon + 1, dt->tm_mday, dt->tm_hour, dt->tm_min);

    MYSQL *conn = get_conn(); // 데이터베이스 연결 획득

    for (int i = 0; i < NUM_CHIMNEY; i++)
    {
        CHIMNEY *c = &chimney[i];

        snprintf(buffer, sizeof(buffer), "TDAH%7s%3s", c->business, c->chimney);

        uint16_t total_len = 35 + (c->num_facility * 15);
        snprintf(newBuffer, sizeof(newBuffer), "%4d%3s%10s%2d", total_len, data_segment, data_time, c->num_facility);
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
            printf("%s(5분TDAH) %s%s\n", insert_time, buffer, crcBuffer);
        else
            printf("%s(30분TDAH) %s%s\n", insert_time, buffer, crcBuffer);

        // TDAH 테이블에 저장
        if (seg == 1)
        {
            if (TOFH > 0 && off)
                snprintf(query_str, sizeof(query_str), "INSERT IGNORE INTO t_05tdah (tim_date, off, cmd, _data, crc, chim_id, send) VALUES (\"%s\", 1, \"%s\", \"%s\", \"%s\", %d, 1);",
                         insert_time, "TDAH", buffer + 4, crcBuffer, i + 1);
            else if (TOFH > 0 && !off)
                snprintf(query_str, sizeof(query_str), "INSERT IGNORE INTO t_05tdah (tim_date, off, cmd, _data, crc, chim_id, send) VALUES (\"%s\", 1, \"%s\", \"%s\", \"%s\", %d, 0);",
                         insert_time, "TDAH", buffer + 4, crcBuffer, i + 1);
            else if (c->send_mode == 0)
                snprintf(query_str, sizeof(query_str), "INSERT IGNORE INTO t_05tdah (tim_date, off, cmd, _data, crc, chim_id, send) VALUES (\"%s\", 0, \"%s\", \"%s\", \"%s\", %d, 1);",
                         insert_time, "TDAH", buffer + 4, crcBuffer, i + 1);
            else
                snprintf(query_str, sizeof(query_str), "INSERT IGNORE INTO t_05tdah (tim_date, off, cmd, _data, crc, chim_id, send) VALUES (\"%s\", 0, \"%s\", \"%s\", \"%s\", %d, 0);",
                         insert_time, "TDAH", buffer + 4, crcBuffer, i + 1);
        }
        else
        {
            if (TOFH > 0 && off)
                snprintf(query_str, sizeof(query_str), "INSERT IGNORE INTO t_30tdah (tim_date, off, cmd, _data, crc, chim_id, send) VALUES (\"%s\", 1, \"%s\", \"%s\", \"%s\", %d, 1);",
                         insert_time, "TDAH", buffer + 4, crcBuffer, i + 1);
            else if (TOFH > 0 && !off)
                snprintf(query_str, sizeof(query_str), "INSERT IGNORE INTO t_30tdah (tim_date, off, cmd, _data, crc, chim_id, send) VALUES (\"%s\", 1, \"%s\", \"%s\", \"%s\", %d, 0);",
                         insert_time, "TDAH", buffer + 4, crcBuffer, i + 1);
            else if (c->send_mode == 2)
                snprintf(query_str, sizeof(query_str), "INSERT IGNORE INTO t_30tdah (tim_date, off, cmd, _data, crc, chim_id, send) VALUES (\"%s\", 0, \"%s\", \"%s\", \"%s\", %d, 1);",
                         insert_time, "TDAH", buffer + 4, crcBuffer, i + 1);
            else
                snprintf(query_str, sizeof(query_str), "INSERT IGNORE INTO t_30tdah (tim_date, off, cmd, _data, crc, chim_id, send) VALUES (\"%s\", 0, \"%s\", \"%s\", \"%s\", %d, 0);",
                         insert_time, "TDAH", buffer + 4, crcBuffer, i + 1);
        }

        execute_query(conn, query_str);

        if (TOFH > 0 && off == 0) // 해당 구간은 전원단절 데이터도 생성해줘야 함.
        {
            snprintf(buffer, sizeof(buffer), "TOFH%7s%3s", c->business, c->chimney);
            uint16_t total_len = 34 + (1 * 4);

            snprintf(data_time, sizeof(data_time), "%04d%02d%02d",
                     dt->tm_year + 1900, dt->tm_mon + 1, dt->tm_mday);

            if (seg == 1)
                snprintf(newBuffer, sizeof(newBuffer), "%4dFIV%8s%3d", total_len, data_time, 1);
            else
                snprintf(newBuffer, sizeof(newBuffer), "%4dHAF%8s%3d", total_len, data_time, 1);

            snprintf(newBuffer, sizeof(newBuffer), "%02d%02d", it->tm_hour, it->tm_min);
            strcat(buffer, newBuffer);

            uint16_t crc = crc16_ccitt_false((uint8_t *)buffer, total_len - 2);
            snprintf(crcBuffer, sizeof(crcBuffer), "%04X", crc);

            if (seg == 1)
                printf("[%s](5분TOFH) %s%s\n", insert_time, buffer, crcBuffer);
            else
                printf("[%s](30분TOFH) %s%s\n", insert_time, buffer, crcBuffer);

            if (seg == 1)
                snprintf(query_str, sizeof(query_str), "INSERT IGNORE INTO t_05tofh (tim_date, cmd, _data, crc, chim_id, send) VALUE (\"%s\", \"%s\", \"%s\", \"%s\", %d, 1);", insert_time, "TOFH", buffer + 4, crcBuffer, i + 1);
            else
                snprintf(query_str, sizeof(query_str), "INSERT IGNORE INTO t_30tofh (tim_date, cmd, _data, crc, chim_id, send) VALUE (\"%s\", \"%s\", \"%s\", \"%s\", %d, 1);", insert_time, "TOFH", buffer + 4, crcBuffer, i + 1);

            execute_query(conn, query_str);
        }
    }

    // 30일 전 데이터 삭제
    struct tm remove = current_time;
    time_t removeT = mktime(&remove);
    removeT -= (DAYSEC * 30);
    remove = *localtime(&removeT);

    char remove_time[100];
    snprintf(remove_time, sizeof(remove_time), "%4d-%02d-%02d %02d:%02d",
             remove.tm_year + 1900, remove.tm_mon + 1, remove.tm_mday, remove.tm_hour, remove.tm_min);

    if (seg == 1)
        snprintf(query_str, sizeof(query_str), "DELETE FROM t_05tdah WHERE tim_date <= \'%s\';", remove_time);
    else
        snprintf(query_str, sizeof(query_str), "DELETE FROM t_30tdah WHERE tim_date <= \'%s\';", remove_time);

    execute_query(conn, query_str);
    release_conn(conn);
}