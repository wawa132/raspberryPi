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