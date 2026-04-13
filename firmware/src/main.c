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

#define RF_TEST_ACK_TIMEOUT_MS 75u
#define RFV2_HEARTBEAT_INTERVAL_MS 5000u

static void build_rfv2_heartbeat(rfv2_frame_t *frame, uint16_t seq)
{
    rfv2_heartbeat_payload_t payload = {
        .uptime_ms = to_ms_since_boot(get_absolute_time()),
        .mode = 0u,
        .system_state = 0u,
        .link_state = 0u,
        .reserved0 = 0u,
    };

    memset(frame, 0, sizeof(*frame));
    frame->header.magic = RFV2_FRAME_MAGIC;
    frame->header.version = RFV2_FRAME_VERSION;
    frame->header.pkt_type = RFV2_PKT_HEARTBEAT;
    frame->header.src_id = RFV2_SRC_RP2040;
    frame->header.flags = RFV2_FLAG_NONE;
    frame->header.seq = seq;
    frame->header.payload_len = (uint8_t)sizeof(payload);
    memcpy(frame->payload, &payload, sizeof(payload));
    frame->header.header_crc8 = crc8_compute(&frame->header, offsetof(rfv2_header_t, header_crc8));
}

int main(void) {
    uint16_t seq = 0;
    uint16_t heartbeat_seq = 0;
    uint32_t last_heartbeat_ms = 0;

    stdio_init_all();
    sleep_ms(2000);
    (void)nrf24_radio_init_tx();

    while (true) {
        rf_test_packet_t packet = {
            .magic = RF_TEST_PACKET_MAGIC,
            .version = RF_TEST_PACKET_VERSION,
            .msg_type = RF_TEST_MSG_DATA,
            .seq = seq++,
            .uptime_ms = to_ms_since_boot(get_absolute_time()),
            .arg0 = 0,
            .flags = RF_TEST_FLAG_NONE,
        };
        const bool sent = nrf24_radio_send_fixed(&packet, sizeof(packet));
        rf_test_packet_t ack_packet;
        int ack_len = 0;

        printf("portable_demo: rf_test seq=%u sent=%u status=0x%02x\r\n",
               (unsigned)packet.seq,
               sent ? 1u : 0u,
               (unsigned)nrf24_radio_last_status());

        if (sent) {
            ack_len = nrf24_radio_recv_fixed(&ack_packet, sizeof(ack_packet), RF_TEST_ACK_TIMEOUT_MS);
        }

        if (ack_len == (int)sizeof(ack_packet) &&
            ack_packet.magic == RF_TEST_PACKET_MAGIC &&
            ack_packet.version == RF_TEST_PACKET_VERSION &&
            ack_packet.msg_type == RF_TEST_MSG_ACK &&
            ack_packet.seq == packet.seq) {
            printf("portable_demo: ack seq=%u status=0x%02x\r\n",
                   (unsigned)ack_packet.seq,
                   (unsigned)nrf24_radio_last_status());
        } else if (sent) {
            printf("portable_demo: ack timeout seq=%u status=0x%02x\r\n",
                   (unsigned)packet.seq,
                   (unsigned)nrf24_radio_last_status());
        }

        {
            const uint32_t now_ms = to_ms_since_boot(get_absolute_time());

            if ((now_ms - last_heartbeat_ms) >= RFV2_HEARTBEAT_INTERVAL_MS) {
                rfv2_frame_t heartbeat_frame;
                const bool heartbeat_sent;

                build_rfv2_heartbeat(&heartbeat_frame, heartbeat_seq++);
                heartbeat_sent = nrf24_radio_send_frame_v2(&heartbeat_frame, sizeof(heartbeat_frame));

                printf("portable_demo: hfv2 seq=%u sent=%u status=0x%02x\r\n",
                       (unsigned)heartbeat_frame.header.seq,
                       heartbeat_sent ? 1u : 0u,
                       (unsigned)nrf24_radio_last_status());
                last_heartbeat_ms = now_ms;
            }
        }
        sleep_ms(1000);
    }
}
