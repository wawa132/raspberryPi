#include "var.h"

void convert_ascii_to_hex(const char *src, uint8_t *dst, size_t start, size_t length)
{
    uint8_t convert_buffer[BUFFER_SIZE];

    for (size_t i = 0; i < length; i++)
    {
        convert_buffer[i] = (uint8_t)src[i];
    }

    memcpy(dst + start, convert_buffer, length);
}

void check_sent_data(const uint8_t *data, size_t data_len)
{
    char command[5];

    printf("data sent by hex(%ld): ", data_len);
    for (size_t i = 0; i < data_len - 2; i++)
    {
        printf("%02x", data[i]);
    }
    printf("\n");

    memcpy(command, data, 4);
    command[4] = '\0';

    printf("data sent by ascii(%ld): ", data_len);

    if (strncmp(command, "TCN2", 4) == 0)
    {
        for (size_t i = 0; i < data_len - 2; i++)
        {
            if (i < 18)
                printf("%c", data[i]);
            else if (i < 50)
                printf("%02x", data[i]);
            else if (i < 92)
                printf("%c", data[i]);
            else if (i < 140)
                printf("%02x", data[i]);
            else
                printf("%c", data[i]);
        }
    }
    else if (strncmp(command, "TVER", 4) == 0 || strncmp(command, "TUPG", 4) == 0)
    {
        for (size_t i = 0; i < data_len - 2; i++)
        {
            if (i < 18)
                printf("%c", data[i]);
            else if (i < 50)
                printf("%02x", data[i]);
            else if (i < 92)
                printf("%c", data[i]);
            else
                printf("%02x", data[i]);
        }
    }
    else
    {
        for (size_t i = 0; i < data_len - 2; i++)
        {
            printf("%c", data[i]);
        }
    }

    printf("\n");
}
