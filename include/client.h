#ifndef __CLIENT_H__
#define __CLIENT_H__

#include "var.h"
#include "crc16.h"
#include "queue.h"
#include "process.h"
#include "database.h"
#include "systime.h"

extern int clntfd;

void *tcp_client();
void send_process(DATA_Q *q, int no_chimney);
int send_data_to_server(SEND_Q *q, char *IP, uint16_t PORT);
void exit_client_socket();
int handle_response(uint8_t *data, ssize_t byte_num);

#endif