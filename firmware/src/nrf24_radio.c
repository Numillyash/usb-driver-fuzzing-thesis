#include "nrf24_radio.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "pico/time.h"
#include "rf_frame_v2.h"
#include "rf_test_packet.h"

#define NRF24_SPI_PORT                  spi0
#define NRF24_PIN_SCK                   2
#define NRF24_PIN_MOSI                  3
#define NRF24_PIN_MISO                  4
#define NRF24_PIN_CSN                   5
#define NRF24_PIN_CE                    6

#define NRF24_SPI_BAUDRATE              2000000u
#define NRF24_RF_CHANNEL                40u
#define NRF24_CONFIG_RX                 0x0Fu
#define NRF24_CONFIG_TX                 0x0Eu
#define NRF24_STATUS_CLEAR_IRQS         0x70u
#define NRF24_RF_SETUP_250KBPS_LOW_PWR  0x20u
#define NRF24_SETUP_AW_5BYTES           0x03u
#define NRF24_TX_TIMEOUT_MS             20u
#define NRF24_RX_TIMEOUT_MS             100u

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

static const uint8_t k_rf_test_addr[5] = { 'R', 'F', 'T', '0', '1' };
static const uint8_t k_rfv2_addr[5] = { 'R', 'F', 'V', '2', '1' };

static uint8_t g_last_status;
static bool g_radio_ready;

static void nrf24_csn_high(void)
{
    gpio_put(NRF24_PIN_CSN, 1);
}

static void nrf24_csn_low(void)
{
    gpio_put(NRF24_PIN_CSN, 0);
}

static void nrf24_ce_high(void)
{
    gpio_put(NRF24_PIN_CE, 1);
}

static void nrf24_ce_low(void)
{
    gpio_put(NRF24_PIN_CE, 0);
}

static uint8_t nrf24_transfer_byte(uint8_t tx)
{
    uint8_t rx = 0;

    spi_write_read_blocking(NRF24_SPI_PORT, &tx, &rx, 1);
    return rx;
}

static uint8_t nrf24_command(uint8_t cmd)
{
    uint8_t status;

    nrf24_csn_low();
    status = nrf24_transfer_byte(cmd);
    nrf24_csn_high();
    g_last_status = status;
    return status;
}

static uint8_t nrf24_write_register(uint8_t reg, uint8_t value)
{
    uint8_t status;

    nrf24_csn_low();
    status = nrf24_transfer_byte(NRF24_CMD_W_REGISTER | reg);
    (void)nrf24_transfer_byte(value);
    nrf24_csn_high();
    g_last_status = status;
    return status;
}

static uint8_t nrf24_read_register(uint8_t reg)
{
    uint8_t status;
    uint8_t value;

    nrf24_csn_low();
    status = nrf24_transfer_byte(NRF24_CMD_R_REGISTER | reg);
    value = nrf24_transfer_byte(0xffu);
    nrf24_csn_high();
    g_last_status = status;
    return value;
}

static void nrf24_write_register_buf(uint8_t reg, const uint8_t *data, size_t len)
{
    size_t i;

    nrf24_csn_low();
    g_last_status = nrf24_transfer_byte(NRF24_CMD_W_REGISTER | reg);
    for (i = 0; i < len; ++i) {
        (void)nrf24_transfer_byte(data[i]);
    }
    nrf24_csn_high();
}

static void nrf24_write_payload(const uint8_t *data, size_t len)
{
    size_t i;

    nrf24_csn_low();
    g_last_status = nrf24_transfer_byte(NRF24_CMD_W_TX_PAYLOAD);
    for (i = 0; i < len; ++i) {
        (void)nrf24_transfer_byte(data[i]);
    }
    nrf24_csn_high();
}

static void nrf24_select_tx_addr(const uint8_t *addr)
{
    nrf24_write_register_buf(NRF24_REG_TX_ADDR, addr, 5u);
}

static void nrf24_read_payload(uint8_t *data, size_t len)
{
    size_t i;

    nrf24_csn_low();
    g_last_status = nrf24_transfer_byte(NRF24_CMD_R_RX_PAYLOAD);
    for (i = 0; i < len; ++i) {
        data[i] = nrf24_transfer_byte(0xffu);
    }
    nrf24_csn_high();
}

