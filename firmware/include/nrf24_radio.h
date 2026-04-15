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
bool nrf24_radio_send_frame_v2(const void *data, size_t len);
int nrf24_radio_recv_any(void *data, size_t max_len, uint8_t *pipe_out, size_t *payload_len_out, uint32_t timeout_ms);
int nrf24_radio_recv_fixed(void *data, size_t len, uint32_t timeout_ms);
int nrf24_radio_recv_frame_v2(void *data, size_t len, uint32_t timeout_ms);
uint8_t nrf24_radio_last_status(void);

#ifdef __cplusplus
}
#endif

#endif /* NRF24_RADIO_H */
