#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "bridge_uart.h"
#include "nrf24_drv.h"
#include "rf_test_packet.h"

#define BRIDGE_HEARTBEAT_INTERVAL_MS 5000

static const char *TAG = "bridge_main";

void app_main(void)
{
    TickType_t last_heartbeat_tick;
    uint32_t rx_count = 0;
    uint16_t last_seq = 0;
    uint8_t rx_buf[RF_TEST_PAYLOAD_SIZE];

    bridge_uart_init();
    nrf24_drv_init();
    last_heartbeat_tick = xTaskGetTickCount();

    bridge_uart_write_str("esp32c3_bridge: boot\r\n");
    bridge_uart_write_str("esp32c3_bridge: nrf24 rx bring-up mode\r\n");

    while (1) {
        const TickType_t now = xTaskGetTickCount();

        if ((now - last_heartbeat_tick) >= pdMS_TO_TICKS(BRIDGE_HEARTBEAT_INTERVAL_MS)) {
            ESP_LOGI(
                TAG,
                "HB bridge=esp32c3 rf=rx_ready rx_count=%u last_seq=%u status=0x%02x",
                (unsigned)rx_count,
                (unsigned)last_seq,
                (unsigned)nrf24_drv_last_status());
            last_heartbeat_tick = now;
        }

        if (nrf24_drv_poll()) {
            const int rx_len = nrf24_drv_recv(rx_buf, sizeof(rx_buf));

            if (rx_len == (int)sizeof(rf_test_packet_t)) {
                rf_test_packet_t packet;
                memcpy(&packet, rx_buf, sizeof(packet));

                if (packet.magic == RF_TEST_PACKET_MAGIC && packet.version == RF_TEST_PACKET_VERSION) {
                    rx_count++;
                    last_seq = packet.seq;

                    char line[128];
                    const int line_len = snprintf(
                        line,
                        sizeof(line),
                        "RFTEST seq=%u uptime_ms=%lu arg0=%lu flags=0x%04x\r\n",
                        (unsigned)packet.seq,
                        (unsigned long)packet.uptime_ms,
                        (unsigned long)packet.arg0,
                        (unsigned)packet.flags);

                    if (line_len > 0) {
                        bridge_uart_write(line, (size_t)line_len);
                    }
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
