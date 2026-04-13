#include "bridge_uart.h"
#include <stddef.h>

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "driver/uart.h"
#include "esp_err.h"

#define BRIDGE_UART_PORT           UART_NUM_0
#define BRIDGE_UART_BAUD_RATE      115200
#define BRIDGE_UART_RX_BUF_SIZE    1024
#define BRIDGE_UART_TX_BUF_SIZE    1024

void bridge_uart_init(void)
{
    const uart_config_t uart_cfg = {
        .baud_rate = BRIDGE_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_driver_install(
        BRIDGE_UART_PORT,
        BRIDGE_UART_RX_BUF_SIZE,
        BRIDGE_UART_TX_BUF_SIZE,
        0,
        NULL,
        0));
    ESP_ERROR_CHECK(uart_param_config(BRIDGE_UART_PORT, &uart_cfg));

    /* Keep default pins until the board-specific bridge connector is fixed. */
    ESP_ERROR_CHECK(uart_set_pin(
        BRIDGE_UART_PORT,
        UART_PIN_NO_CHANGE,
        UART_PIN_NO_CHANGE,
        UART_PIN_NO_CHANGE,
        UART_PIN_NO_CHANGE));
}

int bridge_uart_read(uint8_t *buf, size_t buf_len, uint32_t timeout_ms)
{
    if (buf == NULL || buf_len == 0) {
        return 0;
    }

    return uart_read_bytes(
        BRIDGE_UART_PORT,
        buf,
        buf_len,
        pdMS_TO_TICKS(timeout_ms));
}

int bridge_uart_write(const void *data, size_t len)
{
    if (data == NULL || len == 0) {
        return 0;
    }

    return uart_write_bytes(BRIDGE_UART_PORT, data, len);
}

int bridge_uart_write_str(const char *str)
{
    if (str == NULL) {
        return 0;
    }

    return bridge_uart_write(str, strlen(str));
}
