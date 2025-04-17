#ifndef __VAR_H__
#define __VAR_H__

#include <signal.h> // ctrl+c

#include <time.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>

#include <netdb.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <netinet/in.h>

#include <openssl/evp.h>
#include <mariadb/mysql.h>

#define BUFFER_SIZE 4095
#define CHIMNEY_NUM 10
#define MAX_NUM 100
#define MAX_THREAD 10

#define FIVSEC 300
#define HAFSEC 1800
#define DAYSEC 86400

#define FIVNUM 60
#define HAFNUM 6

#define FIVTDDH 288
#define HAFTDDH 48

#define TIMEOUT 30

typedef struct
{
    int32_t min, max, std;
} M_RANGE;

typedef struct
{
    uint8_t bsec_run, bfiv_run, lock;
    uint16_t ond, ofd;
} DELAY_TIME;

typedef struct
{
    int32_t value;
    uint8_t stat, run, rel;
} DATA;

typedef struct
{
    uint16_t nrm, low, com, off, chk, total;
} STATE;

typedef struct
{
    char name[6], item[2];
    M_RANGE range;
    DELAY_TIME delay;
    uint8_t check, type, port, rel_facility, TNOH;
    DATA SEC, FIV, HAF;
    STATE SFIV, SHAF;

} FACILITY;

typedef struct
{
    char assign_ip[16];
    uint8_t result;

} UPGRADE;

typedef struct
{
    char sv_ip[16], gw_ip[16], model[21], manufacture_code[3],
        version[21], passwd[11], hash[100], ntp_ip[16];
    uint16_t sv_port, remote_port, ntp_port;
    UPGRADE upgrade;

} INFORM;

typedef struct
{
    char business[8], chimney[4];
    uint8_t num_facility, send_mode, reboot, TNOH;
    uint16_t time_resend;
    FACILITY facility[MAX_NUM];
    DELAY_TIME delay;
    UPGRADE upgrade;
    INFORM info;

} CHIMNEY;

typedef struct
{
    struct tm off_fiv;
    struct tm off_haf;
    struct tm off_now;

} TOFH_TIME;

extern CHIMNEY chimney[CHIMNEY_NUM];
extern INFORM info[CHIMNEY_NUM];
extern TOFH_TIME off_time[2]; // [0] 부팅 후 전원단절 구간, [1] 서버 시간 동기화 오차로 인한 전원단절 구간

extern bool RUNNING, sec_checker, min_checker;
extern uint8_t NUM_CHIMNEY, processing, time_change;
extern uint8_t request_data;
extern uint16_t SYNC_TIME;
extern int32_t sensor_data[MAX_NUM];
extern pthread_t thread[MAX_THREAD];
extern pthread_mutex_t time_mtx;
extern pthread_cond_t time_cond;

extern time_t now, before_now, produce_time;

void convert_ascii_to_hex(const char *src, uint8_t *dst, size_t start, size_t length);
void check_sent_data(const uint8_t *data, size_t data_len);

#endif