static void nrf24_set_tx_mode(void)
{
    nrf24_ce_low();
    nrf24_write_register(NRF24_REG_STATUS, NRF24_STATUS_CLEAR_IRQS);
    nrf24_write_register(NRF24_REG_CONFIG, NRF24_CONFIG_TX);
    sleep_us(150);
}

static void nrf24_set_rx_mode(void)
{
    nrf24_ce_low();
    nrf24_write_register(NRF24_REG_STATUS, NRF24_STATUS_CLEAR_IRQS);
    nrf24_write_register(NRF24_REG_CONFIG, NRF24_CONFIG_RX);
    sleep_us(150);
    nrf24_ce_high();
}

static bool nrf24_rx_available(void)
{
    const uint8_t status = nrf24_read_register(NRF24_REG_STATUS);
    const uint8_t fifo_status = nrf24_read_register(NRF24_REG_FIFO_STATUS);

    return ((status & NRF24_STATUS_RX_DR) != 0u) || ((fifo_status & NRF24_FIFO_RX_EMPTY) == 0u);
}

static int nrf24_rx_pipe_number(void)
{
    const uint8_t status = nrf24_read_register(NRF24_REG_STATUS);
    const uint8_t fifo_status = nrf24_read_register(NRF24_REG_FIFO_STATUS);
    const uint8_t rx_p_no = (uint8_t)((status & NRF24_STATUS_RX_P_NO_MASK) >> NRF24_STATUS_RX_P_NO_SHIFT);

    if ((fifo_status & NRF24_FIFO_RX_EMPTY) != 0u) {
        return -1;
    }
    if (rx_p_no == NRF24_STATUS_RX_P_NO_EMPTY) {
        return -1;
    }

    return (int)rx_p_no;
}

