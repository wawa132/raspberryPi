#ifndef __MAIN_H__
#define __MAIN_H__

#include "var.h"
#include "hash.h"
#include "rawdata.h"
#include "process.h"
#include "database.h"
#include "server.h"
#include "client.h"

void exit_handler(int signum);
void time_process();
void thread_create();
void thread_join();

#endif