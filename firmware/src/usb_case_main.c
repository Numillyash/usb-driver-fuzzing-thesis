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

typedef enum {
    RF_RX_STATUS_NONE = 0u,
    RF_RX_STATUS_VALID_BOOTLOADER = 1u,
    RF_RX_STATUS_WRONG_PIPE = 2u,
    RF_RX_STATUS_WRONG_LEN = 3u,
    RF_RX_STATUS_BAD_MAGIC = 4u,
    RF_RX_STATUS_BAD_VERSION = 5u,
    RF_RX_STATUS_BAD_MSG_TYPE = 6u,
    RF_RX_STATUS_BAD_ARG0 = 7u,
} usb_case_rf_last_status_t;

typedef struct {
    bool rf_ready;
    const char *rf_role;
    uint32_t rf_raw_rx_count;
    uint32_t rf_wrong_pipe_count;
    uint32_t rf_wrong_len_count;
    uint32_t rf_bad_magic_count;
    uint32_t rf_bootloader_rx_count;
    uint32_t rf_bootloader_bad_count;
    uint8_t rf_last_msg_type;
    uint16_t rf_last_seq;
    uint8_t rf_last_rx_len;
    uint8_t rf_last_pipe;
    uint8_t rf_last_payload_len;
    uint8_t rf_last_status;
} usb_case_rf_state_t;

static void usb_case_print_rfdiag(void)
{
    nrf24_radio_diag_t diag = { 0 };

    if (!nrf24_radio_read_diag(&diag)) {
        printf("rfdiag=unavailable\r\n");
        return;
    }

    printf("rfdiag_RF_CH=0x%02x\r\n", (unsigned)diag.rf_ch);
    printf("rfdiag_CONFIG=0x%02x\r\n", (unsigned)diag.config);
    printf("rfdiag_EN_RXADDR=0x%02x\r\n", (unsigned)diag.en_rxaddr);
    printf("rfdiag_RX_PW_P0=%u\r\n", (unsigned)diag.rx_pw_p0);
    printf("rfdiag_RX_PW_P1=%u\r\n", (unsigned)diag.rx_pw_p1);
    printf("rfdiag_STATUS=0x%02x\r\n", (unsigned)diag.status);
    printf("rfdiag_FIFO_STATUS=0x%02x\r\n", (unsigned)diag.fifo_status);
}

static void usb_case_print_help(void)
{
    printf("available commands: help info ping bootloader rfdiag\r\n");
}

static void usb_case_print_info(const usb_case_descriptor_profile_t *descriptor_profile, const usb_case_rf_state_t *rf_state)
{
    printf("case_id=%lu\r\n", (unsigned long)USB_CASE_ID);
    printf("case_name=%s\r\n", USB_CASE_NAME);
    printf("case_group=%s\r\n", USB_CASE_GROUP);
    printf("case_base_persona=%s\r\n", USB_CASE_BASE_PERSONA);
    printf("mutation_summary=%s\r\n", USB_CASE_MUTATION_SUMMARY);
    printf("descriptor_persona_name=%s\r\n", descriptor_profile->persona_name);
    printf("descriptor_switched=%u\r\n", descriptor_profile->descriptors_switched ? 1u : 0u);
    printf("descriptor_active_transport=%s\r\n", descriptor_profile->active_transport);
    printf("rf_ready=%u\r\n", rf_state->rf_ready ? 1u : 0u);
    printf("rf_listener_mode=%s\r\n", rf_state->rf_role);
    printf("rf_raw_rx_count=%lu\r\n", (unsigned long)rf_state->rf_raw_rx_count);
    printf("rf_wrong_pipe_count=%lu\r\n", (unsigned long)rf_state->rf_wrong_pipe_count);
    printf("rf_wrong_len_count=%lu\r\n", (unsigned long)rf_state->rf_wrong_len_count);
    printf("rf_bad_magic_count=%lu\r\n", (unsigned long)rf_state->rf_bad_magic_count);
    printf("rf_bootloader_rx_count=%lu\r\n", (unsigned long)rf_state->rf_bootloader_rx_count);
    printf("rf_bootloader_bad_count=%lu\r\n", (unsigned long)rf_state->rf_bootloader_bad_count);
    printf("rf_last_msg_type=%u\r\n", (unsigned)rf_state->rf_last_msg_type);
    printf("rf_last_seq=%u\r\n", (unsigned)rf_state->rf_last_seq);
    printf("rf_last_rx_len=%u\r\n", (unsigned)rf_state->rf_last_rx_len);
    printf("rf_last_pipe=%u\r\n", (unsigned)rf_state->rf_last_pipe);
    printf("rf_last_payload_len=%u\r\n", (unsigned)rf_state->rf_last_payload_len);
    printf("rf_last_status=%u\r\n", (unsigned)rf_state->rf_last_status);
    usb_case_print_rfdiag();
}

