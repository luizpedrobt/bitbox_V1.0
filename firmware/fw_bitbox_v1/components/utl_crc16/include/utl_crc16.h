#pragma once

uint16_t utl_crc16_data(const uint8_t *data, size_t len, uint16_t acc);

#define utl_crc16(a,b) utl_crc16_data(a,b,0xFFFF);


