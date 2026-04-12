#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "bridge_uart.h"
#include "nrf24_drv.h"

void app_main(void)
{
    uint8_t rx_buf[64];

    bridge_uart_init();
    nrf24_drv_init();

    bridge_uart_write_str("esp32c3_bridge: boot\r\n");
    bridge_uart_write_str("esp32c3_bridge: bridge skeleton ready\r\n");
    bridge_uart_write_str("esp32c3_bridge: NRF24 transport is stubbed\r\n");

    while (1) {
        const int rx_len = bridge_uart_read(rx_buf, sizeof(rx_buf), 10);

        if (rx_len > 0) {
            bridge_uart_write_str("esp32c3_bridge: uart echo ");
            bridge_uart_write(rx_buf, (size_t)rx_len);
            bridge_uart_write_str("\r\n");
        }

        nrf24_drv_poll();
        bridge_uart_write_str("esp32c3_bridge: alive\r\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
