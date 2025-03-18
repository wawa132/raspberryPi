#ifndef __CRC16_H__
#define __CRC16_H__

#include "var.h"

uint16_t crc16_ccitt_false(const uint8_t *data, size_t length);

#endif /*__CRC16_H__*/