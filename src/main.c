#include "main.h"

bool RUNNING = true;
struct tm current_time;
pthread_t thread[MAX_THREAD];

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
    time_t now = mktime(&current_time);

    if (now % 5 == 0) // 5초 간격
    {
        process_5sec(); // 5초 데이터 생성

        if (now % FIVSEC == 0)
        {
            process_5min(); // 5분 데이터 생성

            if (now % HAFSEC == 0)
            {
                process_30min(); // 30분 데이터 생성
            }
        }
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
    if (pthread_create(&thread[0], NULL, make_data, NULL) != 0)
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
}

void thread_join()
{
    pthread_join(thread[0], NULL);
    pthread_join(thread[1], NULL);
    pthread_join(thread[2], NULL);
}