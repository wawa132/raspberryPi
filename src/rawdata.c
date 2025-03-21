#include "rawdata.h"

int32_t sensor_data[MAX_NUM];

void *sensor_thread()
{
    while (RUNNING)
    {
        for (int i = 0; i < MAX_NUM; i++)
        {
            int data = rand() % 10;

            if (data < 2)
                data = 0;

            sensor_data[i] = data;
        }

        sleep(1);
    }

    return NULL;
}