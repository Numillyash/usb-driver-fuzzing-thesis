#include <stdint.h>
#include <stdio.h>

#include "pico/stdlib.h"
#include "usb_case_config.h"

#define USB_CASE_HEARTBEAT_INTERVAL_MS 2000u

int main(void)
{
    uint32_t last_log_ms = 0u;

    stdio_init_all();
    sleep_ms(1500);

    while (true) {
        const uint32_t now_ms = to_ms_since_boot(get_absolute_time());

        if ((now_ms - last_log_ms) >= USB_CASE_HEARTBEAT_INTERVAL_MS) {
            printf("usb_case_demo: alive\r\n");
            printf("case_id=%lu\r\n", (unsigned long)USB_CASE_ID);
            printf("case_name=%s\r\n", USB_CASE_NAME);
            printf("case_group=%s\r\n", USB_CASE_GROUP);
            last_log_ms = now_ms;
        }

        sleep_ms(10);
    }
}
