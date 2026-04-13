#include "crc8.h"

#include <stddef.h>
#include <stdint.h>

#define CRC8_INIT       0x00u
#define CRC8_POLYNOMIAL 0x07u

uint8_t crc8_compute(const void *data, size_t len)
{
    const uint8_t *bytes = (const uint8_t *)data;
    uint8_t crc = CRC8_INIT;
    size_t i;
    uint8_t bit;

    if (bytes == NULL) {
        return 0;
    }

    for (i = 0; i < len; ++i) {
        crc ^= bytes[i];
        for (bit = 0; bit < 8u; ++bit) {
            if ((crc & 0x80u) != 0u) {
                crc = (uint8_t)((crc << 1u) ^ CRC8_POLYNOMIAL);
            } else {
                crc <<= 1u;
            }
        }
    }

    return crc;
}
