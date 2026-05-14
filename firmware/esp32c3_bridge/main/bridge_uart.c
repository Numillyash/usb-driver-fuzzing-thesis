#include "bridge_uart.h"

#include <stddef.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "driver/usb_serial_jtag.h"
#include "esp_err.h"

#define BRIDGE_UART_RX_BUF_SIZE    1024
#define BRIDGE_UART_TX_BUF_SIZE    1024

void bridge_uart_init(void)
{
    const usb_serial_jtag_driver_config_t usb_jtag_cfg = {
        .tx_buffer_size = BRIDGE_UART_TX_BUF_SIZE,
        .rx_buffer_size = BRIDGE_UART_RX_BUF_SIZE,
    };

    /*
     * ESP32-C3 console/logs are exposed over the internal USB Serial/JTAG
     * interface (Windows COMx with VID:PID 303A:1001). Use the same backend
     * for command RX/TX so user commands and bridge responses share one port.
     */
    ESP_ERROR_CHECK(usb_serial_jtag_driver_install(&usb_jtag_cfg));
}

int bridge_uart_read(uint8_t *buf, size_t buf_len, uint32_t timeout_ms)
{
    if (buf == NULL || buf_len == 0) {
        return 0;
    }

    return usb_serial_jtag_read_bytes(buf, buf_len, pdMS_TO_TICKS(timeout_ms));
}

int bridge_uart_write(const void *data, size_t len)
{
    if (data == NULL || len == 0) {
        return 0;
    }

    return usb_serial_jtag_write_bytes(data, len, portMAX_DELAY);
}

int bridge_uart_write_str(const char *str)
{
    if (str == NULL) {
        return 0;
    }

    return bridge_uart_write(str, strlen(str));
}
