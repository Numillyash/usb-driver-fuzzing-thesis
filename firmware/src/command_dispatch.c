#include "command_dispatch.h"
#include <stddef.h>

#include <string.h>

static void command_dispatch_fill_header(command_packet_t *packet,
                                         uint8_t packet_type,
                                         uint16_t sequence,
                                         uint16_t payload_len)
{
    if (packet == NULL) {
        return;
    }

    memset(packet, 0, sizeof(*packet));
    packet->header.magic = CP_PROTOCOL_MAGIC;
    packet->header.version = CP_PROTOCOL_VERSION;
    packet->header.packet_type = packet_type;
    packet->header.header_len = CP_PROTOCOL_HEADER_SIZE;
    packet->header.sequence = sequence;
    packet->header.payload_len = payload_len;
}

void command_dispatch_init(command_dispatch_context_t *ctx,
                           mode_manager_state_t *mode_manager,
                           runtime_status_t *status)
{
    if (ctx == NULL) {
        return;
    }

    ctx->mode_manager = mode_manager;
    ctx->status = status;
}

command_dispatch_result_t command_dispatch_handle(command_dispatch_context_t *ctx,
                                                  const command_packet_t *request,
                                                  command_packet_t *response)
{
    if (ctx == NULL || request == NULL || response == NULL) {
        return COMMAND_DISPATCH_BAD_PACKET;
    }

    if (request->header.magic != CP_PROTOCOL_MAGIC || request->header.version != CP_PROTOCOL_VERSION) {
        command_dispatch_build_nack(
            request->header.packet_type,
            request->header.sequence,
            CP_ERR_BAD_VERSION,
            response);
        return COMMAND_DISPATCH_BAD_PACKET;
    }

    /* Stub only: real command parsing and payload validation are added later. */
    command_dispatch_build_nack(
        request->header.packet_type,
        request->header.sequence,
        CP_ERR_UNSUPPORTED,
        response);
    return COMMAND_DISPATCH_UNSUPPORTED;
}

bool command_dispatch_build_ack(uint8_t command_id,
                                uint16_t sequence,
                                command_packet_t *response)
{
    cp_ack_payload_t *ack;

    if (response == NULL) {
        return false;
    }

    command_dispatch_fill_header(response, CP_RSP_ACK, sequence, sizeof(cp_ack_payload_t));
    ack = (cp_ack_payload_t *)response->payload;
    ack->command_id = command_id;
    ack->status_code = 0;
    ack->reserved0 = 0;
    ack->detail = 0;
    return true;
}

bool command_dispatch_build_nack(uint8_t command_id,
                                 uint16_t sequence,
                                 cp_error_code_t error_code,
                                 command_packet_t *response)
{
    cp_nack_payload_t *nack;

    if (response == NULL) {
        return false;
    }

    command_dispatch_fill_header(response, CP_RSP_NACK, sequence, sizeof(cp_nack_payload_t));
    nack = (cp_nack_payload_t *)response->payload;
    nack->command_id = command_id;
    nack->reason_code = (uint8_t)error_code;
    nack->retry_after_ms = 0;
    nack->detail = 0;
    return true;
}
