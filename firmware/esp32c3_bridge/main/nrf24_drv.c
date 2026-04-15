#include "nrf24_drv.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "rf_frame_v2.h"
#include "rf_test_packet.h"

static const char *TAG = "nrf24_drv";

#ifndef NRF24_DIAG
#define NRF24_DIAG 1
#endif
#ifndef NRF24_SINGLE_PIPE_RFTEST
#define NRF24_SINGLE_PIPE_RFTEST 1
#endif
#ifndef NRF24_DUAL_PIPE_COEX_TEST
#define NRF24_DUAL_PIPE_COEX_TEST 1
#endif

#define NRF24_PIN_SCK                   GPIO_NUM_4
#define NRF24_PIN_MOSI                  GPIO_NUM_6
#define NRF24_PIN_MISO                  GPIO_NUM_5
#define NRF24_PIN_CSN                   GPIO_NUM_7
#define NRF24_PIN_CE                    GPIO_NUM_10

#define NRF24_SPI_HOST                  SPI2_HOST
#define NRF24_SPI_CLOCK_HZ              (2 * 1000 * 1000)
#define NRF24_RF_CHANNEL                40u
#define NRF24_CONFIG_RX                 0x0Fu
#define NRF24_CONFIG_TX                 0x0Eu
#define NRF24_STATUS_CLEAR_IRQS         0x70u
#define NRF24_RF_SETUP_250KBPS_LOW_PWR  0x20u
#define NRF24_SETUP_AW_5BYTES           0x03u
#define NRF24_TX_TIMEOUT_MS             20

#define NRF24_CMD_R_REGISTER            0x00u
#define NRF24_CMD_W_REGISTER            0x20u
#define NRF24_CMD_R_RX_PAYLOAD          0x61u
#define NRF24_CMD_W_TX_PAYLOAD          0xA0u
#define NRF24_CMD_FLUSH_TX              0xE1u
#define NRF24_CMD_FLUSH_RX              0xE2u

#define NRF24_REG_CONFIG                0x00u
#define NRF24_REG_EN_AA                 0x01u
#define NRF24_REG_EN_RXADDR             0x02u
#define NRF24_REG_SETUP_AW              0x03u
#define NRF24_REG_SETUP_RETR            0x04u
#define NRF24_REG_RF_CH                 0x05u
#define NRF24_REG_RF_SETUP              0x06u
#define NRF24_REG_STATUS                0x07u
#define NRF24_REG_RX_ADDR_P0            0x0Au
#define NRF24_REG_RX_ADDR_P1            0x0Bu
#define NRF24_REG_TX_ADDR               0x10u
#define NRF24_REG_RX_PW_P0              0x11u
#define NRF24_REG_RX_PW_P1              0x12u
#define NRF24_REG_FIFO_STATUS           0x17u
#define NRF24_REG_DYNPD                 0x1Cu
#define NRF24_REG_FEATURE               0x1Du

#define NRF24_STATUS_RX_DR              0x40u
#define NRF24_STATUS_TX_DS              0x20u
#define NRF24_STATUS_MAX_RT             0x10u
#define NRF24_STATUS_RX_P_NO_MASK       0x0Eu
#define NRF24_STATUS_RX_P_NO_SHIFT      1u
#define NRF24_STATUS_RX_P_NO_EMPTY      0x07u
#define NRF24_FIFO_RX_EMPTY             0x01u

#define NRF24_ADDR_RFTEST_BYTES { 0xD2u, 0xF1u, 0xC0u, 0x5Au, 0x11u }
#define NRF24_ADDR_RFV2_BYTES   { 0x27u, 0x4Eu, 0x9Bu, 0xA5u, 0xEEu }

static const uint8_t k_rf_test_addr[5] = NRF24_ADDR_RFTEST_BYTES;
static const uint8_t k_rfv2_addr[5] = NRF24_ADDR_RFV2_BYTES;

static spi_device_handle_t g_spi;
static bool g_ready;
static uint8_t g_last_status;

static void nrf24_ce_low(void)
{
    gpio_set_level(NRF24_PIN_CE, 0);
}

