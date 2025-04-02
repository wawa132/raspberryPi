#ifndef __SYSTIME_H__
#define __SYSTIME_H__

#include "var.h"

// server request is command PSET -> time sync stop
void set_system_time(struct tm set_time, int server_req);

#endif