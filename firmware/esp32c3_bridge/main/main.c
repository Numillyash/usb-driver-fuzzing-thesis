#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "crc8.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "bridge_uart.h"
#include "nrf24_drv.h"
#include "rf_frame_v2.h"
#include "rf_test_packet.h"

#ifndef PIPE0_PROBE_DIAG
#define PIPE0_PROBE_DIAG 1
#endif

#define BRIDGE_HEARTBEAT_INTERVAL_MS 5000
#define RFV2_PING_INTERVAL_MS 7000
#define RFV2_PING_TIMEOUT_MS 1500
#define RFV2_STATUS_INTERVAL_MS 10000
#define RFV2_STATUS_TIMEOUT_MS 1500
#define PIPE0_PROBE_INTERVAL_MS 3000
#define PIPE0_PROBE_ARG0 UINT32_C(0x50424F30)
#ifndef RF_DEBUG
#define RF_DEBUG 2
#endif

static const char *TAG = "bridge_main";

static uint16_t rfv2_next_seq(uint16_t seq)
{
    if (seq == RFV2_SEQ_RESERVED || seq == RFV2_SEQ_MAX) {
        return RFV2_SEQ_FIRST_VALID;
    }

    return (uint16_t)(seq + 1u);
}

static void build_rfv2_header(rfv2_frame_t *frame, uint8_t pkt_type, uint8_t src_id, uint16_t seq, uint8_t payload_len)
{
    memset(frame, 0, sizeof(*frame));
    frame->header.magic = RFV2_FRAME_MAGIC;
    frame->header.version = RFV2_FRAME_VERSION;
    frame->header.pkt_type = pkt_type;
    frame->header.src_id = src_id;
    frame->header.flags = RFV2_FLAG_NONE;
    frame->header.seq = seq;
    frame->header.payload_len = payload_len;
    frame->header.header_crc8 = crc8_compute(&frame->header, offsetof(rfv2_header_t, header_crc8));
}

static bool rfv2_header_crc_is_valid(const rfv2_frame_t *frame)
{
    return frame->header.header_crc8 ==
           crc8_compute(&frame->header, offsetof(rfv2_header_t, header_crc8));
}

static bool rfv2_frame_is_valid(const rfv2_frame_t *frame)
{
    return frame->header.magic == RFV2_FRAME_MAGIC &&
           frame->header.version == RFV2_FRAME_VERSION &&
           frame->header.seq != RFV2_SEQ_RESERVED &&
           frame->header.payload_len <= RFV2_PAYLOAD_SIZE &&
           rfv2_header_crc_is_valid(frame);
}

static void build_rfv2_ping(rfv2_frame_t *frame, uint16_t seq, uint32_t nonce)
{
    rfv2_ping_payload_t payload = {
        .nonce = nonce,
    };

    build_rfv2_header(frame, RFV2_PKT_PING, RFV2_SRC_ESP32C3, seq, (uint8_t)sizeof(payload));
    memcpy(frame->payload, &payload, sizeof(payload));
}

static void build_rfv2_get_status(rfv2_frame_t *frame, uint16_t seq)
{
    build_rfv2_header(frame, RFV2_PKT_GET_STATUS, RFV2_SRC_ESP32C3, seq, 0u);
}

static void log_hfv2(const rfv2_frame_t *frame)
{
    rfv2_heartbeat_payload_t heartbeat;
    unsigned long fault_flags = 0;
    unsigned usb_state = 0;
    unsigned scenario_state = 0;

    memset(&heartbeat, 0, sizeof(heartbeat));
    memcpy(&heartbeat, frame->payload, sizeof(heartbeat));
    usb_state = (unsigned)heartbeat.link_state;
    scenario_state = (unsigned)heartbeat.reserved0;

    ESP_LOGI(
        TAG,
        "HFV2 src=%u seq=%u mode=%u sys=%u usb=%u scn=%u uptime_ms=%lu fault=%lu",
        (unsigned)frame->header.src_id,
        (unsigned)frame->header.seq,
        (unsigned)heartbeat.mode,
        (unsigned)heartbeat.system_state,
        usb_state,
        scenario_state,
        (unsigned long)heartbeat.uptime_ms,
        fault_flags);
}

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
    TickType_t last_ping_tick;
    TickType_t last_status_tick;
#if PIPE0_PROBE_DIAG
    TickType_t last_pipe0_probe_tick;
    uint16_t pipe0_probe_seq = 1;
