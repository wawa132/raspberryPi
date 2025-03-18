#ifndef __CLIENT_H__
#define __CLIENT_H__

#include "var.h"
#include "queue.h"
#include "database.h"

extern int clntfd;

void *tcp_client();
void isMessageToSend(int i);
int send_message_to_server(DATA_Q *q, char *IP, uint16_t PORT);

#endif