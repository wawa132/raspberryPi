#include "database.h"

MYDB db;
CHIMNEY chimney[CHIMNEY_NUM];
INFORM info[CHIMNEY_NUM];

uint8_t NUM_CHIMNEY;
uint16_t SYNC_TIME;

void init_db()
{
    pthread_mutex_init(&db.lock, NULL);
    pthread_cond_init(&db.cond, NULL);

    db.conn = mysql_init(NULL);
    if (!mysql_real_connect(db.conn, DB_HOST, DB_USER, DB_PASS, DB_NAME, 3306, NULL, 0))
    {
        fprintf(stderr, "DB connection err: %s\n", mysql_error(db.conn));
        exit(EXIT_FAILURE);
    }

    db.usable = 1;
}

void destroy_db()
{
    if (db.conn)
    {
        mysql_close(db.conn);
    }
    pthread_mutex_destroy(&db.lock);
    pthread_cond_destroy(&db.cond);

    printf("database disconnect...\n");
}

MYSQL *get_conn()
{
    pthread_mutex_lock(&db.lock);
    while (1)
    {
        if (db.usable)
        {
            db.usable = 0; // change unusable
            pthread_mutex_unlock(&db.lock);

            printf("db get\n");
            return db.conn;
        }

        // wait
        pthread_cond_wait(&db.cond, &db.lock);
    }
}

void release_conn(MYSQL *conn)
{
    pthread_mutex_lock(&db.lock);
    execute_query(conn, "FLUSH TABLES");

    if (db.conn == conn)
    {
        db.usable = 1; // change usable
        printf("db release\n");
    }

    pthread_cond_signal(&db.cond); // awake waiting thread
    pthread_mutex_unlock(&db.lock);
}

void init_facility()
{
    char query_str[BUFFER_SIZE];

    MYSQL *conn = get_conn();

    printf("Initialize Chimney...\n");

    snprintf(query_str, sizeof(query_str), "select * from t_chimney;");

    execute_query(conn, query_str);
    MYSQL_RES *res = mysql_store_result(conn);

    NUM_CHIMNEY = mysql_num_rows(res);

    for (int i = 0; i < NUM_CHIMNEY; i++)
    {
        MYSQL_ROW row = mysql_fetch_row(res);

        if (row)
        {
            CHIMNEY *c = &chimney[i];

            // row[0] : chimeny id
            strcpy(c->business, row[1]);
            strcpy(c->chimney, row[2]);
            c->num_facility = atoi(row[3]);
            c->time_resend = atoi(row[4]);
            c->send_mode = atoi(row[5]);
            c->delay.ond = atoi(row[6]);
            c->delay.ofd = atoi(row[7]);
            c->reboot = atoi(row[8]);

            SYNC_TIME = atoi(c->business);
            SYNC_TIME %= 24;

            printf("사업장코드: %s | 굴뚝코드: %s | 시설갯수: %d | 전송모드: %d | ",
                   c->business, c->chimney, c->num_facility, c->send_mode);
            printf("가동유예: %d | 중지유예: %d | 미전송자료(전송시간): %d | 재부팅여부: %d | 시간동기화: %d\n",
                   c->delay.ond, c->delay.ofd, c->time_resend, c->reboot, SYNC_TIME);
        }
    }

    mysql_free_result(res);

    printf("Initialize Information...\n");

    snprintf(query_str, sizeof(query_str), "select * from t_info;");

    execute_query(conn, query_str);
    res = mysql_store_result(conn);

    for (int i = 0; i < NUM_CHIMNEY; i++)
    {
        MYSQL_ROW row = mysql_fetch_row(res);

        if (row)
        {
            INFORM *m = &info[i];

            strcpy(m->sv_ip, row[1]);
            m->sv_port = atoi(row[2]);
            m->remote_port = atoi(row[3]);
            strcpy(m->gw_ip, row[4]);
            strcpy(m->manufacture_code, row[5]);
            strcpy(m->model, row[6]);
            strcpy(m->version, row[7]);
            strcpy(m->hash, row[8]);
            strcpy(m->passwd, row[9]);
            strcpy(m->upgrade.assign_ip, row[10]);
            m->upgrade.result = atoi(row[11]);
            strcpy(m->ntp_ip, row[12]);
            m->ntp_port = atoi(row[13]);

            printf("sv_ip: %s | sv_port: %5d | gw_ip: %15s | manufacture_code: %2s | version: %s | ",
                   m->sv_ip, m->sv_port, m->gw_ip, m->manufacture_code, m->version);
            printf("hash-code: %s | passwd: %10s | upgrade_ip: %15s | up_result: %d | ntp_ip: %s | ntp_port: %d\n",
                   m->hash, m->passwd, m->upgrade.assign_ip, m->upgrade.result, m->ntp_ip, m->ntp_port);
        }
    }

    mysql_free_result(res);

    printf("Initialize Facility...\n");

    for (int i = 0; i < NUM_CHIMNEY; i++)
    {
        snprintf(query_str, sizeof(query_str), "select * from t_facility where chim_id = %d;", i + 1);
        execute_query(conn, query_str);
        res = mysql_store_result(conn);

        CHIMNEY *c = &chimney[i];

        for (int j = 0; j < c->num_facility; j++)
        {
            MYSQL_ROW row = mysql_fetch_row(res);

            if (row)
            {
                FACILITY *f = &(c->facility[j]);

                strcpy(f->name, row[1]);
                strcpy(f->item, row[2]);
                f->range.min = atoi(row[3]);
                f->range.max = atoi(row[4]);
                f->range.std = atoi(row[5]);
                f->rel_facility = atoi(row[6]);
                f->check = atoi(row[7]);
                f->port = atoi(row[8]);
                f->delay.ond = c->delay.ond / 5;
                f->delay.ofd = c->delay.ofd / 5;

                if (f->name[0] == 'E')
                {
                    if (f->item[0] == 'A')
                        f->type = 0;
                    else if (f->item[0] == 'b' || f->item[0] == 'c' || f->item[0] == 'd')
                        f->type = 4;
                    else
                        f->type = 3;
                }
                else if (f->name[0] == 'P')
                {
                    if (f->item[0] == 'b' || f->item[0] == 'c' || f->item[0] == 'd')
                        f->type = 4;
                    else
                        f->type = 3;
                }
                else if (f->name[0] == 'F')
                {
                    if (f->item[0] == 'b' || f->item[0] == 'c' || f->item[0] == 'd')
                        f->type = 4;
                    else
                        f->type = 3;
                }

                f->delay.bsec_run = 0;
                f->delay.bfiv_run = 0;
                f->delay.lock = 1;

                printf("시설 %s | 항목 %s | 최소값 %5d | 최대값 %5d | 기준값: %5d | ",
                       f->name, f->item, f->range.min, f->range.max, f->range.std);
                printf("관계시설번호 %2d | 점검중 %2d | 연결포트 %2d | 시설타입 %d | 가동유예 %d | 중지유예 %d\n",
                       f->rel_facility, f->check, f->port, f->type, f->delay.ond, f->delay.ofd);
            }
        }

        mysql_free_result(res);
    }
    release_conn(conn);
}

