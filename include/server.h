#ifndef __SERVER_H__
#define __SERVER_H__

#include "var.h"
#include "seed.h"
#include "crc16.h"
#include "queue.h"
#include "client.h"
#include "database.h"
#include "systime.h"

extern int servfd;

void *tcp_server();
void process_client_message(int connfd);
int recv_handle_data(const unsigned char *recvData, ssize_t byte_num);
void exit_server_socket();

int pduh_process(const uint8_t *recvData, int no_chimney);
int pfst_process(const uint8_t *recvData, int no_chimney);
int psep_process(const uint8_t *recvData, int no_chimney);
int pupg_process(const uint8_t *recvData, int no_chimney);
int pver_process(const uint8_t *recvData, int no_chimney);
int pset_process(const uint8_t *recvData, int no_chimney);
int pfcc_process(const uint8_t *recvData, int no_chimney);
int past_process(const uint8_t *recvData, int no_chimney);
int pfcr_process(const uint8_t *recvData, int no_chimney);
int pfrs_process(const uint8_t *recvData, int no_chimney);
int prsi_process(const uint8_t *recvData, int no_chimney);
int pdat_process(const uint8_t *recvData, int no_chimney);
int podt_process(const uint8_t *recvData, int no_chimney);
int prbt_process(const uint8_t *recvData, int no_chimney);

int send_tcn2(int no_chimney, int type);
int send_tver(const uint8_t *recvData, int no_chimney);

int send_response(const uint8_t *data, int data_len);
void trim_space(char *buffer);
int Is_IpAddress(const char *address);
void check_recv_data(const uint8_t *data, size_t data_len);

#endif