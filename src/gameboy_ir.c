#include "picoir/gameboy_ir.h"
#include <string.h>

static uint16_t packet_checksum(uint8_t command, uint8_t length,
                                const uint8_t *data) {
    uint32_t sum = (uint32_t)command + length;
    for (size_t i = 0; i < length; ++i) sum += data[i];
    return (uint16_t)sum;
}

pir_gbc_packet_status_t pir_gbc_packet_encode(
    const uint8_t *data, size_t length,
    uint8_t *out, size_t out_capacity, size_t *out_length) {
    if (!out || !out_length || (length && !data))
        return PIR_GBC_PACKET_INVALID_ARGUMENT;
    if (length > PIR_GBC_MAX_PAYLOAD)
        return PIR_GBC_PACKET_INVALID_LENGTH;

    const size_t required = length + PIR_GBC_PACKET_OVERHEAD;
    if (out_capacity < required) return PIR_GBC_PACKET_BUFFER_TOO_SMALL;

    out[0] = PIR_GBC_RAW_DATA_COMMAND;
    out[1] = (uint8_t)length;
    if (length) memcpy(&out[2], data, length);

    uint16_t checksum = packet_checksum(PIR_GBC_RAW_DATA_COMMAND,
                                        (uint8_t)length, data);
    out[2 + length] = (uint8_t)(checksum >> 8);
    out[3 + length] = (uint8_t)checksum;
    *out_length = required;
    return PIR_GBC_PACKET_OK;
}

pir_gbc_packet_status_t pir_gbc_packet_decode(
    const uint8_t *bytes, size_t byte_count, pir_gbc_packet_t *out) {
    if (!bytes || !out) return PIR_GBC_PACKET_INVALID_ARGUMENT;
    if (byte_count < PIR_GBC_PACKET_OVERHEAD)
        return PIR_GBC_PACKET_INVALID_LENGTH;
    if (bytes[0] != PIR_GBC_RAW_DATA_COMMAND)
        return PIR_GBC_PACKET_UNSUPPORTED_COMMAND;

    size_t length = bytes[1];
    size_t expected = length + PIR_GBC_PACKET_OVERHEAD;
    if (byte_count != expected) return PIR_GBC_PACKET_INVALID_LENGTH;

    uint16_t received = (uint16_t)(((uint16_t)bytes[2 + length] << 8) |
                                    bytes[3 + length]);
    uint16_t calculated = packet_checksum(bytes[0], bytes[1], &bytes[2]);
    if (received != calculated) return PIR_GBC_PACKET_CHECKSUM_MISMATCH;

    memset(out, 0, sizeof(*out));
    out->command = bytes[0];
    out->length = (uint8_t)length;
    if (length) memcpy(out->data, &bytes[2], length);
    out->checksum = received;
    out->complete = (length == 0);
    return PIR_GBC_PACKET_OK;
}

const char *pir_gbc_packet_status_name(pir_gbc_packet_status_t status) {
    switch (status) {
    case PIR_GBC_PACKET_OK: return "OK";
    case PIR_GBC_PACKET_INVALID_ARGUMENT: return "INVALID_ARGUMENT";
    case PIR_GBC_PACKET_INVALID_LENGTH: return "INVALID_LENGTH";
    case PIR_GBC_PACKET_UNSUPPORTED_COMMAND: return "UNSUPPORTED_COMMAND";
    case PIR_GBC_PACKET_CHECKSUM_MISMATCH: return "CHECKSUM_MISMATCH";
    case PIR_GBC_PACKET_BUFFER_TOO_SMALL: return "BUFFER_TOO_SMALL";
    default: return "UNKNOWN_STATUS";
    }
}
