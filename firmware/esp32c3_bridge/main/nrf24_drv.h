#ifndef NRF24_DRV_H
#define NRF24_DRV_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void nrf24_drv_init(void);
int nrf24_drv_send(const uint8_t *data, size_t len);
int nrf24_drv_recv(uint8_t *data, size_t max_len, uint8_t *pipe_out);
bool nrf24_drv_poll(void);
void nrf24_drv_recover_rx(void);
uint8_t nrf24_drv_last_status(void);

#ifdef __cplusplus
}
#endif

#endif /* NRF24_DRV_H */
