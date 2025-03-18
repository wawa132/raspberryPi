#ifndef __SERVER_H__
#define __SERVER_H__

#include "var.h"
#include "queue.h"
#include "database.h"

extern int sockfd;

void *tcp_server();
void process_client_message(int connfd);

#endif