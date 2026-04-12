#include "nrf24_drv.h"

#include "esp_log.h"

static const char *TAG = "nrf24_drv";

void nrf24_drv_init(void)
{
    /* TODO: configure SPI bus for the selected ESP32-C3 board pins. */
    /* TODO: configure CE and CSN GPIO assignments for the NRF24 module. */
    /* TODO: add power-up, register init, and presence-check sequence. */
    ESP_LOGI(TAG, "NRF24 driver stub initialized");
}

int nrf24_drv_send(const uint8_t *data, size_t len)
{
    (void)data;
    (void)len;

    /* TODO: push one payload through the real NRF24 TX path. */
    return 0;
}

int nrf24_drv_recv(uint8_t *data, size_t max_len)
{
    (void)data;
    (void)max_len;

    /* TODO: fetch one payload from the real NRF24 RX FIFO. */
    return 0;
}

bool nrf24_drv_poll(void)
{
    /* TODO: read IRQ/status registers and service pending RX/TX events. */
    return false;
}
