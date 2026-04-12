#ifndef BRIDGE_UART_H
#define BRIDGE_UART_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void bridge_uart_init(void);
int bridge_uart_read(uint8_t *buf, size_t buf_len, uint32_t timeout_ms);
int bridge_uart_write(const void *data, size_t len);
int bridge_uart_write_str(const char *str);

#ifdef __cplusplus
}
#endif

#endif /* BRIDGE_UART_H */