void execute_query(MYSQL *conn, const char *query_str)
{
    if (mysql_query(conn, query_str))
    {
        fprintf(stderr, "Execute query failed: %s\n", mysql_error(conn));
        exit(EXIT_FAILURE);
    }
}

void enqueue_tdah_to_transmit(time_t *datetime)
{
    // rangeTime_str 은 TNOH 발생으로 인해 FIV 자료가 필요할 때, 시작 시점을 기록하는 버퍼
    char query_str[300], update_str[300], loadTime_str[50], rangeTime_str[50];

    struct tm *load_time = localtime(datetime);
    snprintf(loadTime_str, sizeof(loadTime_str), "%4d-%02d-%02d %02d:%02d:%02d",
             load_time->tm_year + 1900, load_time->tm_mon + 1, load_time->tm_mday,
             load_time->tm_hour, load_time->tm_min, load_time->tm_sec);

    time_t range_t = *datetime;
    range_t -= HAFSEC;
    struct tm range_time = *localtime(&range_t);
    snprintf(rangeTime_str, sizeof(rangeTime_str), "%4d-%02d-%02d %02d:%02d:%02d",
             range_time.tm_year + 1900, range_time.tm_mon + 1, range_time.tm_mday,
             range_time.tm_hour, range_time.tm_min, range_time.tm_sec);

    for (int i = 0; i < NUM_CHIMNEY; i++)
    {
        CHIMNEY *c = &chimney[i];
        DATA_Q *q = &data_q[i];

        if (c->send_mode == 0) // 30min data transmit
        {
            // get 30min tdah
            snprintf(query_str, sizeof(query_str), "SELECT CONCAT(cmd, _data) FROM t_30tdah WHERE insert_time = \'%s\' AND chim_id = %d AND send = 0;", loadTime_str, i + 1);
            snprintf(update_str, sizeof(update_str), "UPDATE t_30tdah SET send = 1 WHERE insert_time = \'%s\' AND chim_id = %d AND send = 0;", loadTime_str, i + 1);

            if (enqueue_transmit_data(q, query_str, update_str) > 0)
            {
                // get TNOH data
                snprintf(query_str, sizeof(query_str), "SELECT CONCAT(cmd, _data) FROM t_05tnoh WHERE insert_time = \'%s\' AND chim_id = %d AND send = 0;", loadTime_str, i + 1);
                snprintf(update_str, sizeof(update_str), "UPDATE t_05tnoh SET send = 1 WHERE insert_time = \'%s\' AND chim_id = %d AND send = 0;", loadTime_str, i + 1);

                if (enqueue_transmit_data(q, query_str, update_str) > 0)
                {
                    // get FIV data that evidence TNOH
                    snprintf(query_str, sizeof(query_str), "SELECT CONCAT(cmd, _data) FROM t_05tdah WHERE insert_time > \'%s\' AND insert_time <= \'%s\' AND chim_id = %d;", rangeTime_str, loadTime_str, i + 1);
                    snprintf(update_str, sizeof(update_str), ";");

                    if (enqueue_transmit_data(q, query_str, update_str) < 0)
                        printf("=== No exists TDAH(HAF) evidence data(FIV) to transmit===\n");
                }
                else
                    printf("=== No exists TNOH data to transmit ===\n");
            }
            else
                printf("=== No exists TDAH(HAF) data to transmit ===\n");
        }
        else if (c->send_mode == 1) // 5min, 30min data transmit
        {
            // get 5min tdah
            snprintf(query_str, sizeof(query_str), "SELECT CONCAT(cmd, _data) FROM t_05tdah WHERE insert_time = \'%s\' AND chim_id = %d AND send = 0;", loadTime_str, i + 1);
            snprintf(update_str, sizeof(update_str), "UPDATE t_05tdah SET send = 1 WHERE insert_time = \'%s\' AND chim_id = %d AND send = 0;", loadTime_str, i + 1);

            if (enqueue_transmit_data(q, query_str, update_str) < 0)
                printf("=== No exists TDAH(FIV) data to transmit ===\n");

            // get 30min tdah
            snprintf(query_str, sizeof(query_str), "SELECT CONCAT(cmd, _data) FROM t_30tdah WHERE insert_time = \'%s\' AND chim_id = %d AND send = 0;", loadTime_str, i + 1);
            snprintf(update_str, sizeof(update_str), "UPDATE t_30tdah SET send = 1 WHERE insert_time = \'%s\' AND chim_id = %d AND send = 0;", loadTime_str, i + 1);

            if (enqueue_transmit_data(q, query_str, update_str) < 0)
                printf("=== No exists TDAH(HAF) data to transmit ===\n");
        }
        else // 5min data transmit
        {
            // get 5min tdah
            snprintf(query_str, sizeof(query_str), "SELECT CONCAT(cmd, _data) FROM t_05tdah WHERE insert_time = \'%s\' AND chim_id = %d AND send = 0;", loadTime_str, i + 1);
            snprintf(update_str, sizeof(update_str), "UPDATE t_05tdah SET send = 1 WHERE insert_time = \'%s\' AND chim_id = %d AND send = 0;", loadTime_str, i + 1);

            if (enqueue_transmit_data(q, query_str, update_str) < 0)
                printf("=== No exists TDAH(FIV) data to transmit ===\n");
        }

        // miss_send_data_check
        if (c->time_resend == 9999 || (load_time->tm_hour == (c->time_resend / 100) && load_time->tm_min == (c->time_resend % 100)))
        {
            enqueue_miss_to_transmit(datetime);
        }
    }
}

