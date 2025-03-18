#include "crc16.h"

#define CRC_POLY 0x1021
#define CRC_INIT 0xFFFF

uint16_t crc16_ccitt_false(const uint8_t *data, size_t length)
{
    uint16_t crc = CRC_INIT;
    size_t i;

    for (i = 0; i < length; ++i)
    {
        crc ^= (data[i] << 8);
        for (int j = 0; j < 8; ++j)
        {
            if (crc & 0x8000)
                crc = (crc << 1) ^ CRC_POLY;
            else
                crc <<= 1;
        }
    }

    return crc;
}