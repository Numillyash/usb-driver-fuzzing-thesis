#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "crc8.h"
#include "nrf24_radio.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "rf_frame_v2.h"
#include "rf_test_packet.h"
#include "status.h"

#ifndef PIPE0_PROBE_DIAG
#define PIPE0_PROBE_DIAG 1
#endif

#define RF_TEST_INTERVAL_MS 1000u
#define RF_TEST_ACK_TIMEOUT_MS 75u
#define RFV2_HEARTBEAT_INTERVAL_MS 5000u
#define RFV2_PING_POLL_MS 10u
#define PIPE0_PROBE_ARG0 UINT32_C(0x50424F30)

static uint16_t rfv2_next_seq(uint16_t seq)
{
    if (seq == RFV2_SEQ_RESERVED || seq == RFV2_SEQ_MAX) {
        return RFV2_SEQ_FIRST_VALID;
    }

    return (uint16_t)(seq + 1u);
}

static void build_rfv2_header(rfv2_frame_t *frame, uint8_t pkt_type, uint16_t seq, uint8_t payload_len)
{
    memset(frame, 0, sizeof(*frame));
    frame->header.magic = RFV2_FRAME_MAGIC;
    frame->header.version = RFV2_FRAME_VERSION;
    frame->header.pkt_type = pkt_type;
    frame->header.src_id = RFV2_SRC_RP2040;
    frame->header.flags = RFV2_FLAG_NONE;
    frame->header.seq = seq;
    frame->header.payload_len = payload_len;
    frame->header.header_crc8 = crc8_compute(&frame->header, offsetof(rfv2_header_t, header_crc8));
}

static bool rfv2_header_is_valid(const rfv2_frame_t *frame)
{
    return frame->header.magic == RFV2_FRAME_MAGIC &&
           frame->header.version == RFV2_FRAME_VERSION &&
           frame->header.seq != RFV2_SEQ_RESERVED &&
           frame->header.payload_len <= RFV2_PAYLOAD_SIZE &&
           frame->header.header_crc8 ==
               crc8_compute(&frame->header, offsetof(rfv2_header_t, header_crc8));
}

static void build_rfv2_heartbeat(rfv2_frame_t *frame, uint16_t seq)
{
    rfv2_heartbeat_payload_t payload = {
        .uptime_ms = to_ms_since_boot(get_absolute_time()),
        .mode = 0u,
        .system_state = 0u,
        .link_state = RFV2_LINK_OK,
        .reserved0 = 0u,
    };

    build_rfv2_header(frame, RFV2_PKT_HEARTBEAT, seq, (uint8_t)sizeof(payload));
    memcpy(frame->payload, &payload, sizeof(payload));
}

static void build_rfv2_pong(rfv2_frame_t *frame, uint16_t seq, uint32_t nonce)
{
    rfv2_pong_payload_t payload = {
        .nonce = nonce,
        .responder_uptime_ms = to_ms_since_boot(get_absolute_time()),
    };

    build_rfv2_header(frame, RFV2_PKT_PONG, seq, (uint8_t)sizeof(payload));
    memcpy(frame->payload, &payload, sizeof(payload));
}

static void build_rfv2_status(rfv2_frame_t *frame, uint16_t seq, uint16_t active_seq)
{
    rfv2_status_payload_t payload = {
        .uptime_ms = to_ms_since_boot(get_absolute_time()),
        .active_seq = active_seq,
        .mode = CP_DEVICE_MODE_SAFE,
        .system_state = STATUS_SYSTEM_SAFE_IDLE,
        .usb_state = STATUS_USB_ACTIVE,
        .scenario_state = SCENARIO_STATE_IDLE,
        .fault_flags = 0u,
    };

    build_rfv2_header(frame, RFV2_PKT_STATUS, seq, (uint8_t)sizeof(payload));
    memcpy(frame->payload, &payload, sizeof(payload));
}

