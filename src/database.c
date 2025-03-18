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
