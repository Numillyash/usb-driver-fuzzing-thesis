#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

#include "nrf24_radio.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "rf_test_packet.h"

int main(void) {
    uint16_t seq = 0;

    stdio_init_all();
    sleep_ms(2000);
    (void)nrf24_radio_init_tx();

    while (true) {
        rf_test_packet_t packet = {
            .magic = RF_TEST_PACKET_MAGIC,
            .version = RF_TEST_PACKET_VERSION,
            .msg_type = RF_TEST_MSG_HEARTBEAT,
            .seq = seq++,
            .uptime_ms = to_ms_since_boot(get_absolute_time()),
            .arg0 = 0,
            .flags = RF_TEST_FLAG_NONE,
        };
        const bool sent = nrf24_radio_send_fixed(&packet, sizeof(packet));

        printf("portable_demo: rf_test seq=%u sent=%u status=0x%02x\r\n",
               (unsigned)packet.seq,
               sent ? 1u : 0u,
               (unsigned)nrf24_radio_last_status());
        sleep_ms(1000);
    }
}