static void nrf24_ce_high(void)
{
    gpio_set_level(NRF24_PIN_CE, 1);
}

static esp_err_t nrf24_transfer(const uint8_t *tx, uint8_t *rx, size_t len)
{
    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = tx,
        .rx_buffer = rx,
    };

    return spi_device_transmit(g_spi, &t);
}

static esp_err_t nrf24_command(uint8_t cmd, uint8_t *status_out)
{
    uint8_t tx[1] = { cmd };
    uint8_t rx[1] = { 0 };
    esp_err_t err = nrf24_transfer(tx, rx, sizeof(tx));

    if (err == ESP_OK) {
        g_last_status = rx[0];
        if (status_out != NULL) {
            *status_out = rx[0];
        }
    }

    return err;
}

static esp_err_t nrf24_write_register(uint8_t reg, uint8_t value)
{
    uint8_t tx[2] = { (uint8_t)(NRF24_CMD_W_REGISTER | reg), value };
    uint8_t rx[2] = { 0 };
    esp_err_t err = nrf24_transfer(tx, rx, sizeof(tx));

    if (err == ESP_OK) {
        g_last_status = rx[0];
    }

    return err;
}

static esp_err_t nrf24_read_register(uint8_t reg, uint8_t *value_out)
{
    uint8_t tx[2] = { (uint8_t)(NRF24_CMD_R_REGISTER | reg), 0xffu };
    uint8_t rx[2] = { 0 };
    esp_err_t err = nrf24_transfer(tx, rx, sizeof(tx));

    if (err == ESP_OK) {
        g_last_status = rx[0];
        if (value_out != NULL) {
            *value_out = rx[1];
        }
    }

    return err;
}

static esp_err_t nrf24_write_register_buf(uint8_t reg, const uint8_t *data, size_t len)
{
    uint8_t tx[1 + 5] = { 0 };
    uint8_t rx[1 + 5] = { 0 };

    if (len > 5u) {
        return ESP_ERR_INVALID_ARG;
    }

    tx[0] = (uint8_t)(NRF24_CMD_W_REGISTER | reg);
    memcpy(&tx[1], data, len);

    if (nrf24_transfer(tx, rx, len + 1u) == ESP_OK) {
        g_last_status = rx[0];
        return ESP_OK;
    }

    return ESP_FAIL;
}

static esp_err_t nrf24_read_register_buf(uint8_t reg, uint8_t *data, size_t len)
{
    uint8_t tx[1 + 5] = { 0 };
    uint8_t rx[1 + 5] = { 0 };

    if (len > 5u) {
        return ESP_ERR_INVALID_ARG;
    }

    tx[0] = (uint8_t)(NRF24_CMD_R_REGISTER | reg);
    memset(&tx[1], 0xff, len);

    if (nrf24_transfer(tx, rx, len + 1u) == ESP_OK) {
        g_last_status = rx[0];
        memcpy(data, &rx[1], len);
        return ESP_OK;
    }

    return ESP_FAIL;
}

#if NRF24_DIAG
static void nrf24_log_addr5(const char *label, const uint8_t *addr)
{
    ESP_LOGI(
        TAG,
        "%s=%02x:%02x:%02x:%02x:%02x",
        label,
        (unsigned)addr[0],
        (unsigned)addr[1],
        (unsigned)addr[2],
        (unsigned)addr[3],
        (unsigned)addr[4]);
}

