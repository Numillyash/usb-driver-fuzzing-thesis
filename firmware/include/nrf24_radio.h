#ifndef NRF24_RADIO_H
#define NRF24_RADIO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

bool nrf24_radio_init_tx(void);
bool nrf24_radio_send_fixed(const void *data, size_t len);
uint8_t nrf24_radio_last_status(void);

#ifdef __cplusplus
}
#endif

#endif /* NRF24_RADIO_H */