bool nrf24_radio_init_tx(void)
{
    spi_init(NRF24_SPI_PORT, NRF24_SPI_BAUDRATE);
    gpio_set_function(NRF24_PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(NRF24_PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(NRF24_PIN_MISO, GPIO_FUNC_SPI);

    gpio_init(NRF24_PIN_CSN);
    gpio_set_dir(NRF24_PIN_CSN, GPIO_OUT);
    gpio_init(NRF24_PIN_CE);
    gpio_set_dir(NRF24_PIN_CE, GPIO_OUT);

    nrf24_csn_high();
    nrf24_ce_low();
    sleep_ms(10);

    nrf24_write_register(NRF24_REG_CONFIG, 0x08u);
    nrf24_write_register(NRF24_REG_EN_AA, 0x00u);
    nrf24_write_register(NRF24_REG_EN_RXADDR, 0x03u);
    nrf24_write_register(NRF24_REG_SETUP_AW, NRF24_SETUP_AW_5BYTES);
    nrf24_write_register(NRF24_REG_SETUP_RETR, 0x00u);
    nrf24_write_register(NRF24_REG_RF_CH, NRF24_RF_CHANNEL);
    nrf24_write_register(NRF24_REG_RF_SETUP, NRF24_RF_SETUP_250KBPS_LOW_PWR);
    nrf24_write_register_buf(NRF24_REG_RX_ADDR_P0, k_rf_test_addr, sizeof(k_rf_test_addr));
    nrf24_write_register_buf(NRF24_REG_RX_ADDR_P1, k_rfv2_addr, sizeof(k_rfv2_addr));
    nrf24_write_register_buf(NRF24_REG_TX_ADDR, k_rf_test_addr, sizeof(k_rf_test_addr));
    nrf24_write_register(NRF24_REG_RX_PW_P0, RF_TEST_PAYLOAD_SIZE);
    nrf24_write_register(NRF24_REG_RX_PW_P1, RFV2_FRAME_SIZE);
    nrf24_write_register(NRF24_REG_DYNPD, 0x00u);
    nrf24_write_register(NRF24_REG_FEATURE, 0x00u);
    nrf24_write_register(NRF24_REG_STATUS, NRF24_STATUS_CLEAR_IRQS);
    (void)nrf24_command(NRF24_CMD_FLUSH_TX);
    (void)nrf24_command(NRF24_CMD_FLUSH_RX);
    nrf24_set_tx_mode();
    sleep_ms(5);

    g_radio_ready = true;
    return true;
}

bool nrf24_radio_send_fixed(const void *data, size_t len)
{
    absolute_time_t deadline;
    uint8_t payload[RF_TEST_PAYLOAD_SIZE];
    uint8_t status;

    if (!g_radio_ready || data == NULL || len != RF_TEST_PAYLOAD_SIZE) {
        return false;
    }

    memcpy(payload, data, sizeof(payload));
    nrf24_select_tx_addr(k_rf_test_addr);
    nrf24_set_tx_mode();
    (void)nrf24_command(NRF24_CMD_FLUSH_TX);
    nrf24_write_payload(payload, sizeof(payload));

    nrf24_ce_high();
    sleep_us(20);
    nrf24_ce_low();

    deadline = make_timeout_time_ms(NRF24_TX_TIMEOUT_MS);
    do {
        status = nrf24_read_register(NRF24_REG_STATUS);
        if ((status & (NRF24_STATUS_TX_DS | NRF24_STATUS_MAX_RT)) != 0u) {
            nrf24_write_register(NRF24_REG_STATUS, NRF24_STATUS_CLEAR_IRQS);
            return (status & NRF24_STATUS_TX_DS) != 0u;
        }
    } while (!time_reached(deadline));

    (void)nrf24_command(NRF24_CMD_FLUSH_TX);
    return false;
}

bool nrf24_radio_send_frame_v2(const void *data, size_t len)
{
    uint8_t payload[RFV2_FRAME_SIZE];

    if (!g_radio_ready || data == NULL || len != RFV2_FRAME_SIZE) {
        return false;
    }

    memcpy(payload, data, sizeof(payload));
    nrf24_select_tx_addr(k_rfv2_addr);
    nrf24_set_tx_mode();
    (void)nrf24_command(NRF24_CMD_FLUSH_TX);
    nrf24_write_payload(payload, sizeof(payload));

    nrf24_ce_high();
    sleep_us(20);
    nrf24_ce_low();

    {
        absolute_time_t deadline = make_timeout_time_ms(NRF24_TX_TIMEOUT_MS);
        uint8_t status;

        do {
            status = nrf24_read_register(NRF24_REG_STATUS);
            if ((status & (NRF24_STATUS_TX_DS | NRF24_STATUS_MAX_RT)) != 0u) {
                nrf24_write_register(NRF24_REG_STATUS, NRF24_STATUS_CLEAR_IRQS);
                return (status & NRF24_STATUS_TX_DS) != 0u;
            }
        } while (!time_reached(deadline));
    }

    (void)nrf24_command(NRF24_CMD_FLUSH_TX);
    return false;
}

int nrf24_radio_recv_fixed(void *data, size_t len, uint32_t timeout_ms)
{
    absolute_time_t deadline;
    uint8_t payload[RF_TEST_PAYLOAD_SIZE];

    if (!g_radio_ready || data == NULL || len != RF_TEST_PAYLOAD_SIZE) {
        return -1;
    }

    nrf24_set_rx_mode();
    deadline = make_timeout_time_ms(timeout_ms == 0 ? NRF24_RX_TIMEOUT_MS : timeout_ms);

    while (!time_reached(deadline)) {
        if (nrf24_rx_available() && nrf24_rx_pipe_number() == 0) {
            nrf24_read_payload(payload, sizeof(payload));
            memcpy(data, payload, sizeof(payload));
            nrf24_write_register(NRF24_REG_STATUS, NRF24_STATUS_CLEAR_IRQS);
            nrf24_ce_low();
            return (int)sizeof(payload);
        }
        sleep_ms(1);
    }

    nrf24_ce_low();
    return 0;
}

int nrf24_radio_recv_frame_v2(void *data, size_t len, uint32_t timeout_ms)
{
    absolute_time_t deadline;
    uint8_t payload[RFV2_FRAME_SIZE];

    if (!g_radio_ready || data == NULL || len != RFV2_FRAME_SIZE) {
        return -1;
    }

    nrf24_set_rx_mode();
    deadline = make_timeout_time_ms(timeout_ms == 0 ? NRF24_RX_TIMEOUT_MS : timeout_ms);

    while (!time_reached(deadline)) {
        if (nrf24_rx_available() && nrf24_rx_pipe_number() == 1) {
            nrf24_read_payload(payload, sizeof(payload));
            memcpy(data, payload, sizeof(payload));
            nrf24_write_register(NRF24_REG_STATUS, NRF24_STATUS_CLEAR_IRQS);
            nrf24_ce_low();
            return (int)sizeof(payload);
        }
        sleep_ms(1);
    }

    nrf24_ce_low();
    return 0;
}

uint8_t nrf24_radio_last_status(void)
{
    return g_last_status;
}