void enqueue_tddh_to_transmit(time_t *datetime)
{
    char query_str[300], update_str[300], loadTime_str[50];

    struct tm *load_time = localtime(datetime);
    snprintf(loadTime_str, sizeof(loadTime_str), "%4d-%02d-%02d %02d:%02d:%02d",
             load_time->tm_year + 1900, load_time->tm_mon + 1, load_time->tm_mday,
             load_time->tm_hour, load_time->tm_min, load_time->tm_sec);

    for (int i = 0; i < NUM_CHIMNEY; i++)
    {
        CHIMNEY *c = &chimney[i];
        DATA_Q *q = &data_q[i];

        if (c->send_mode == 0) // 30min data transmit
        {
            // get 30min tddh
            snprintf(query_str, sizeof(query_str), "SELECT CONCAT(cmd, _data) FROM t_30tddh WHERE insert_time = \'%s\' AND chim_id = %d AND send = 0;", loadTime_str, i + 1);
            snprintf(update_str, sizeof(update_str), "UPDATE t_30tddh SET send = 1 WHERE insert_time = \'%s\' AND chim_id = %d AND send = 0;", loadTime_str, i + 1);

            if (enqueue_transmit_data(q, query_str, update_str) < 0)
                printf("=== No exists TDDH(HAF) data to transmit ===\n");

            // get 30min tofh-day
            snprintf(query_str, sizeof(query_str), "SELECT CONCAT(cmd, _data) FROM t_30tofh_day WHERE insert_time = \'%s\' AND chim_id = %d AND send = 0;", loadTime_str, i + 1);
            snprintf(update_str, sizeof(update_str), "UPDATE t_30tofh_day SET send = 1 WHERE insert_time = \'%s\' AND chim_id = %d AND send = 0;", loadTime_str, i + 1);

            if (enqueue_transmit_data(q, query_str, update_str) < 0)
                printf("=== No exists TOFH-DAY(HAF) data to transmit ===\n");
        }
        else if (c->send_mode == 1) // 5min, 30min data transmit
        {
            // get 5min tddh
            snprintf(query_str, sizeof(query_str), "SELECT CONCAT(cmd, _data) FROM t_05tddh WHERE insert_time = \'%s\' AND chim_id = %d AND send = 0;", loadTime_str, i + 1);
            snprintf(update_str, sizeof(update_str), "UPDATE t_05tddh SET send = 1 WHERE insert_time = \'%s\' AND chim_id = %d AND send = 0;", loadTime_str, i + 1);

            if (enqueue_transmit_data(q, query_str, update_str) < 0)
                printf("=== No exists TDDH(FIV) data to transmit ===\n");

            // get 5min tofh-day
            snprintf(query_str, sizeof(query_str), "SELECT CONCAT(cmd, _data) FROM t_05tofh_day WHERE insert_time = \'%s\' AND chim_id = %d AND send = 0;", loadTime_str, i + 1);
            snprintf(update_str, sizeof(update_str), "UPDATE t_05tofh_day SET send = 1 WHERE insert_time = \'%s\' AND chim_id = %d AND send = 0;", loadTime_str, i + 1);

            if (enqueue_transmit_data(q, query_str, update_str) < 0)
                printf("=== No exists TOFH-DAY(FIV) data to transmit ===\n");

            // get 30min tddh
            snprintf(query_str, sizeof(query_str), "SELECT CONCAT(cmd, _data) FROM t_30tddh WHERE insert_time = \'%s\' AND chim_id = %d AND send = 0;", loadTime_str, i + 1);
            snprintf(update_str, sizeof(update_str), "UPDATE t_30tddh SET send = 1 WHERE insert_time = \'%s\' AND chim_id = %d AND send = 0;", loadTime_str, i + 1);

            if (enqueue_transmit_data(q, query_str, update_str) < 0)
                printf("=== No exists TDDH(HAF) data to transmit ===\n");

            // get 30min tofh-day
            snprintf(query_str, sizeof(query_str), "SELECT CONCAT(cmd, _data) FROM t_30tofh_day WHERE insert_time = \'%s\' AND chim_id = %d AND send = 0;", loadTime_str, i + 1);
            snprintf(update_str, sizeof(update_str), "UPDATE t_30tofh_day SET send = 1 WHERE insert_time = \'%s\' AND chim_id = %d AND send = 0;", loadTime_str, i + 1);

            if (enqueue_transmit_data(q, query_str, update_str) < 0)
                printf("=== No exists TOFH-DAY(HAF) data to transmit ===\n");
        }
        else // 5min data transmit
        {
            // get 5min tddh
            snprintf(query_str, sizeof(query_str), "SELECT CONCAT(cmd, _data) FROM t_05tddh WHERE insert_time = \'%s\' AND chim_id = %d AND send = 0;", loadTime_str, i + 1);
            snprintf(update_str, sizeof(update_str), "UPDATE t_05tddh SET send = 1 WHERE insert_time = \'%s\' AND chim_id = %d AND send = 0;", loadTime_str, i + 1);

            if (enqueue_transmit_data(q, query_str, update_str) < 0)
                printf("=== No exists TDDH(FIV) data to transmit ===\n");

            // get 5min tofh-day
            snprintf(query_str, sizeof(query_str), "SELECT CONCAT(cmd, _data) FROM t_05tofh_day WHERE insert_time = \'%s\' AND chim_id = %d AND send = 0;", loadTime_str, i + 1);
            snprintf(update_str, sizeof(update_str), "UPDATE t_05tofh_day SET send = 1 WHERE insert_time = \'%s\' AND chim_id = %d AND send = 0;", loadTime_str, i + 1);

            if (enqueue_transmit_data(q, query_str, update_str) < 0)
                printf("=== No exists TOFH-DAY(FIV) data to transmit ===\n");
        }
    }
}

