#ifndef __PROCESS_H__
#define __PROCESS_H__

#include "var.h"
#include "crc16.h"
#include "database.h"

void process_5sec();
void process_5min();
void process_30min();

void update_5sec_data();
void update_5sec_state();
void update_5sec_relation();

void update_5min_data();
void update_5min_state();
void update_5min_relation();

void update_tdah(int seg, int off);

#endif