#endif
    uint32_t valid_rx = 0;
    uint32_t garbage_rx = 0;
    uint32_t ack_tx_ok = 0;
    uint32_t ack_tx_fail = 0;
    uint32_t ping_nonce = 1;
    uint16_t last_seq = 0;
    uint16_t request_seq = RFV2_SEQ_FIRST_VALID;
    uint16_t ping_wait_seq = RFV2_SEQ_RESERVED;
    uint16_t status_wait_seq = RFV2_SEQ_RESERVED;
    uint32_t ping_wait_nonce = 0;
    bool ping_in_flight = false;
    bool status_in_flight = false;
    TickType_t ping_deadline_tick = 0;
    TickType_t status_deadline_tick = 0;
    uint8_t rx_buf[RFV2_FRAME_SIZE];

    bridge_uart_init();
    nrf24_drv_init();
    last_heartbeat_tick = xTaskGetTickCount();
    last_ping_tick = last_heartbeat_tick;
    last_status_tick = last_heartbeat_tick;
#if PIPE0_PROBE_DIAG
    last_pipe0_probe_tick = last_heartbeat_tick;
#endif

    bridge_uart_write_str("esp32c3_bridge: boot\r\n");
    bridge_uart_write_str("esp32c3_bridge: nrf24 rx bring-up mode\r\n");

    while (1) {
        const TickType_t now = xTaskGetTickCount();

        if ((now - last_heartbeat_tick) >= pdMS_TO_TICKS(BRIDGE_HEARTBEAT_INTERVAL_MS)) {
            ESP_LOGI(
                TAG,
                "HB bridge=esp32c3 rf=rx_ready valid_rx=%u garbage_rx=%u ack_tx_ok=%u ack_tx_fail=%u last_seq=%u status=0x%02x",
                (unsigned)valid_rx,
                (unsigned)garbage_rx,
                (unsigned)ack_tx_ok,
                (unsigned)ack_tx_fail,
                (unsigned)last_seq,
                (unsigned)nrf24_drv_last_status());
            last_heartbeat_tick = now;
        }

        if (!ping_in_flight && (now - last_ping_tick) >= pdMS_TO_TICKS(RFV2_PING_INTERVAL_MS)) {
            rfv2_frame_t ping_frame;
            const uint16_t current_ping_seq = request_seq;
            const uint32_t current_ping_nonce = ping_nonce;
            int ping_ok;

            build_rfv2_ping(&ping_frame, current_ping_seq, current_ping_nonce);
            ping_ok = nrf24_drv_send_frame_v2((const uint8_t *)&ping_frame, sizeof(ping_frame));
            if (ping_ok == (int)sizeof(ping_frame)) {
                ESP_LOGI(
                    TAG,
                    "PINGTX seq=%u nonce=%lu",
                    (unsigned)current_ping_seq,
                    (unsigned long)current_ping_nonce);
                ping_in_flight = true;
                ping_wait_seq = current_ping_seq;
                ping_wait_nonce = current_ping_nonce;
                ping_deadline_tick = now + pdMS_TO_TICKS(RFV2_PING_TIMEOUT_MS);
                request_seq = rfv2_next_seq(request_seq);
                ping_nonce++;
            }
            last_ping_tick = now;
        }

        if (!status_in_flight && (now - last_status_tick) >= pdMS_TO_TICKS(RFV2_STATUS_INTERVAL_MS)) {
            rfv2_frame_t status_req_frame;
            const uint16_t current_status_seq = request_seq;
            int status_ok;

            build_rfv2_get_status(&status_req_frame, current_status_seq);
            status_ok = nrf24_drv_send_frame_v2((const uint8_t *)&status_req_frame, sizeof(status_req_frame));
            if (status_ok == (int)sizeof(status_req_frame)) {
                ESP_LOGI(TAG, "STATUSTX seq=%u", (unsigned)current_status_seq);
                status_in_flight = true;
                status_wait_seq = current_status_seq;
                status_deadline_tick = now + pdMS_TO_TICKS(RFV2_STATUS_TIMEOUT_MS);
                request_seq = rfv2_next_seq(request_seq);
            }
            last_status_tick = now;
        }

#if PIPE0_PROBE_DIAG
        if ((now - last_pipe0_probe_tick) >= pdMS_TO_TICKS(PIPE0_PROBE_INTERVAL_MS)) {
            rf_test_packet_t probe_packet = {
                .magic = RF_TEST_PACKET_MAGIC,
                .version = RF_TEST_PACKET_VERSION,
                .msg_type = RF_TEST_MSG_DATA,
                .seq = pipe0_probe_seq++,
                .uptime_ms = (uint32_t)pdTICKS_TO_MS(now),
                .arg0 = PIPE0_PROBE_ARG0,
                .flags = RF_TEST_FLAG_NONE,
            };
            const int probe_ok = nrf24_drv_send((const uint8_t *)&probe_packet, sizeof(probe_packet));

            ESP_LOGI(
                TAG,
                "PIPE0_PROBETX seq=%u ok=%d status=0x%02x",
                (unsigned)probe_packet.seq,
                probe_ok == (int)sizeof(probe_packet),
                (unsigned)nrf24_drv_last_status());
            last_pipe0_probe_tick = now;
        }
#endif

        if (nrf24_drv_poll()) {
            uint8_t rx_pipe = 0xffu;
            const int rx_len = nrf24_drv_recv(rx_buf, sizeof(rx_buf), &rx_pipe);

            if (rx_len == -2) {
                garbage_rx++;
                continue;
            }

            if (rx_pipe == 0u && rx_len == (int)sizeof(rf_test_packet_t)) {
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

                        valid_rx++;
                        last_seq = packet.seq;
                        if (ack_ok == (int)sizeof(ack_packet)) {
                            ack_tx_ok++;
                        } else {
                            ack_tx_fail++;
                        }

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
            } else if (rx_pipe == 1u && rx_len == (int)sizeof(rfv2_frame_t)) {
                rfv2_frame_t frame;

                memcpy(&frame, rx_buf, sizeof(frame));
                if (rfv2_frame_is_valid(&frame) &&
                    frame.header.pkt_type == RFV2_PKT_HEARTBEAT &&
                    frame.header.payload_len == sizeof(rfv2_heartbeat_payload_t)) {
                    log_hfv2(&frame);
                } else if (rfv2_frame_is_valid(&frame) &&
                           frame.header.pkt_type == RFV2_PKT_PONG &&
                           frame.header.payload_len == sizeof(rfv2_pong_payload_t)) {
                    rfv2_pong_payload_t pong_payload;

                    memcpy(&pong_payload, frame.payload, sizeof(pong_payload));
                    if (ping_in_flight &&
                        frame.header.seq == ping_wait_seq &&
                        pong_payload.nonce == ping_wait_nonce) {
                        ESP_LOGI(
                            TAG,
                            "PONGRX seq=%u nonce=%lu ok=1",
                            (unsigned)frame.header.seq,
                            (unsigned long)pong_payload.nonce);
                        ping_in_flight = false;
                        ping_wait_seq = RFV2_SEQ_RESERVED;
                        ping_wait_nonce = 0;
                    }
                } else if (rfv2_frame_is_valid(&frame) &&
                           frame.header.pkt_type == RFV2_PKT_STATUS &&
                           frame.header.payload_len == sizeof(rfv2_status_payload_t)) {
                    rfv2_status_payload_t status_payload;

                    memcpy(&status_payload, frame.payload, sizeof(status_payload));
                    if (status_in_flight && frame.header.seq == status_wait_seq) {
                        ESP_LOGI(
                            TAG,
                            "STATUSRX seq=%u mode=%u sys=%u usb=%u scn=%u uptime_ms=%lu fault=%lu",
                            (unsigned)frame.header.seq,
                            (unsigned)status_payload.mode,
                            (unsigned)status_payload.system_state,
                            (unsigned)status_payload.usb_state,
                            (unsigned)status_payload.scenario_state,
                            (unsigned long)status_payload.uptime_ms,
                            (unsigned long)status_payload.fault_flags);
                        status_in_flight = false;
                        status_wait_seq = RFV2_SEQ_RESERVED;
                    }
                }
            }
        }

        if (ping_in_flight && now >= ping_deadline_tick) {
            ESP_LOGI(
                TAG,
                "PING timeout seq=%u nonce=%lu",
                (unsigned)ping_wait_seq,
                (unsigned long)ping_wait_nonce);
            ping_in_flight = false;
            ping_wait_seq = RFV2_SEQ_RESERVED;
            ping_wait_nonce = 0;
        }

        if (status_in_flight && now >= status_deadline_tick) {
            status_in_flight = false;
            status_wait_seq = RFV2_SEQ_RESERVED;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
