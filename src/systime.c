#include "systime.h"

void set_system_time(struct tm set_time, int server_req)
{
    char bash_command[300];

    // set ntp sync
    if (server_req)
    {
        snprintf(bash_command, sizeof(bash_command), "sudo timedatectl set-ntp false");
        if (system(bash_command) != 0)
        {
            printf("set-ntp sync setting failed: %s\n", bash_command);
            return;
        }
    }

    // change system time
    snprintf(bash_command, sizeof(bash_command), "sudo date %02d%02d%02d%02d%04d.%02d",
             set_time.tm_mon + 1, set_time.tm_mday, set_time.tm_hour,
             set_time.tm_min, set_time.tm_year + 1900, set_time.tm_sec);

    if (system(bash_command) != 0)
        printf("set system time failed:%s\n", bash_command);
    else
        printf("=== successfully set system time ====\n");

    // check system time
    time_t current = time(NULL);
    printf("=== system time after set: %s ===\n", ctime(&current));

    // setup RTC time

    // restore ntp time sync, if not forced setup time(PSET)
    if (!server_req)
    {
        snprintf(bash_command, sizeof(bash_command), "sudo timedatectl set-ntp true");
        if (system(bash_command) != 0)
        {
            printf("set-ntp sync setting failed: %s\n", bash_command);
            return;
        }
    }
}