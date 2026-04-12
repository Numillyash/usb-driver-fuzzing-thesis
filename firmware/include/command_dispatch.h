#ifndef COMMAND_DISPATCH_H
#define COMMAND_DISPATCH_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "mode_manager.h"
#include "protocol.h"
#include "status.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    COMMAND_DISPATCH_OK = 0,
    COMMAND_DISPATCH_IGNORED = 1,
    COMMAND_DISPATCH_BAD_PACKET = 2,
    COMMAND_DISPATCH_UNSUPPORTED = 3
} command_dispatch_result_t;

typedef struct {
    mode_manager_state_t *mode_manager;
    runtime_status_t *status;
} command_dispatch_context_t;

typedef struct {
    cp_header_t header;
    uint8_t payload[CP_PACKET_PAYLOAD_MAX];
} command_packet_t;

void command_dispatch_init(command_dispatch_context_t *ctx,
                           mode_manager_state_t *mode_manager,
                           runtime_status_t *status);
command_dispatch_result_t command_dispatch_handle(command_dispatch_context_t *ctx,
                                                  const command_packet_t *request,
                                                  command_packet_t *response);
bool command_dispatch_build_ack(uint8_t command_id,
                                uint16_t sequence,
                                command_packet_t *response);
bool command_dispatch_build_nack(uint8_t command_id,
                                 uint16_t sequence,
                                 cp_error_code_t error_code,
                                 command_packet_t *response);

#ifdef __cplusplus
}
#endif

#endif /* COMMAND_DISPATCH_H */