int main(void) {
    uint16_t seq = 0;
    uint16_t heartbeat_seq = RFV2_SEQ_FIRST_VALID;
    uint32_t last_rf_test_ms = 0;
    uint32_t last_heartbeat_ms = 0;
    bool rf_test_ack_pending = false;
    uint16_t rf_test_expected_ack_seq = 0;
    uint32_t rf_test_ack_deadline_ms = 0;

    stdio_init_all();
    sleep_ms(2000);
    (void)nrf24_radio_init_tx();
    last_rf_test_ms = to_ms_since_boot(get_absolute_time()) - RF_TEST_INTERVAL_MS;
    last_heartbeat_ms = to_ms_since_boot(get_absolute_time()) - RFV2_HEARTBEAT_INTERVAL_MS;

    while (true) {
        const uint32_t now_ms = to_ms_since_boot(get_absolute_time());

        if ((now_ms - last_rf_test_ms) >= RF_TEST_INTERVAL_MS) {
            rf_test_packet_t packet = {
                .magic = RF_TEST_PACKET_MAGIC,
                .version = RF_TEST_PACKET_VERSION,
                .msg_type = RF_TEST_MSG_DATA,
                .seq = seq++,
                .uptime_ms = now_ms,
                .arg0 = 0,
                .flags = RF_TEST_FLAG_NONE,
            };
            const bool sent = nrf24_radio_send_fixed(&packet, sizeof(packet));

            printf("portable_demo: rf_test seq=%u sent=%u status=0x%02x\r\n",
                   (unsigned)packet.seq,
                   sent ? 1u : 0u,
                   (unsigned)nrf24_radio_last_status());

            if (sent) {
                rf_test_ack_pending = true;
                rf_test_expected_ack_seq = packet.seq;
                rf_test_ack_deadline_ms = now_ms + RF_TEST_ACK_TIMEOUT_MS;
            }

            last_rf_test_ms = now_ms;
        }

        if ((now_ms - last_heartbeat_ms) >= RFV2_HEARTBEAT_INTERVAL_MS) {
                rfv2_frame_t heartbeat_frame;
                bool heartbeat_sent;

                build_rfv2_heartbeat(&heartbeat_frame, heartbeat_seq);
                heartbeat_sent = nrf24_radio_send_frame_v2(&heartbeat_frame, sizeof(heartbeat_frame));

                printf("portable_demo: hfv2 seq=%u sent=%u status=0x%02x\r\n",
                       (unsigned)heartbeat_frame.header.seq,
                       heartbeat_sent ? 1u : 0u,
                       (unsigned)nrf24_radio_last_status());
                heartbeat_seq = rfv2_next_seq(heartbeat_seq);
                last_heartbeat_ms = now_ms;
        }

        {
            uint8_t rx_pipe = 0xffu;
            size_t rx_payload_len = 0u;
            uint8_t rx_buf[RFV2_FRAME_SIZE];
            const int rx_len = nrf24_radio_recv_any(rx_buf, sizeof(rx_buf), &rx_pipe, &rx_payload_len, RFV2_PING_POLL_MS);

            if (rx_len > 0 && rx_pipe == 0u && rx_payload_len == sizeof(rf_test_packet_t)) {
                rf_test_packet_t packet;

                memcpy(&packet, rx_buf, sizeof(packet));

                if (packet.magic == RF_TEST_PACKET_MAGIC &&
                    packet.version == RF_TEST_PACKET_VERSION &&
                    packet.msg_type == RF_TEST_MSG_ACK &&
                    rf_test_ack_pending &&
                    packet.seq == rf_test_expected_ack_seq) {
                    printf("portable_demo: ack seq=%u status=0x%02x\r\n",
                           (unsigned)packet.seq,
                           (unsigned)nrf24_radio_last_status());
                    rf_test_ack_pending = false;
                } else if (packet.magic == RF_TEST_PACKET_MAGIC &&
                           packet.version == RF_TEST_PACKET_VERSION &&
                           packet.msg_type == RF_TEST_MSG_DATA &&
                           packet.arg0 == PIPE0_PROBE_ARG0) {
                    printf("portable_demo: PIPE0_PROBERX seq=%u\r\n",
                           (unsigned)packet.seq);
                }
            } else if (rx_len == (int)sizeof(rfv2_frame_t) &&
                       rx_pipe == 1u) {
                rfv2_frame_t rx_frame;
                memcpy(&rx_frame, rx_buf, sizeof(rx_frame));

                if (rfv2_header_is_valid(&rx_frame) &&
                    rx_frame.header.pkt_type == RFV2_PKT_PING &&
                    rx_frame.header.payload_len == sizeof(rfv2_ping_payload_t)) {
                rfv2_ping_payload_t ping_payload;
                rfv2_frame_t pong_frame;
                rfv2_pong_payload_t pong_payload;

                memcpy(&ping_payload, rx_frame.payload, sizeof(ping_payload));
                printf("portable_demo: rfv2 ping seq=%u nonce=%lu\r\n",
                       (unsigned)rx_frame.header.seq,
                       (unsigned long)ping_payload.nonce);

                build_rfv2_pong(&pong_frame, rx_frame.header.seq, ping_payload.nonce);
                (void)nrf24_radio_send_frame_v2(&pong_frame, sizeof(pong_frame));
                memcpy(&pong_payload, pong_frame.payload, sizeof(pong_payload));

                printf("portable_demo: rfv2 pong seq=%u nonce=%lu\r\n",
                       (unsigned)pong_frame.header.seq,
                       (unsigned long)pong_payload.nonce);
                } else if (rfv2_header_is_valid(&rx_frame) &&
                           rx_frame.header.pkt_type == RFV2_PKT_GET_STATUS &&
                           rx_frame.header.payload_len == 0u) {
                rfv2_frame_t status_frame;
                rfv2_status_payload_t status_payload;

                printf("portable_demo: rfv2 get_status seq=%u\r\n",
                       (unsigned)rx_frame.header.seq);

                build_rfv2_status(&status_frame, rx_frame.header.seq, (uint16_t)(seq == 0 ? 0u : (seq - 1u)));
                (void)nrf24_radio_send_frame_v2(&status_frame, sizeof(status_frame));
                memcpy(&status_payload, status_frame.payload, sizeof(status_payload));

                printf("portable_demo: rfv2 status seq=%u mode=%u sys=%u usb=%u scn=%u\r\n",
                       (unsigned)status_frame.header.seq,
                       (unsigned)status_payload.mode,
                       (unsigned)status_payload.system_state,
                       (unsigned)status_payload.usb_state,
                       (unsigned)status_payload.scenario_state);
                }
            }
        }

        if (rf_test_ack_pending && (int32_t)(now_ms - rf_test_ack_deadline_ms) >= 0) {
            printf("portable_demo: ack timeout seq=%u status=0x%02x\r\n",
                   (unsigned)rf_test_expected_ack_seq,
                   (unsigned)nrf24_radio_last_status());
            rf_test_ack_pending = false;
        }

        sleep_ms(5);
    }
}
