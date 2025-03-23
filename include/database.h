#ifndef __DATABASE_H__
#define __DATABASE_H__

#include "var.h"
#include "process.h"

#define DB_HOST "localhost"
#define DB_USER "root"
#define DB_PASS "12345678"
#define DB_NAME "myGW"

typedef struct
{
    MYSQL *conn;
    int usable; // 1: usable 0: unusable
    pthread_mutex_t lock;
    pthread_cond_t cond;
} MYDB;

void init_db();
void destroy_db();
MYSQL *get_conn();
void release_conn(MYSQL *conn);

void init_facility();
void execute_query(MYSQL *conn, const char *query_str);

#endif