static void nrf24_dump_state(const char *label)
{
    uint8_t rf_ch = 0;
    uint8_t rf_setup = 0;
    uint8_t setup_aw = 0;
    uint8_t en_aa = 0;
    uint8_t setup_retr = 0;
    uint8_t config = 0;
    uint8_t en_rxaddr = 0;
    uint8_t status = 0;
    uint8_t fifo_status = 0;
    uint8_t rx_pw_p0 = 0;
    uint8_t rx_addr_p0[5] = { 0 };
    uint8_t tx_addr[5] = { 0 };
#if NRF24_DUAL_PIPE_COEX_TEST
    uint8_t rx_pw_p1 = 0;
    uint8_t rx_addr_p1[5] = { 0 };
#endif

    (void)nrf24_read_register(NRF24_REG_RF_CH, &rf_ch);
    (void)nrf24_read_register(NRF24_REG_RF_SETUP, &rf_setup);
    (void)nrf24_read_register(NRF24_REG_SETUP_AW, &setup_aw);
    (void)nrf24_read_register(NRF24_REG_EN_AA, &en_aa);
    (void)nrf24_read_register(NRF24_REG_SETUP_RETR, &setup_retr);
    (void)nrf24_read_register(NRF24_REG_CONFIG, &config);
    (void)nrf24_read_register(NRF24_REG_EN_RXADDR, &en_rxaddr);
    (void)nrf24_read_register(NRF24_REG_STATUS, &status);
    (void)nrf24_read_register(NRF24_REG_FIFO_STATUS, &fifo_status);
    (void)nrf24_read_register(NRF24_REG_RX_PW_P0, &rx_pw_p0);
    (void)nrf24_read_register_buf(NRF24_REG_RX_ADDR_P0, rx_addr_p0, sizeof(rx_addr_p0));
    (void)nrf24_read_register_buf(NRF24_REG_TX_ADDR, tx_addr, sizeof(tx_addr));
#if NRF24_DUAL_PIPE_COEX_TEST
    (void)nrf24_read_register(NRF24_REG_RX_PW_P1, &rx_pw_p1);
    (void)nrf24_read_register_buf(NRF24_REG_RX_ADDR_P1, rx_addr_p1, sizeof(rx_addr_p1));
#endif

#if NRF24_DUAL_PIPE_COEX_TEST
    ESP_LOGI(
        TAG,
        "%s RF_CH=0x%02x RF_SETUP=0x%02x SETUP_AW=0x%02x EN_AA=0x%02x SETUP_RETR=0x%02x CFG=0x%02x EN_RXADDR=0x%02x STATUS=0x%02x FIFO=0x%02x RX_PW_P0=%u RX_PW_P1=%u",
        label,
        (unsigned)rf_ch,
        (unsigned)rf_setup,
        (unsigned)setup_aw,
        (unsigned)en_aa,
        (unsigned)setup_retr,
        (unsigned)config,
        (unsigned)en_rxaddr,
        (unsigned)status,
        (unsigned)fifo_status,
        (unsigned)rx_pw_p0,
        (unsigned)rx_pw_p1);
    nrf24_log_addr5("RX_ADDR_P0", rx_addr_p0);
    nrf24_log_addr5("RX_ADDR_P1", rx_addr_p1);
    nrf24_log_addr5("TX_ADDR", tx_addr);
#else
    ESP_LOGI(
        TAG,
        "%s RF_CH=0x%02x RF_SETUP=0x%02x SETUP_AW=0x%02x EN_AA=0x%02x SETUP_RETR=0x%02x CFG=0x%02x EN_RXADDR=0x%02x STATUS=0x%02x FIFO=0x%02x RX_PW_P0=%u",
        label,
        (unsigned)rf_ch,
        (unsigned)rf_setup,
        (unsigned)setup_aw,
        (unsigned)en_aa,
        (unsigned)setup_retr,
        (unsigned)config,
        (unsigned)en_rxaddr,
        (unsigned)status,
        (unsigned)fifo_status,
        (unsigned)rx_pw_p0);
    nrf24_log_addr5("RX_ADDR_P0", rx_addr_p0);
    nrf24_log_addr5("TX_ADDR", tx_addr);
#endif
}

static void nrf24_log_pipe_seen(uint8_t pipe_no, uint8_t status, uint8_t fifo_status)
{
    ESP_LOGI(
        TAG,
        "PIPE%u_SEEN STATUS=0x%02x FIFO=0x%02x rx_p_no=%u",
        (unsigned)pipe_no,
        (unsigned)status,
        (unsigned)fifo_status,
        (unsigned)pipe_no);
}

