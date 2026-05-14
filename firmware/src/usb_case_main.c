#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "pico/bootrom.h"
#include "pico/stdlib.h"
#include "rf_test_packet.h"
#include "nrf24_radio.h"
#include "usb_case_config.h"
#include "usb_case_descriptor_layer.h"

#define USB_CASE_HEARTBEAT_INTERVAL_MS 2000u
#define USB_CASE_CMD_BUF_SIZE 64u
#define USB_CASE_RF_POLL_TIMEOUT_MS 1u
#define USB_CASE_RF_BOOTLOADER_ARG0 UINT32_C(0x424F4F54)

static void usb_case_print_help(void)
{
    printf("available commands: help info ping bootloader\r\n");
}

static void usb_case_print_info(const usb_case_descriptor_profile_t *descriptor_profile)
{
    printf("case_id=%lu\r\n", (unsigned long)USB_CASE_ID);
    printf("case_name=%s\r\n", USB_CASE_NAME);
    printf("case_group=%s\r\n", USB_CASE_GROUP);
    printf("case_base_persona=%s\r\n", USB_CASE_BASE_PERSONA);
    printf("mutation_summary=%s\r\n", USB_CASE_MUTATION_SUMMARY);
    printf("descriptor_persona_name=%s\r\n", descriptor_profile->persona_name);
    printf("descriptor_switched=%u\r\n", descriptor_profile->descriptors_switched ? 1u : 0u);
    printf("descriptor_active_transport=%s\r\n", descriptor_profile->active_transport);
}

static void usb_case_handle_command(const char *cmd, const usb_case_descriptor_profile_t *descriptor_profile)
{
    if (strcmp(cmd, "help") == 0) {
        usb_case_print_help();
        return;
    }

    if (strcmp(cmd, "info") == 0) {
        usb_case_print_info(descriptor_profile);
        return;
    }

    if (strcmp(cmd, "ping") == 0) {
        printf("pong\r\n");
        return;
    }

    if (strcmp(cmd, "bootloader") == 0) {
        printf("usb_case_demo: entering USB bootloader\r\n");
        sleep_ms(20);
        reset_usb_boot(0, 0);
        return;
    }

    if (cmd[0] != '\0') {
        printf("unknown command: %s\r\n", cmd);
        usb_case_print_help();
    }
}

int main(void)
{
    uint32_t last_log_ms = 0u;
    bool profile_printed = false;
    char cmd_buf[USB_CASE_CMD_BUF_SIZE];
    size_t cmd_len = 0u;
    usb_case_descriptor_profile_t descriptor_profile;
    bool rf_ready = false;

    stdio_init_all();
    sleep_ms(1500);
    descriptor_profile = usb_case_get_descriptor_profile();
    rf_ready = nrf24_radio_init_tx();
    printf("usb_case_demo: rf_init=%u\r\n", rf_ready ? 1u : 0u);

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

        for (;;) {
            int ch = getchar_timeout_us(0);

            if (ch == PICO_ERROR_TIMEOUT) {
                break;
            }

            if (ch == '\r' || ch == '\n') {
                if (cmd_len > 0u) {
                    cmd_buf[cmd_len] = '\0';
                    usb_case_handle_command(cmd_buf, &descriptor_profile);
                    cmd_len = 0u;
                }
                continue;
            }

            if (ch == '\b' || ch == 127) {
                if (cmd_len > 0u) {
                    cmd_len--;
                }
                continue;
            }

            if (cmd_len < (USB_CASE_CMD_BUF_SIZE - 1u)) {
                cmd_buf[cmd_len++] = (char)ch;
            } else {
                cmd_len = 0u;
                printf("command too long\r\n");
            }
        }

        if (rf_ready) {
            uint8_t rx_pipe = 0xffu;
            size_t rx_payload_len = 0u;
            rf_test_packet_t packet;
            const int rx_len = nrf24_radio_recv_any(
                &packet,
                sizeof(packet),
                &rx_pipe,
                &rx_payload_len,
                USB_CASE_RF_POLL_TIMEOUT_MS);

            if (rx_len == (int)sizeof(packet) &&
                rx_pipe == 0u &&
                rx_payload_len == sizeof(packet) &&
                packet.magic == RF_TEST_PACKET_MAGIC &&
                packet.version == RF_TEST_PACKET_VERSION &&
                packet.msg_type == RF_TEST_MSG_BOOTLOADER_REQ &&
                packet.arg0 == USB_CASE_RF_BOOTLOADER_ARG0) {
                printf("usb_case_demo: RF bootloader request received\r\n");
                printf("usb_case_demo: entering USB bootloader\r\n");
                sleep_ms(20);
                reset_usb_boot(0, 0);
            }
        }

        sleep_ms(10);
    }
}
