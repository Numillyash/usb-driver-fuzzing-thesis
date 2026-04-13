#ifndef CRC8_H
#define CRC8_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint8_t crc8_compute(const void *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* CRC8_H */
