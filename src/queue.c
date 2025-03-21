#include "queue.h"

DATA_Q data_q[10], remote_q[10][4];
PLANTER planter;

void init_queue()
{
    for (int i = 0; i < ARR_QUEUE; i++)
    {
        data_q[i].front = 0;
        data_q[i].rear = 0;
        data_q[i].cnt = 0;

        pthread_mutex_init(&data_q[i].lock, NULL);
        pthread_cond_init(&data_q[i].cond, NULL);

        for (int j = 0; j < 4; j++)
        {
            remote_q[i][j].front = 0;
            remote_q[i][j].rear = 0;
            remote_q[i][j].cnt = 0;

            pthread_mutex_init(&remote_q[i][j].lock, NULL);
            pthread_cond_init(&remote_q[i][j].cond, NULL);
        }
    }

    planter.front = 0;
    planter.rear = 0;
    planter.cnt = 0;

    pthread_mutex_init(&planter.lock, NULL);
    pthread_cond_init(&planter.cond, NULL);
}

void destroy_queue()
{
    for (int i = 0; i < ARR_QUEUE; i++)
    {
        pthread_mutex_destroy(&data_q[i].lock);
        pthread_cond_destroy(&data_q[i].cond);

        for (int j = 0; j < 4; j++)
        {
            pthread_mutex_destroy(&remote_q[i][j].lock);
            pthread_cond_destroy(&remote_q[i][j].cond);
        }
    }

    pthread_mutex_destroy(&planter.lock);
    pthread_cond_destroy(&planter.cond);
}

int isEmpty(DATA_Q *q)
{
    if (q->cnt == 0)
        return 1;
    else
        return 0;
}

int isProcessEmpty(PLANTER *q)
{
    if (q->cnt == 0)
        return 1;
    else
        return 0;
}

int isFull(DATA_Q *q)
{
    if (q->cnt == MAX_QUEUE)
        return 1;
    else
        return 0;
}

int isProcessFull(PLANTER *q)
{
    if (q->cnt == MAX_QUEUE)
        return 1;
    else
        return 0;
}

void enqueue(DATA_Q *q, unsigned char *message, size_t message_len, const char *query_str)
{
    pthread_mutex_lock(&q->lock);

    if (!isFull(q))
    {
        memcpy(q->message[q->rear], message, message_len);
        strcpy(q->quert_str[q->rear], query_str);
        q->message_len[q->rear] = message_len;

        q->rear = (q->rear + 1) % MAX_QUEUE;
        q->cnt++;
        pthread_cond_signal(&q->cond);
    }

    pthread_mutex_unlock(&q->lock);
}

void process_enqueue(PLANTER *q, time_t datetime)
{
    pthread_mutex_lock(&q->lock);

    if (!isProcessFull(q))
    {
        q->datetime[q->rear] = datetime;

        q->rear = (q->rear + 1) % MAX_QUEUE;
        q->cnt++;
        pthread_cond_signal(&q->cond);
    }

    pthread_mutex_unlock(&q->lock);
}

SEND_Q *dequeue(DATA_Q *q)
{
    pthread_mutex_lock(&q->lock);

    while (isEmpty(q))
    {
        pthread_cond_wait(&q->cond, &q->lock);
    }

    SEND_Q *send_q = (SEND_Q *)malloc(sizeof(SEND_Q));
    if (send_q == NULL)
    {
        printf("SEND_Q 메모리 할당 실패\n");
        return NULL;
    }

    memcpy(send_q->message, q->message[q->front], q->message_len[q->front]);
    strcpy(send_q->query_str, q->quert_str[q->front]);
    send_q->message_len = q->message_len[q->front];

    q->front = (q->front + 1) % MAX_QUEUE;
    q->cnt--;

    pthread_mutex_unlock(&q->lock);

    return send_q;
}

MINER *process_dequeue(PLANTER *q)
{
    pthread_mutex_lock(&q->lock);

    while (isProcessEmpty(q))
    {
        pthread_cond_wait(&q->cond, &q->lock);
    }

    MINER *miner = (MINER *)malloc(sizeof(MINER));
    if (miner == NULL)
    {
        printf("PROCESS 처리를 위한 시간 데이터 메모리 할당 실패\n");
        return NULL;
    }

    miner->datetime = q->datetime[q->front];

    q->front = (q->front + 1) % MAX_QUEUE;
    q->cnt--;

    pthread_mutex_unlock(&q->lock);

    return miner;
}
