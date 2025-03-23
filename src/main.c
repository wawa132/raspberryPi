#include "main.h"

bool RUNNING = true, initial_operation = true;
struct tm current_time;
time_t now, before_now;
pthread_t thread[MAX_THREAD];

TOFH_TIME off_time[2] = {0};

int main()
{
    // 강제종료 시그널 처리 함수 등록
    signal(SIGINT, exit_handler);

    printf("gateway start... v3.55\n");

    // 데이터베이스 초기화, 해시 생성
    init_db();
    init_hash();

    // 시설 설정
    init_facility();

    // 전원 단절 구간 확인
    if (check_off(off_time))
    {
        if (pthread_create(&thread[4], NULL, offData_make_thread, (TOFH_TIME *)off_time) < 0)
        {
            printf("(전원단절 데이터 생성)스레드 생성 실패\n");
        }
    }

    // 스레드 생성
    thread_create();

    while (RUNNING)
    { // 메인 스레드
        time_process();
        sleep(1);
    }

    // 스레드 종료 대기 및 데이터베이스 해제
    thread_join();
    destroy_db();

    return 0;
}

void time_process()
{
    time_t t = time(NULL);
    current_time = *localtime(&t);
    now = mktime(&current_time);

    if (now % 5 == 0 && now != before_now) // 5초 간격
    {
        if (initial_operation)
        {
            before_now = now;
            initial_operation = false;
        }

        if ((now - before_now) > 5) // 5초 데이터 1개 누락
        {
            process_enqueue(&planter, before_now);
        }

        process_enqueue(&planter, now);
        before_now = now; // 이전 시간 기록
    }
}

void exit_handler(int signum)
{
    printf("\n===Received interrupt exit SIGINT(%d)==\n", signum);

    // thread safe-exit
    RUNNING = false;

    // server-socket shutdown
    if (sockfd)
        shutdown(sockfd, SHUT_RDWR);

    // client-socket shutdown
    if (clntfd)
        shutdown(clntfd, SHUT_RDWR);
}

void thread_create()
{
    if (pthread_create(&thread[0], NULL, sensor_thread, NULL) != 0)
    {
        fprintf(stderr, "Failed to create making data thread...\n");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&thread[1], NULL, tcp_server, NULL) != 0)
    {
        fprintf(stderr, "Failed to create tcp-server thread...\n");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&thread[2], NULL, tcp_client, NULL) != 0)
    {
        fprintf(stderr, "Failed to create tcp-client thread...\n");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&thread[3], NULL, make_data, NULL) != 0)
    {
        fprintf(stderr, "Failed to create make_data thread...\n");
        exit(EXIT_FAILURE);
    }
}

void thread_join()
{
    pthread_join(thread[0], NULL);
    pthread_join(thread[1], NULL);
    pthread_join(thread[2], NULL);
    pthread_join(thread[3], NULL);
}

int check_off(TOFH_TIME *t)
{
    char query_str[BUFFER_SIZE], time_buff[100];
    int year, mon, day, hour, min, sec, result;

    // 데이터베이스내 5분 데이터 마지막으로 저장 시간 확인
    snprintf(query_str, sizeof(query_str), "SELECT tim_date FROM t_05tdah ORDER BY tim_date DESC LIMIT 1;");

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
    snprintf(query_str, sizeof(query_str), "SELECT tim_date FROM t_30tdah ORDER BY tim_date DESC LIMIT 1;");
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