static void nrf24_log_pipe0_raw(const uint8_t *data, size_t len, uint8_t pipe_no)
{
    char line[3 * 16 + 32];
    size_t dump_len = len < 16u ? len : 16u;
    int offset = snprintf(line, sizeof(line), "PIPE0_RAW len=%u pipe=%u", (unsigned)len, (unsigned)pipe_no);
    size_t i;

    for (i = 0; i < dump_len && offset > 0 && offset < (int)sizeof(line); ++i) {
        offset += snprintf(line + offset, sizeof(line) - (size_t)offset, " %02x", (unsigned)data[i]);
    }
    ESP_LOGI(TAG, "%s", line);
}
#endif

static size_t nrf24_payload_size_for_pipe(uint8_t rx_p_no)
{
    if (rx_p_no == 0u) {
        return RF_TEST_PAYLOAD_SIZE;
    }
#if NRF24_DUAL_PIPE_COEX_TEST
    if (rx_p_no == 1u) {
        return RFV2_FRAME_SIZE;
    }
#endif
    return 0u;
}

static esp_err_t nrf24_configure_prx_pipes(void)
{
    ESP_ERROR_CHECK(nrf24_write_register(NRF24_REG_EN_RXADDR, NRF24_DUAL_PIPE_COEX_TEST ? 0x03u : 0x01u));
    ESP_ERROR_CHECK(nrf24_write_register_buf(NRF24_REG_RX_ADDR_P0, k_rf_test_addr, sizeof(k_rf_test_addr)));
    ESP_ERROR_CHECK(nrf24_write_register(NRF24_REG_RX_PW_P0, RF_TEST_PAYLOAD_SIZE));
#if NRF24_DUAL_PIPE_COEX_TEST
    ESP_ERROR_CHECK(nrf24_write_register_buf(NRF24_REG_RX_ADDR_P1, k_rfv2_addr, sizeof(k_rfv2_addr)));
    ESP_ERROR_CHECK(nrf24_write_register(NRF24_REG_RX_PW_P1, RFV2_FRAME_SIZE));
#endif
    ESP_ERROR_CHECK(nrf24_write_register(NRF24_REG_DYNPD, 0x00u));
    ESP_ERROR_CHECK(nrf24_write_register(NRF24_REG_FEATURE, 0x00u));
    return ESP_OK;
}

static esp_err_t nrf24_set_rx_mode(void)
{
    nrf24_ce_low();
    ESP_ERROR_CHECK(nrf24_write_register(NRF24_REG_STATUS, NRF24_STATUS_CLEAR_IRQS));
    ESP_ERROR_CHECK(nrf24_write_register(NRF24_REG_CONFIG, NRF24_CONFIG_RX));
    esp_rom_delay_us(150);
    nrf24_ce_high();
    return ESP_OK;
}

static esp_err_t nrf24_prepare_tx(const uint8_t *tx_addr)
{
    nrf24_ce_low();
    ESP_ERROR_CHECK(nrf24_write_register(NRF24_REG_STATUS, NRF24_STATUS_CLEAR_IRQS));
    ESP_ERROR_CHECK(nrf24_write_register(NRF24_REG_DYNPD, 0x00u));
    ESP_ERROR_CHECK(nrf24_write_register(NRF24_REG_FEATURE, 0x00u));
    ESP_ERROR_CHECK(nrf24_write_register_buf(NRF24_REG_TX_ADDR, tx_addr, 5u));
    ESP_ERROR_CHECK(nrf24_write_register(NRF24_REG_CONFIG, NRF24_CONFIG_TX));
    esp_rom_delay_us(150);
    ESP_ERROR_CHECK(nrf24_command(NRF24_CMD_FLUSH_TX, NULL));
#if NRF24_DIAG
    nrf24_dump_state("prepare_tx");
#endif
    return ESP_OK;
}

static esp_err_t nrf24_restore_prx(void)
{
    nrf24_ce_low();
    ESP_ERROR_CHECK(nrf24_write_register(NRF24_REG_STATUS, NRF24_STATUS_CLEAR_IRQS));
    ESP_ERROR_CHECK(nrf24_command(NRF24_CMD_FLUSH_TX, NULL));
    ESP_ERROR_CHECK(nrf24_configure_prx_pipes());
    ESP_ERROR_CHECK(nrf24_set_rx_mode());
#if NRF24_DIAG
    nrf24_dump_state("restore_prx");
#endif
    return ESP_OK;
}