void enqueue_tofh_to_transmit(time_t *datetime)
{
    char query_str[300], update_str[300], loadTime_str[50];

    struct tm *load_time = localtime(datetime);
    snprintf(loadTime_str, sizeof(loadTime_str), "%4d-%02d-%02d %02d:%02d:%02d",
             load_time->tm_year + 1900, load_time->tm_mon + 1, load_time->tm_mday,
             load_time->tm_hour, load_time->tm_min, load_time->tm_sec);

    for (int i = 0; i < NUM_CHIMNEY; i++)
    {
        CHIMNEY *c = &chimney[i];
        DATA_Q *q = &data_q[i];

        if (c->send_mode == 0) // 30min data transmit
        {
            // get 30min tofh
            snprintf(query_str, sizeof(query_str), "SELECT CONCAT(cmd, _data) FROM t_30tofh WHERE insert_time = \'%s\' AND chim_id = %d AND send = 0;", loadTime_str, i + 1);
            snprintf(update_str, sizeof(update_str), "UPDATE t_30tofh SET send = 1 WHERE insert_time = \'%s\' AND chim_id = %d AND send = 0;", loadTime_str, i + 1);

            if (enqueue_transmit_data(q, query_str, update_str) < 0)
                printf("=== No exists TOFH(HAF) data to transmit ===\n");
        }
        else if (c->send_mode == 1) // 5min, 30min data transmit
        {
            // get 5min tofh
            snprintf(query_str, sizeof(query_str), "SELECT CONCAT(cmd, _data) FROM t_05tofh WHERE insert_time = \'%s\' AND chim_id = %d AND send = 0;", loadTime_str, i + 1);
            snprintf(update_str, sizeof(update_str), "UPDATE t_05tofh SET send = 1 WHERE insert_time = \'%s\' AND chim_id = %d AND send = 0;", loadTime_str, i + 1);

            if (enqueue_transmit_data(q, query_str, update_str) < 0)
                printf("=== No exists TOFH(FIV) data to transmit ===\n");

            // get 30min tofh
            snprintf(query_str, sizeof(query_str), "SELECT CONCAT(cmd, _data) FROM t_30tofh WHERE insert_time = \'%s\' AND chim_id = %d AND send = 0;", loadTime_str, i + 1);
            snprintf(update_str, sizeof(update_str), "UPDATE t_30tofh SET send = 1 WHERE insert_time = \'%s\' AND chim_id = %d AND send = 0;", loadTime_str, i + 1);

            if (enqueue_transmit_data(q, query_str, update_str) < 0)
                printf("=== No exists TOFH(HAF) data to transmit ===\n");
        }
        else // 5min data transmit
        {
            // get 5min tofh
            snprintf(query_str, sizeof(query_str), "SELECT CONCAT(cmd, _data) FROM t_05tofh WHERE insert_time = \'%s\' AND chim_id = %d AND send = 0;", loadTime_str, i + 1);
            snprintf(update_str, sizeof(update_str), "UPDATE t_05tofh SET send = 1 WHERE insert_time = \'%s\' AND chim_id = %d AND send = 0;", loadTime_str, i + 1);

            if (enqueue_transmit_data(q, query_str, update_str) < 0)
                printf("=== No exists TOFH(FIV) data to transmit ===\n");
        }
    }
}

