#ifndef RF_TEST_PACKET_H
#define RF_TEST_PACKET_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RF_TEST_PACKET_MAGIC        UINT16_C(0x5246)
#define RF_TEST_PACKET_VERSION      UINT8_C(1)
#define RF_TEST_PAYLOAD_SIZE        UINT8_C(16)

typedef enum {
    RF_TEST_MSG_HEARTBEAT = 1
} rf_test_msg_type_t;

typedef enum {
    RF_TEST_FLAG_NONE = 0
} rf_test_flags_t;

typedef struct __attribute__((packed)) {
    uint16_t magic;
    uint8_t version;
    uint8_t msg_type;
    uint16_t seq;
    uint32_t uptime_ms;
    uint32_t arg0;
    uint16_t flags;
} rf_test_packet_t;

#if defined(__cplusplus)
static_assert(sizeof(rf_test_packet_t) == RF_TEST_PAYLOAD_SIZE, "rf_test_packet_t size must be 16 bytes");
#else
_Static_assert(sizeof(rf_test_packet_t) == RF_TEST_PAYLOAD_SIZE, "rf_test_packet_t size must be 16 bytes");
#endif

#ifdef __cplusplus
}
#endif

#endif /* RF_TEST_PACKET_H */