static bool nrf24_payload_is_garbage(const uint8_t *data, size_t len)
{
    size_t i;
    bool all_zero = true;
    bool all_ff = true;

    for (i = 0; i < len; ++i) {
        if (data[i] != 0x00u) {
            all_zero = false;
        }
        if (data[i] != 0xffu) {
            all_ff = false;
        }
    }

    return all_zero || all_ff;
}

void nrf24_drv_recover_rx(void)
{
    if (!g_ready) {
        return;
    }

    ESP_ERROR_CHECK(nrf24_command(NRF24_CMD_FLUSH_RX, NULL));
    ESP_ERROR_CHECK(nrf24_restore_prx());
}

void nrf24_drv_init(void)
{
    spi_bus_config_t buscfg = {
        .mosi_io_num = NRF24_PIN_MOSI,
        .miso_io_num = NRF24_PIN_MISO,
        .sclk_io_num = NRF24_PIN_SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = RFV2_FRAME_SIZE + 1,
    };
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = NRF24_SPI_CLOCK_HZ,
        .mode = 0,
        .spics_io_num = NRF24_PIN_CSN,
        .queue_size = 1,
    };

    ESP_ERROR_CHECK(gpio_set_direction(NRF24_PIN_CE, GPIO_MODE_OUTPUT));
    nrf24_ce_low();

    ESP_ERROR_CHECK(spi_bus_initialize(NRF24_SPI_HOST, &buscfg, SPI_DMA_DISABLED));
    ESP_ERROR_CHECK(spi_bus_add_device(NRF24_SPI_HOST, &devcfg, &g_spi));

    ESP_ERROR_CHECK(nrf24_write_register(NRF24_REG_CONFIG, 0x08u));
    ESP_ERROR_CHECK(nrf24_write_register(NRF24_REG_EN_AA, 0x00u));
    ESP_ERROR_CHECK(nrf24_write_register(NRF24_REG_SETUP_AW, NRF24_SETUP_AW_5BYTES));
    ESP_ERROR_CHECK(nrf24_write_register(NRF24_REG_SETUP_RETR, 0x00u));
    ESP_ERROR_CHECK(nrf24_write_register(NRF24_REG_RF_CH, NRF24_RF_CHANNEL));
    ESP_ERROR_CHECK(nrf24_write_register(NRF24_REG_RF_SETUP, NRF24_RF_SETUP_250KBPS_LOW_PWR));
    ESP_ERROR_CHECK(nrf24_write_register_buf(NRF24_REG_TX_ADDR, k_rf_test_addr, sizeof(k_rf_test_addr)));
    ESP_ERROR_CHECK(nrf24_configure_prx_pipes());
    ESP_ERROR_CHECK(nrf24_write_register(NRF24_REG_STATUS, NRF24_STATUS_CLEAR_IRQS));
    ESP_ERROR_CHECK(nrf24_command(NRF24_CMD_FLUSH_TX, NULL));
    ESP_ERROR_CHECK(nrf24_command(NRF24_CMD_FLUSH_RX, NULL));
    ESP_ERROR_CHECK(nrf24_restore_prx());
    vTaskDelay(pdMS_TO_TICKS(5));

    g_ready = true;
    ESP_LOGI(TAG, "NRF24 RX bring-up ready");
#if NRF24_DIAG
    nrf24_dump_state("post_init");
#endif
}