void enqueue_miss_to_transmit(time_t *datetime)
{
    char query_str[300], update_str[300], loadTime_str[50], loadHafTime_str[50], rangeTime_str[50];

    time_t load_t = *datetime;
    load_t -= FIVSEC;
    struct tm load_time = *localtime(&load_t);
    snprintf(loadTime_str, sizeof(loadTime_str), "%4d-%02d-%02d %02d:%02d:%02d",
             load_time.tm_year + 1900, load_time.tm_mon + 1, load_time.tm_mday,
             load_time.tm_hour, load_time.tm_min, load_time.tm_sec);

    time_t loadHaf_t = *datetime;
    loadHaf_t -= HAFSEC;
    struct tm loadHaf_time = *localtime(&loadHaf_t);
    snprintf(loadHafTime_str, sizeof(loadHafTime_str), "%4d-%02d-%02d %02d:%02d:%02d",
             loadHaf_time.tm_year + 1900, loadHaf_time.tm_mon + 1, loadHaf_time.tm_mday,
             loadHaf_time.tm_hour, loadHaf_time.tm_min, loadHaf_time.tm_sec);

    time_t range_t = *datetime;
    range_t -= (DAYSEC * 3);
    struct tm range_time = *localtime(&range_t);
    snprintf(rangeTime_str, sizeof(rangeTime_str), "%4d-%02d-%02d %02d:%02d:%02d",
             range_time.tm_year + 1900, range_time.tm_mon + 1, range_time.tm_mday,
             range_time.tm_hour, range_time.tm_min, range_time.tm_sec);

    for (int i = 0; i < NUM_CHIMNEY; i++)
    {
        CHIMNEY *c = &chimney[i];
        DATA_Q *q = &data_q[i];

        if (c->send_mode == 0) // 30min data transmit
        {
            // get 30min TFDH
            snprintf(query_str, sizeof(query_str), "SELECT CONCAT('TFDH', _data) FROM t_30tdah WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND chim_id = %d AND send = 0;", rangeTime_str, loadHafTime_str, i + 1);
            snprintf(update_str, sizeof(update_str), "UPDATE t_30tdah SET send = 1 WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND chim_id = %d AND send = 0;", rangeTime_str, loadHafTime_str, i + 1);

            if (enqueue_transmit_data(q, query_str, update_str) < 0)
                printf("=== No exists MISS TFDH(HAF) data to transmit ===\n");

            // get TNOH data
            /*snprintf(query_str, sizeof(query_str), "SELECT CONCAT(cmd, _data) FROM t_05tnoh WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND chim_id = %d AND send = 0;", rangeTime_str, loadHafTime_str, i + 1);
            snprintf(update_str, sizeof(update_str), "UPDATE t_05tnoh SET send = 1 WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND chim_id = %d AND send = 0;", rangeTime_str, loadHafTime_str, i + 1);

            if (enqueue_transmit_data(q, query_str, update_str) < 0)
                printf("=== No exists MISS TNOH data to transmit ===\n");*/

            // get TOFH data
            snprintf(query_str, sizeof(query_str), "SELECT CONCAT(cmd, _data) FROM t_30tofh WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND chim_id = %d AND send = 0;", rangeTime_str, loadHafTime_str, i + 1);
            snprintf(update_str, sizeof(update_str), "UPDATE t_30tofh SET send = 1 WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND chim_id = %d AND send = 0;", rangeTime_str, loadHafTime_str, i + 1);

            if (enqueue_transmit_data(q, query_str, update_str) < 0)
                printf("=== No exists MISS TOFH(HAF) data to transmit ===\n");

            // get TOFH-DAY data
            snprintf(query_str, sizeof(query_str), "SELECT CONCAT(cmd, _data) FROM t_30tofh_day WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND chim_id = %d AND send = 0;", rangeTime_str, loadHafTime_str, i + 1);
            snprintf(update_str, sizeof(update_str), "UPDATE t_30tofh_day SET send = 1 WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND chim_id = %d AND send = 0;", rangeTime_str, loadHafTime_str, i + 1);

            if (enqueue_transmit_data(q, query_str, update_str) < 0)
                printf("=== No exists MISS TOFH(HAF)-DAY data to transmit ===\n");

            // get TDDH data
            snprintf(query_str, sizeof(query_str), "SELECT CONCAT(cmd, _data) FROM t_30tddh WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND chim_id = %d AND send = 0;", rangeTime_str, loadHafTime_str, i + 1);
            snprintf(update_str, sizeof(update_str), "UPDATE t_30tddh SET send = 1 WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND chim_id = %d AND send = 0;", rangeTime_str, loadHafTime_str, i + 1);

            if (enqueue_transmit_data(q, query_str, update_str) < 0)
                printf("=== No exists MISS TDDH(HAF)-DAY data to transmit ===\n");
        }
        else if (c->send_mode == 1) // 5min, 30min data transmit
        {
            // get 5min TFDH
            snprintf(query_str, sizeof(query_str), "SELECT CONCAT('TFDH', _data) FROM t_05tdah WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND chim_id = %d AND send = 0;", rangeTime_str, loadTime_str, i + 1);
            snprintf(update_str, sizeof(update_str), "UPDATE t_05tdah SET send = 1 WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND chim_id = %d AND send = 0;", rangeTime_str, loadTime_str, i + 1);

            if (enqueue_transmit_data(q, query_str, update_str) < 0)
                printf("=== No exists MISS TFDH(FIV) data to transmit ===\n");

            // get TOFH data
            snprintf(query_str, sizeof(query_str), "SELECT CONCAT(cmd, _data) FROM t_05tofh WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND chim_id = %d AND send = 0;", rangeTime_str, loadTime_str, i + 1);
            snprintf(update_str, sizeof(update_str), "UPDATE t_05tofh SET send = 1 WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND chim_id = %d AND send = 0;", rangeTime_str, loadTime_str, i + 1);

            if (enqueue_transmit_data(q, query_str, update_str) < 0)
                printf("=== No exists MISS TOFH(FIV) data to transmit ===\n");

            // get TOFH-DAY data
            snprintf(query_str, sizeof(query_str), "SELECT CONCAT(cmd, _data) FROM t_05tofh_day WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND chim_id = %d AND send = 0;", rangeTime_str, loadTime_str, i + 1);
            snprintf(update_str, sizeof(update_str), "UPDATE t_05tofh_day SET send = 1 WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND chim_id = %d AND send = 0;", rangeTime_str, loadTime_str, i + 1);

            if (enqueue_transmit_data(q, query_str, update_str) < 0)
                printf("=== No exists MISS TOFH(FIV)-DAY data to transmit ===\n");

            // get TDDH data
            snprintf(query_str, sizeof(query_str), "SELECT CONCAT(cmd, _data) FROM t_05tddh WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND chim_id = %d AND send = 0;", rangeTime_str, loadTime_str, i + 1);
            snprintf(update_str, sizeof(update_str), "UPDATE t_05tddh SET send = 1 WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND chim_id = %d AND send = 0;", rangeTime_str, loadTime_str, i + 1);

            if (enqueue_transmit_data(q, query_str, update_str) < 0)
                printf("=== No exists MISS TDDH(FIV) data to transmit ===\n");

            // get 30min TFDH
            snprintf(query_str, sizeof(query_str), "SELECT CONCAT('TFDH', _data) FROM t_30tdah WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND chim_id = %d AND send = 0;", rangeTime_str, loadHafTime_str, i + 1);
            snprintf(update_str, sizeof(update_str), "UPDATE t_30tdah SET send = 1 WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND chim_id = %d AND send = 0;", rangeTime_str, loadHafTime_str, i + 1);

            if (enqueue_transmit_data(q, query_str, update_str) < 0)
                printf("=== No exists MISS TFDH(HAF) data to transmit ===\n");

            // get TNOH data
            /*snprintf(query_str, sizeof(query_str), "SELECT CONCAT(cmd, _data) FROM t_05tnoh WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND chim_id = %d AND send = 0;", rangeTime_str, loadHafTime_str, i + 1);
            snprintf(update_str, sizeof(update_str), "UPDATE t_05tnoh SET send = 1 WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND chim_id = %d AND send = 0;", rangeTime_str, loadHafTime_str, i + 1);

            if (enqueue_transmit_data(q, query_str, update_str) < 0)
                printf("=== No exists MISS TNOH data to transmit ===\n");*/

            // get TOFH data
            snprintf(query_str, sizeof(query_str), "SELECT CONCAT(cmd, _data) FROM t_30tofh WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND chim_id = %d AND send = 0;", rangeTime_str, loadHafTime_str, i + 1);
            snprintf(update_str, sizeof(update_str), "UPDATE t_30tofh SET send = 1 WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND chim_id = %d AND send = 0;", rangeTime_str, loadHafTime_str, i + 1);

            if (enqueue_transmit_data(q, query_str, update_str) < 0)
                printf("=== No exists MISS TOFH(HAF) data to transmit ===\n");

            // get TOFH-DAY data
            snprintf(query_str, sizeof(query_str), "SELECT CONCAT(cmd, _data) FROM t_30tofh_day WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND chim_id = %d AND send = 0;", rangeTime_str, loadHafTime_str, i + 1);
            snprintf(update_str, sizeof(update_str), "UPDATE t_30tofh_day SET send = 1 WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND chim_id = %d AND send = 0;", rangeTime_str, loadHafTime_str, i + 1);

            if (enqueue_transmit_data(q, query_str, update_str) < 0)
                printf("=== No exists MISS TOFH(HAF)-DAY data to transmit ===\n");

            // get TDDH data
            snprintf(query_str, sizeof(query_str), "SELECT CONCAT(cmd, _data) FROM t_30tddh WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND chim_id = %d AND send = 0;", rangeTime_str, loadHafTime_str, i + 1);
            snprintf(update_str, sizeof(update_str), "UPDATE t_30tddh SET send = 1 WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND chim_id = %d AND send = 0;", rangeTime_str, loadHafTime_str, i + 1);

            if (enqueue_transmit_data(q, query_str, update_str) < 0)
                printf("=== No exists MISS TDDH(HAF)-DAY data to transmit ===\n");
        }
        else // 5min data transmit
        {
            // get 5min TFDH
            snprintf(query_str, sizeof(query_str), "SELECT CONCAT('TFDH', _data) FROM t_05tdah WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND chim_id = %d AND send = 0;", rangeTime_str, loadTime_str, i + 1);
            snprintf(update_str, sizeof(update_str), "UPDATE t_05tdah SET send = 1 WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND chim_id = %d AND send = 0;", rangeTime_str, loadTime_str, i + 1);

            if (enqueue_transmit_data(q, query_str, update_str) < 0)
                printf("=== No exists MISS TFDH(FIV) data to transmit ===\n");

            // get TOFH data
            snprintf(query_str, sizeof(query_str), "SELECT CONCAT(cmd, _data) FROM t_05tofh WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND chim_id = %d AND send = 0;", rangeTime_str, loadTime_str, i + 1);
            snprintf(update_str, sizeof(update_str), "UPDATE t_05tofh SET send = 1 WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND chim_id = %d AND send = 0;", rangeTime_str, loadTime_str, i + 1);

            if (enqueue_transmit_data(q, query_str, update_str) < 0)
                printf("=== No exists MISS TOFH(FIV) data to transmit ===\n");

            // get TOFH-DAY data
            snprintf(query_str, sizeof(query_str), "SELECT CONCAT(cmd, _data) FROM t_05tofh_day WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND chim_id = %d AND send = 0;", rangeTime_str, loadTime_str, i + 1);
            snprintf(update_str, sizeof(update_str), "UPDATE t_05tofh_day SET send = 1 WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND chim_id = %d AND send = 0;", rangeTime_str, loadTime_str, i + 1);

            if (enqueue_transmit_data(q, query_str, update_str) < 0)
                printf("=== No exists MISS TOFH(FIV)-DAY data to transmit ===\n");

            // get TDDH data
            snprintf(query_str, sizeof(query_str), "SELECT CONCAT(cmd, _data) FROM t_05tddh WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND chim_id = %d AND send = 0;", rangeTime_str, loadTime_str, i + 1);
            snprintf(update_str, sizeof(update_str), "UPDATE t_05tddh SET send = 1 WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND chim_id = %d AND send = 0;", rangeTime_str, loadTime_str, i + 1);

            if (enqueue_transmit_data(q, query_str, update_str) < 0)
                printf("=== No exists MISS TDDH(FIV) data to transmit ===\n");
        }
    }
}

