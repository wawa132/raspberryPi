#ifndef __CLIENT_H__
#define __CLIENT_H__

#include "var.h"
#include "queue.h"
#include "database.h"

extern int clntfd;

void *tcp_client();
void isData_to_send(int i);
int send_data_to_server(SEND_Q *q, char *IP, uint16_t PORT);
int handle_response(uint8_t *data, ssize_t byte_num);

#endif