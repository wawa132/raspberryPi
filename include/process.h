#ifndef __PROCESS_H__
#define __PROCESS_H__

#include "var.h"
#include "crc16.h"
#include "queue.h"
#include "database.h"

void *make_data();

void process_5sec(time_t *datetime);
void process_5min(time_t *datetime);
void process_30min(time_t *datetime);

void update_5sec_data();
void update_5sec_state();
void update_5sec_relation(time_t *datetime);

void update_5min_data();
void update_5min_state(time_t *datetime);
void update_5min_relation(time_t *datetime);

void update_tdah(time_t *datetime, int seg, int off);

#endif