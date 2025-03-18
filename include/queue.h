#ifndef __QUEUE_H__
#define __QUEUE_H__

#include "var.h"

#define MAX_QUEUE 300
#define ARR_QUEUE 10

typedef struct DATA_Q
{
    unsigned char message[MAX_QUEUE][BUFFER_SIZE];
    char quert_str[MAX_QUEUE][BUFFER_SIZE];
    size_t message_len[MAX_QUEUE];
    int front, rear, cnt;
    pthread_mutex_t lock;
    pthread_cond_t cond;

} DATA_Q;

typedef struct SEND_Q
{
    unsigned char message[BUFFER_SIZE];
    char query_str[BUFFER_SIZE];
    int message_len;

} SEND_Q;

extern DATA_Q data_q[10], remote_q[10][4];

void init_queue();
void destroy_queue();

int isEmpty(DATA_Q *q);
int isFull(DATA_Q *q);

void enqueue(DATA_Q *q, unsigned char *message, size_t message_len, const char *query_str);
SEND_Q *dequeue(DATA_Q *q);

#endif