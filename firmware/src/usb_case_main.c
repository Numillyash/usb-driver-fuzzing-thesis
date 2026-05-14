#include <stdint.h>
#include <stdio.h>

#include "pico/stdlib.h"
#include "usb_case_config.h"
#include "usb_case_descriptor_layer.h"

#define USB_CASE_HEARTBEAT_INTERVAL_MS 2000u

int main(void)
{
    uint32_t last_log_ms = 0u;
    bool profile_printed = false;
    usb_case_descriptor_profile_t descriptor_profile;

    stdio_init_all();
    sleep_ms(1500);
    descriptor_profile = usb_case_get_descriptor_profile();

    while (true) {
        const uint32_t now_ms = to_ms_since_boot(get_absolute_time());

        if (!profile_printed) {
            /*
             * Current implementation status:
             * - persona selection by USB_CASE_ID is active;
             * - actual USB descriptor switching is not wired yet;
             * - active USB transport remains safe CDC stdio.
             */
            printf("descriptor_persona_id=%lu\r\n", (unsigned long)descriptor_profile.persona_id);
            printf("descriptor_persona_name=%s\r\n", descriptor_profile.persona_name);
            printf("descriptor_switched=%u\r\n", descriptor_profile.descriptors_switched ? 1u : 0u);
            printf("descriptor_active_transport=%s\r\n", descriptor_profile.active_transport);
            profile_printed = true;
        }

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