void enqueue_load_to_transmit(time_t begin_t, time_t end_t, int no_chimney, int seg)
{
    char query_str[300], beginTime_str[50], beginHafTime_str[50], endTime_str[50], endHafTime_str[50];

    DATA_Q *q = remote_q[no_chimney];

    time_t begin = begin_t;
    begin += FIVSEC;

    time_t beginHaf = begin_t;
    beginHaf += HAFSEC;

    time_t end = end_t;
    end += FIVSEC;

    time_t endHaf = end_t;
    endHaf += HAFSEC;

    struct tm begin_time = *localtime(&begin);
    struct tm beginHaf_time = *localtime(&beginHaf);
    struct tm end_time = *localtime(&end);
    struct tm endHaf_time = *localtime(&endHaf);

    snprintf(beginTime_str, sizeof(beginTime_str), "%4d-%02d-%02d %02d:%02d:%02d",
             begin_time.tm_year + 1900, begin_time.tm_mon + 1, begin_time.tm_mday,
             begin_time.tm_hour, begin_time.tm_min, begin_time.tm_sec);
    snprintf(beginHafTime_str, sizeof(beginHafTime_str), "%4d-%02d-%02d %02d:%02d:%02d",
             beginHaf_time.tm_year + 1900, beginHaf_time.tm_mon + 1, beginHaf_time.tm_mday,
             beginHaf_time.tm_hour, beginHaf_time.tm_min, beginHaf_time.tm_sec);
    snprintf(endTime_str, sizeof(endTime_str), "%4d-%02d-%02d %02d:%02d:%02d",
             end_time.tm_year + 1900, end_time.tm_mon + 1, end_time.tm_mday,
             end_time.tm_hour, end_time.tm_min, end_time.tm_sec);
    snprintf(endHafTime_str, sizeof(endHafTime_str), "%4d-%02d-%02d %02d:%02d:%02d",
             endHaf_time.tm_year + 1900, endHaf_time.tm_mon + 1, endHaf_time.tm_mday,
             endHaf_time.tm_hour, endHaf_time.tm_min, endHaf_time.tm_sec);

    printf("저장자료 요청 시간(FIV): %s\n", beginTime_str);
    printf("저장자료 요청 마감(FIV): %s\n", endTime_str);
    printf("저장자료 요청 시간(HAF): %s\n", beginHafTime_str);
    printf("저장자료 요청 마감(HAF): %s\n", endHafTime_str);

    if (seg == 0)
    {
        snprintf(query_str, sizeof(query_str), "SELECT CONCAT('TDUH', _data) FROM t_30tdah WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND chim_id = %d;", beginHafTime_str, endHafTime_str, no_chimney + 1);
        if (enqueue_transmit_data(q, query_str, ";") < 0)
            printf("=== There is no TDAH(HAF) data matching the request period ===\n");

        snprintf(query_str, sizeof(query_str), "SELECT CONCAT(cmd, _data) FROM t_05tnoh WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND chim_id = %d;", beginHafTime_str, endHafTime_str, no_chimney + 1);
        if (enqueue_transmit_data(q + 1, query_str, ";") < 0)
            printf("=== There is no TNOH(HAF) data matching the request period ===\n");

        snprintf(query_str, sizeof(query_str), "SELECT CONCAT(cmd, _data) FROM t_30tofh WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND chim_id = %d;", beginHafTime_str, endHafTime_str, no_chimney + 1);
        if (enqueue_transmit_data(q + 2, query_str, ";") < 0)
            printf("=== There is no TOFH(HAF) data matching the request period ===\n");

        snprintf(query_str, sizeof(query_str), "SELECT CONCAT(cmd, _data) FROM t_30tofh_day WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND chim_id = %d;", beginHafTime_str, endHafTime_str, no_chimney + 1);
        if (enqueue_transmit_data(q + 2, query_str, ";") < 0)
            printf("=== There is no TOFH(HAF)-DAY data matching the request period ===\n");

        snprintf(query_str, sizeof(query_str), "SELECT CONCAT(cmd, _data) FROM t_30tddh WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND chim_id = %d;", beginHafTime_str, endHafTime_str, no_chimney + 1);
        if (enqueue_transmit_data(q + 3, query_str, ";") < 0)
            printf("=== There is no TDDH(HAF) data matching the request period ===\n");
    }
    else if (seg == 1)
    {
        snprintf(query_str, sizeof(query_str), "SELECT CONCAT('TDUH', _data) FROM t_05tdah WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND chim_id = %d;", beginTime_str, endTime_str, no_chimney + 1);
        if (enqueue_transmit_data(q, query_str, ";") < 0)
            printf("=== There is no TDAH(FIV) data matching the request period ===\n");

        snprintf(query_str, sizeof(query_str), "SELECT CONCAT('TDUH', _data) FROM t_30tdah WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND chim_id = %d;", beginHafTime_str, endHafTime_str, no_chimney + 1);
        if (enqueue_transmit_data(q, query_str, ";") < 0)
            printf("=== There is no TDAH(HAF) data matching the request period ===\n");

        snprintf(query_str, sizeof(query_str), "SELECT CONCAT(cmd, _data) FROM t_05tnoh WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND chim_id = %d;", beginHafTime_str, endHafTime_str, no_chimney + 1);
        if (enqueue_transmit_data(q + 1, query_str, ";") < 0)
            printf("=== There is no TNOH(HAF) data matching the request period ===\n");

        snprintf(query_str, sizeof(query_str), "SELECT CONCAT(cmd, _data) FROM t_05tofh WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND chim_id = %d;", beginTime_str, endTime_str, no_chimney + 1);
        if (enqueue_transmit_data(q + 2, query_str, ";") < 0)
            printf("=== There is no TOFH(FIV) data matching the request period ===\n");

        snprintf(query_str, sizeof(query_str), "SELECT CONCAT(cmd, _data) FROM t_30tofh WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND chim_id = %d;", beginHafTime_str, endHafTime_str, no_chimney + 1);
        if (enqueue_transmit_data(q + 2, query_str, ";") < 0)
            printf("=== There is no TOFH(HAF) data matching the request period ===\n");

        snprintf(query_str, sizeof(query_str), "SELECT CONCAT(cmd, _data) FROM t_05tofh_day WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND chim_id = %d;", beginTime_str, endTime_str, no_chimney + 1);
        if (enqueue_transmit_data(q + 2, query_str, ";") < 0)
            printf("=== There is no TOFH-DAY(FIV) data matching the request period ===\n");

        snprintf(query_str, sizeof(query_str), "SELECT CONCAT(cmd, _data) FROM t_30tofh_day WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND chim_id = %d;", beginHafTime_str, endHafTime_str, no_chimney + 1);
        if (enqueue_transmit_data(q + 2, query_str, ";") < 0)
            printf("=== There is no TOFH-DAY(HAF) data matching the request period ===\n");

        snprintf(query_str, sizeof(query_str), "SELECT CONCAT(cmd, _data) FROM t_05tddh WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND chim_id = %d;", beginTime_str, endTime_str, no_chimney + 1);
        if (enqueue_transmit_data(q + 3, query_str, ";") < 0)
            printf("=== There is no TDDH(FIV) data matching the request period ===\n");

        snprintf(query_str, sizeof(query_str), "SELECT CONCAT(cmd, _data) FROM t_30tddh WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND chim_id = %d;", beginHafTime_str, endHafTime_str, no_chimney + 1);
        if (enqueue_transmit_data(q + 3, query_str, ";") < 0)
            printf("=== There is no TDDH(HAF) data matching the request period ===\n");
    }
    else
    {
        snprintf(query_str, sizeof(query_str), "SELECT CONCAT('TDUH', _data) FROM t_05tdah WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND chim_id = %d;", beginTime_str, endTime_str, no_chimney + 1);
        if (enqueue_transmit_data(q, query_str, ";") < 0)
            printf("=== There is no TDAH(FIV) data matching the request period ===\n");

        snprintf(query_str, sizeof(query_str), "SELECT CONCAT(cmd, _data) FROM t_05tofh WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND chim_id = %d;", beginTime_str, endTime_str, no_chimney + 1);
        if (enqueue_transmit_data(q + 2, query_str, ";") < 0)
            printf("=== There is no TOFH(FIV) data matching the request period ===\n");

        snprintf(query_str, sizeof(query_str), "SELECT CONCAT(cmd, _data) FROM t_05tofh_day WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND chim_id = %d;", beginTime_str, endTime_str, no_chimney + 1);
        if (enqueue_transmit_data(q + 2, query_str, ";") < 0)
            printf("=== There is no TOFH-DAY(FIV) data matching the request period ===\n");

        snprintf(query_str, sizeof(query_str), "SELECT CONCAT(cmd, _data) FROM t_05tddh WHERE insert_time >= \'%s\' AND insert_time <= \'%s\' AND chim_id = %d;", beginTime_str, endTime_str, no_chimney + 1);
        if (enqueue_transmit_data(q + 3, query_str, ";") < 0)
            printf("=== There is no TDDH(FIV) data matching the request period ===\n");
    }
}