int nrf24_drv_send(const uint8_t *data, size_t len)
{
    uint8_t tx[1 + RF_TEST_PAYLOAD_SIZE] = { 0 };
    uint8_t rx[1 + RF_TEST_PAYLOAD_SIZE] = { 0 };
    TickType_t deadline;

    if (!g_ready || data == NULL || len != RF_TEST_PAYLOAD_SIZE) {
        return -1;
    }

    ESP_ERROR_CHECK(nrf24_prepare_tx(k_rf_test_addr));

    tx[0] = NRF24_CMD_W_TX_PAYLOAD;
    memcpy(&tx[1], data, len);
    ESP_ERROR_CHECK(nrf24_transfer(tx, rx, len + 1u));
    g_last_status = rx[0];

    nrf24_ce_high();
    esp_rom_delay_us(20);
    nrf24_ce_low();

    deadline = xTaskGetTickCount() + pdMS_TO_TICKS(NRF24_TX_TIMEOUT_MS);
    while (xTaskGetTickCount() <= deadline) {
        uint8_t status = 0;

        ESP_ERROR_CHECK(nrf24_read_register(NRF24_REG_STATUS, &status));
        if ((status & (NRF24_STATUS_TX_DS | NRF24_STATUS_MAX_RT)) != 0u) {
            ESP_ERROR_CHECK(nrf24_restore_prx());
#if NRF24_DIAG
            nrf24_dump_state("post_tx_rf_test");
#endif
            return (status & NRF24_STATUS_TX_DS) != 0u ? (int)len : -1;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    ESP_ERROR_CHECK(nrf24_restore_prx());
#if NRF24_DIAG
    nrf24_dump_state("post_tx_rf_test_timeout");
#endif
    return -1;
}

int nrf24_drv_send_frame_v2(const uint8_t *data, size_t len)
{
#if NRF24_SINGLE_PIPE_RFTEST || NRF24_DUAL_PIPE_COEX_TEST
    (void)data;
    (void)len;
    return -1;
#else
    uint8_t tx[1 + RFV2_FRAME_SIZE] = { 0 };
    uint8_t rx[1 + RFV2_FRAME_SIZE] = { 0 };
    TickType_t deadline;

    if (!g_ready || data == NULL || len != RFV2_FRAME_SIZE) {
        return -1;
    }

    ESP_ERROR_CHECK(nrf24_prepare_tx(k_rfv2_addr));

    tx[0] = NRF24_CMD_W_TX_PAYLOAD;
    memcpy(&tx[1], data, len);
    ESP_ERROR_CHECK(nrf24_transfer(tx, rx, len + 1u));
    g_last_status = rx[0];

    nrf24_ce_high();
    esp_rom_delay_us(20);
    nrf24_ce_low();

    deadline = xTaskGetTickCount() + pdMS_TO_TICKS(NRF24_TX_TIMEOUT_MS);
    while (xTaskGetTickCount() <= deadline) {
        uint8_t status = 0;

        ESP_ERROR_CHECK(nrf24_read_register(NRF24_REG_STATUS, &status));
        if ((status & (NRF24_STATUS_TX_DS | NRF24_STATUS_MAX_RT)) != 0u) {
            ESP_ERROR_CHECK(nrf24_restore_prx());
#if NRF24_DIAG
            nrf24_dump_state("post_tx_rfv2");
#endif
            return (status & NRF24_STATUS_TX_DS) != 0u ? (int)len : -1;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    ESP_ERROR_CHECK(nrf24_restore_prx());
#if NRF24_DIAG
    nrf24_dump_state("post_tx_rfv2_timeout");
#endif
    return -1;
#endif
}

int nrf24_drv_recv(uint8_t *data, size_t max_len, uint8_t *pipe_out)
{
    size_t payload_len = 0u;

    return nrf24_drv_recv_any(data, max_len, pipe_out, &payload_len);
}

int nrf24_drv_recv_any(uint8_t *data, size_t max_len, uint8_t *pipe_out, size_t *payload_len_out)
{
    uint8_t tx[1 + RFV2_FRAME_SIZE] = { 0 };
    uint8_t rx[1 + RFV2_FRAME_SIZE] = { 0 };
    size_t copy_len;
    uint8_t status = 0;
    uint8_t fifo_status = NRF24_FIFO_RX_EMPTY;
    uint8_t rx_p_no = NRF24_STATUS_RX_P_NO_EMPTY;

    if (!g_ready || data == NULL || max_len < RFV2_FRAME_SIZE) {
        return 0;
    }

    if (nrf24_read_register(NRF24_REG_STATUS, &status) != ESP_OK) {
        return -1;
    }
    if (nrf24_read_register(NRF24_REG_FIFO_STATUS, &fifo_status) != ESP_OK) {
        return -1;
    }

#if NRF24_SINGLE_PIPE_RFTEST
    rx_p_no = 0u;
#else
    rx_p_no = (uint8_t)((status & NRF24_STATUS_RX_P_NO_MASK) >> NRF24_STATUS_RX_P_NO_SHIFT);
#endif
#if NRF24_DIAG
    if ((status & NRF24_STATUS_RX_DR) != 0u || (fifo_status & NRF24_FIFO_RX_EMPTY) == 0u) {
        nrf24_log_pipe_seen(rx_p_no, status, fifo_status);
    }
#endif
    if ((status & NRF24_STATUS_RX_DR) == 0u || (fifo_status & NRF24_FIFO_RX_EMPTY) != 0u
#if !NRF24_SINGLE_PIPE_RFTEST
        || rx_p_no == NRF24_STATUS_RX_P_NO_EMPTY
#endif
    ) {
        nrf24_drv_recover_rx();
        return 0;
    }

    tx[0] = NRF24_CMD_R_RX_PAYLOAD;
    copy_len = nrf24_payload_size_for_pipe(rx_p_no);
    if (copy_len == 0u) {
        nrf24_drv_recover_rx();
        return -1;
    }

    if (nrf24_transfer(tx, rx, copy_len + 1u) != ESP_OK) {
        nrf24_drv_recover_rx();
        return -1;
    }

    g_last_status = rx[0];
    memcpy(data, &rx[1], copy_len);
#if NRF24_DIAG
    if (rx_p_no == 0u) {
        nrf24_log_pipe0_raw(data, copy_len, rx_p_no);
    }
#endif
    if (nrf24_payload_is_garbage(data, copy_len)) {
        nrf24_drv_recover_rx();
        return -2;
    }

    ESP_ERROR_CHECK(nrf24_write_register(NRF24_REG_STATUS, NRF24_STATUS_CLEAR_IRQS));
    if (pipe_out != NULL) {
        *pipe_out = rx_p_no;
    }
    if (payload_len_out != NULL) {
        *payload_len_out = copy_len;
    }
    return (int)copy_len;
}

bool nrf24_drv_poll(void)
{
    uint8_t status = 0;
    uint8_t fifo_status = NRF24_FIFO_RX_EMPTY;
    uint8_t rx_p_no = NRF24_STATUS_RX_P_NO_EMPTY;

    if (!g_ready) {
        return false;
    }

    if (nrf24_read_register(NRF24_REG_STATUS, &status) != ESP_OK) {
        return false;
    }
    if (nrf24_read_register(NRF24_REG_FIFO_STATUS, &fifo_status) != ESP_OK) {
        return false;
    }

#if NRF24_SINGLE_PIPE_RFTEST
    rx_p_no = 0u;
#else
    rx_p_no = (uint8_t)((status & NRF24_STATUS_RX_P_NO_MASK) >> NRF24_STATUS_RX_P_NO_SHIFT);
#endif
#if NRF24_DIAG
    if ((status & NRF24_STATUS_RX_DR) != 0u || (fifo_status & NRF24_FIFO_RX_EMPTY) == 0u) {
        nrf24_log_pipe_seen(rx_p_no, status, fifo_status);
    }
#endif

    if (((status & NRF24_STATUS_RX_DR) == 0u) ||
        ((fifo_status & NRF24_FIFO_RX_EMPTY) != 0u)) {
        if ((status & NRF24_STATUS_RX_DR) != 0u || (fifo_status & NRF24_FIFO_RX_EMPTY) == 0u) {
            nrf24_drv_recover_rx();
        }
        return false;
    }

#if !NRF24_SINGLE_PIPE_RFTEST
    if (rx_p_no == NRF24_STATUS_RX_P_NO_EMPTY) {
        nrf24_drv_recover_rx();
        return false;
    }
#endif

    return true;
}

uint8_t nrf24_drv_last_status(void)
{
    return g_last_status;
}
