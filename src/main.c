#include "main.h"
#define MAX_FILE_SIZE 50 * 1024 * 1024 // 50MBytes

bool RUNNING = true, sec_checker = true, min_checker = true;
struct tm current_time;
time_t now, before_now, before_5min;
pthread_t thread[MAX_THREAD];
pthread_mutex_t time_mtx;
TOFH_TIME off_time[2] = {0};

int main()
{
    // 강제종료 시그널 처리 함수 등록
    signal(SIGINT, exit_handler);

    printf("=== gateway start... v3.55 ===\n");
    pthread_mutex_init(&time_mtx, NULL);

    // 데이터베이스 초기화, 해시 생성
    init_db();
    init_hash();

    // 시설 설정
    init_facility();

    // 전원 단절 구간 확인
    if (check_off(off_time))
    {
        if (pthread_create(&thread[4], NULL, tofh_thread, (TOFH_TIME *)off_time) < 0)
        {
            printf("(전원단절 데이터 생성)스레드 생성 실패\n");
        }
    }

    // 스레드 생성
    thread_create();

    while (RUNNING)
    { // 메인 스레드
        time_process();
        usleep(100000); // 100msec delay
    }

    // 스레드 종료 대기 및 데이터베이스 해제
    thread_join();
    destroy_db();

    // EXIT
    pthread_mutex_destroy(&time_mtx);
    printf("=== gateway exit... ===\n");

    return 0;
}

void time_process()
{
    pthread_mutex_lock(&time_mtx);

    time_t t = time(NULL);
    current_time = *localtime(&t);
    now = mktime(&current_time);

    pthread_mutex_unlock(&time_mtx);

    if (now % 5 == 0 && now != before_now) // 5초 간격
    {
        if (sec_checker)
        {
            before_now = now;
            sec_checker = false;
        }

        if ((now - before_now) > 5 && (now - before_now) < 300) // 5초 데이터 1개 누락
        {
            time_t missing_time;
            for (missing_time = before_now + 5; missing_time < now; missing_time += 5)
            {
                write_time_log("missing 5sec time", missing_time);
                process_enqueue(&planter, missing_time);
            }
            write_time_log("5sec cplt restore", now);
        }

        process_enqueue(&planter, now);
        before_now = now; // 이전 시간 기록

        if (now % FIVSEC == 0 && now != before_5min)
        {
            if (min_checker)
            {
                before_5min = now;
                min_checker = false;
            }

            if ((now - before_5min) > FIVSEC)
            {
                time_t missing_time;
                for (missing_time = before_5min + FIVSEC; missing_time < now; missing_time += FIVSEC)
                {
                    write_time_log("missing 5min time", missing_time);
                    process_enqueue(&planter, missing_time);
                }
                write_time_log("5min cplt restore", now);
            }

            before_5min = now;
        }
    }
}

void exit_handler(int signum)
{
    printf("\n===Received interrupt exit SIGINT(%d)==\n", signum);

    // thread safe-exit
    RUNNING = false;

    // server-socket shutdown
    if (servfd != -1)
        shutdown(servfd, SHUT_RDWR);

    // client-socket shutdown
    if (clntfd != -1)
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

void write_time_log(const char *message, time_t process_time)
{
    char time_buff[50], content_buff[50];

    // file open
    FILE *file = fopen("/home/pi/userLog.txt", "r+");
    if (file == NULL)
    {
        // if not exists file, create
        file = fopen("/home/pi/userLog.txt", "w");
        if (file == NULL)
        {
            perror("can't open \"userLog.txt\"");
            return;
        }
    }
    else
    {
        // check file size
        fseek(file, 0, SEEK_END);
        long file_size = ftell(file);
        if (file_size >= MAX_FILE_SIZE)
        {
            // if file size above 50MB, reopen file for overwite mode
            freopen("/home/pi/userLog.txt", "w", file);
        }
    }

    // record time
    time_t current_t = time(NULL);
    struct tm *tm_info = localtime(&current_t);
    strftime(time_buff, sizeof(time_buff), "%Y-%m-%d %H:%M:%S", tm_info);

    // process time(param)
    struct tm *proc_time = localtime(&process_time);
    strftime(content_buff, sizeof(content_buff), "%Y-%m-%d %H:%M:%S", proc_time);

    // log record
    fprintf(file, "[%s]%s: %s (queue rear: %3d, front: %3d, cnt: %3d)\n", time_buff, message, content_buff, planter.rear, planter.front, planter.cnt);

    // file close
    fclose(file);
}