int enqueue_transmit_data(DATA_Q *q, const char *query_str, const char *update_str)
{
    MYSQL *conn = get_conn();
    execute_query(conn, query_str);

    MYSQL_RES *res = mysql_store_result(conn);
    if (res == NULL)
    {
        printf("쿼리 결과 저장 실패\n");
        release_conn(conn);
        return -1;
    }

    int rows = mysql_num_rows(res);
    if (rows > 0)
    {
        char raw_data[BUFFER_SIZE];
        uint8_t data[BUFFER_SIZE];
        int data_len;

        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res)))
        {
            strcpy(raw_data, row[0]);
            printf("transmit data(ascii): %s\n", raw_data);

            data_len = convert_hex(raw_data, data);
            if (data_len < 0)
            {
                printf("Too short to convert data length.\n");
            }
            else
            {
                enqueue(q, data, data_len, update_str);
                printf("transmit data(hex) with crc: ");
                for (int i = 0; i < data_len; i++)
                {
                    printf("%02X", data[i]);
                }
                printf("\n");
            }

            usleep(10000); // 10msec 딜레이
        }

        mysql_free_result(res);
        release_conn(conn);

        return 1;
    }

    release_conn(conn);
    return -1;
}

int convert_hex(const char *asc, uint8_t *hex)
{
    size_t num_bytes = strlen(asc);
    if (num_bytes < 4)
        return -1;

    uint16_t crc = crc16_ccitt_false((uint8_t *)asc, num_bytes);

    for (int i = 0; i < (int)num_bytes; i++)
    {
        hex[i] = (uint8_t)asc[i];
    }

    hex[num_bytes] = (crc >> 8) & 0xff;
    hex[num_bytes + 1] = crc & 0xff;

    return (int)num_bytes + 2;
}