static void usb_case_handle_command(
    const char *cmd,
    const usb_case_descriptor_profile_t *descriptor_profile,
    const usb_case_rf_state_t *rf_state)
{
    if (strcmp(cmd, "help") == 0) {
        usb_case_print_help();
        return;
    }

    if (strcmp(cmd, "info") == 0) {
        usb_case_print_info(descriptor_profile, rf_state);
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

    if (strcmp(cmd, "rfdiag") == 0) {
        usb_case_print_rfdiag();
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
    usb_case_rf_state_t rf_state = {
        .rf_ready = false,
        .rf_role = "rx_listener",
        .rf_raw_rx_count = 0u,
        .rf_wrong_pipe_count = 0u,
        .rf_wrong_len_count = 0u,
        .rf_bad_magic_count = 0u,
        .rf_bootloader_rx_count = 0u,
        .rf_bootloader_bad_count = 0u,
        .rf_last_msg_type = 0u,
        .rf_last_seq = 0u,
        .rf_last_rx_len = 0u,
        .rf_last_pipe = 0xffu,
        .rf_last_payload_len = 0u,
        .rf_last_status = RF_RX_STATUS_NONE,
    };

    stdio_init_all();
    sleep_ms(1500);
    descriptor_profile = usb_case_get_descriptor_profile();
    rf_state.rf_ready = nrf24_radio_init_rx();
    printf("usb_case_demo: rf_init=%u\r\n", rf_state.rf_ready ? 1u : 0u);

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
            printf("rf_ready=%u\r\n", rf_state.rf_ready ? 1u : 0u);
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
                    usb_case_handle_command(cmd_buf, &descriptor_profile, &rf_state);
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

        if (rf_state.rf_ready) {
            uint8_t rx_pipe = 0u;
            size_t rx_payload_len = sizeof(rf_test_packet_t);
            rf_test_packet_t packet;
            const int rx_len = nrf24_radio_recv_rf_test_packet_raw(
                &packet,
                sizeof(packet),
                USB_CASE_RF_POLL_TIMEOUT_MS);

            if (rx_len > 0) {
                rf_state.rf_raw_rx_count++;
                rf_state.rf_last_rx_len = (uint8_t)rx_len;
                rf_state.rf_last_pipe = rx_pipe;
                rf_state.rf_last_payload_len = (uint8_t)rx_payload_len;

                if (rx_pipe != 0u) {
                    rf_state.rf_wrong_pipe_count++;
                    rf_state.rf_last_status = RF_RX_STATUS_WRONG_PIPE;
                    sleep_ms(10);
                    continue;
                }

                if (rx_len != (int)sizeof(packet) || rx_payload_len != sizeof(packet)) {
                    rf_state.rf_wrong_len_count++;
                    rf_state.rf_bootloader_bad_count++;
                    rf_state.rf_last_status = RF_RX_STATUS_WRONG_LEN;
                    sleep_ms(10);
                    continue;
                }

                rf_state.rf_last_msg_type = packet.msg_type;
                rf_state.rf_last_seq = packet.seq;

                if (packet.magic != RF_TEST_PACKET_MAGIC) {
                    rf_state.rf_bad_magic_count++;
                    rf_state.rf_bootloader_bad_count++;
                    rf_state.rf_last_status = RF_RX_STATUS_BAD_MAGIC;
                    sleep_ms(10);
                    continue;
                }

                if (packet.version != RF_TEST_PACKET_VERSION) {
                    rf_state.rf_bootloader_bad_count++;
                    rf_state.rf_last_status = RF_RX_STATUS_BAD_VERSION;
                    sleep_ms(10);
                    continue;
                }

                if (packet.msg_type != RF_TEST_MSG_BOOTLOADER_REQ) {
                    rf_state.rf_bootloader_bad_count++;
                    rf_state.rf_last_status = RF_RX_STATUS_BAD_MSG_TYPE;
                    sleep_ms(10);
                    continue;
                }

                if (packet.arg0 != USB_CASE_RF_BOOTLOADER_ARG0) {
                    rf_state.rf_bootloader_bad_count++;
                    rf_state.rf_last_status = RF_RX_STATUS_BAD_ARG0;
                    sleep_ms(10);
                    continue;
                }

                rf_state.rf_bootloader_rx_count++;
                rf_state.rf_last_status = RF_RX_STATUS_VALID_BOOTLOADER;
                printf("usb_case_demo: RF bootloader request received\r\n");
                printf("usb_case_demo: entering USB bootloader\r\n");
                sleep_ms(20);
                reset_usb_boot(0, 0);
            } else {
                rf_state.rf_last_status = RF_RX_STATUS_NONE;
            }
        }

        sleep_ms(10);
    }
}
