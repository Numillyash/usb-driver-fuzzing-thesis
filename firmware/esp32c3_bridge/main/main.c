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
#ifndef RF_DEBUG
#define RF_DEBUG 2
#endif

static const char *TAG = "bridge_main";

#if RF_DEBUG >= 1
static void log_rxraw(const rf_test_packet_t *packet, uint8_t status)
{
    const uint8_t *bytes = (const uint8_t *)packet;

    ESP_LOGI(
        TAG,
        "RXRAW m0=0x%02x m1=0x%02x ver=%u type=%u seq=%u uptime_ms=%lu arg0=%lu flags=0x%04x status=0x%02x",
        (unsigned)bytes[0],
        (unsigned)bytes[1],
        (unsigned)packet->version,
        (unsigned)packet->msg_type,
        (unsigned)packet->seq,
        (unsigned long)packet->uptime_ms,
        (unsigned long)packet->arg0,
        (unsigned)packet->flags,
        (unsigned)status);
}
#endif

#if RF_DEBUG >= 2
static void log_rxhex(const uint8_t *buf, size_t len)
{
    char line[3 * RF_TEST_PAYLOAD_SIZE + 8];
    int offset = snprintf(line, sizeof(line), "RXHEX");
    size_t i;

    for (i = 0; i < len && offset > 0 && offset < (int)sizeof(line); ++i) {
        offset += snprintf(line + offset, sizeof(line) - (size_t)offset, " %02x", (unsigned)buf[i]);
    }

    ESP_LOGI(TAG, "%s", line);
}
#endif

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

#if RF_DEBUG >= 1
                log_rxraw(&packet, nrf24_drv_last_status());
#endif
#if RF_DEBUG >= 2
                log_rxhex(rx_buf, sizeof(rx_buf));
#endif

                if (packet.magic == RF_TEST_PACKET_MAGIC && packet.version == RF_TEST_PACKET_VERSION) {
                    if (packet.msg_type == RF_TEST_MSG_DATA) {
                        rf_test_packet_t ack_packet = {
                            .magic = RF_TEST_PACKET_MAGIC,
                            .version = RF_TEST_PACKET_VERSION,
                            .msg_type = RF_TEST_MSG_ACK,
                            .seq = packet.seq,
                            .uptime_ms = packet.uptime_ms,
                            .arg0 = packet.arg0,
                            .flags = RF_TEST_FLAG_NONE,
                        };
                        const int ack_ok = nrf24_drv_send((const uint8_t *)&ack_packet, sizeof(ack_packet));

                        rx_count++;
                        last_seq = packet.seq;

                        ESP_LOGI(
                            TAG,
                            "RFTEST seq=%u uptime_ms=%lu arg0=%lu flags=0x%04x",
                            (unsigned)packet.seq,
                            (unsigned long)packet.uptime_ms,
                            (unsigned long)packet.arg0,
                            (unsigned)packet.flags);
                        ESP_LOGI(
                            TAG,
                            "ACKTX seq=%u ok=%d status=0x%02x",
                            (unsigned)packet.seq,
                            ack_ok == (int)sizeof(ack_packet),
                            (unsigned)nrf24_drv_last_status());
                    }
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
