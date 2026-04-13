#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

#include "nrf24_radio.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "rf_test_packet.h"

#define RF_TEST_ACK_TIMEOUT_MS 75u

int main(void) {
    uint16_t seq = 0;

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
        sleep_ms(1000);
    }
}
