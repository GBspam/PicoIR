#include "picoir/gameboy_ir.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    uint8_t encoded[PIR_GBC_MAX_PACKET_SIZE];
    uint8_t payload[] = { 0x00, 0x5A, 0xFF, 0x01, 0x80 };
    size_t encoded_length = 0;
    pir_gbc_packet_t packet;

    assert(pir_gbc_packet_encode(payload, sizeof(payload), encoded,
                                 sizeof(encoded), &encoded_length) == PIR_GBC_PACKET_OK);
    assert(encoded_length == sizeof(payload) + PIR_GBC_PACKET_OVERHEAD);
    assert(encoded[0] == PIR_GBC_RAW_DATA_COMMAND);
    assert(encoded[1] == sizeof(payload));
    assert(pir_gbc_packet_decode(encoded, encoded_length, &packet) == PIR_GBC_PACKET_OK);
    assert(packet.command == PIR_GBC_RAW_DATA_COMMAND);
    assert(packet.length == sizeof(payload));
    assert(!packet.complete);
    assert(memcmp(packet.data, payload, sizeof(payload)) == 0);

    /* The zero-length header is the documented end-of-communication marker. */
    assert(pir_gbc_packet_encode(NULL, 0, encoded, sizeof(encoded),
                                 &encoded_length) == PIR_GBC_PACKET_OK);
    assert(encoded_length == PIR_GBC_PACKET_OVERHEAD);
    assert(pir_gbc_packet_decode(encoded, encoded_length, &packet) == PIR_GBC_PACKET_OK);
    assert(packet.complete && packet.length == 0);

    /* Maximum-sized packet and bounds checks. */
    uint8_t maximum[PIR_GBC_MAX_PAYLOAD];
    for (size_t i = 0; i < sizeof(maximum); ++i) maximum[i] = (uint8_t)i;
    assert(pir_gbc_packet_encode(maximum, sizeof(maximum), encoded,
                                 sizeof(encoded), &encoded_length) == PIR_GBC_PACKET_OK);
    assert(encoded_length == PIR_GBC_MAX_PACKET_SIZE);
    assert(pir_gbc_packet_decode(encoded, encoded_length, &packet) == PIR_GBC_PACKET_OK);
    assert(packet.length == PIR_GBC_MAX_PAYLOAD);
    assert(memcmp(packet.data, maximum, sizeof(maximum)) == 0);

    assert(pir_gbc_packet_encode(payload, sizeof(payload), encoded,
                                 sizeof(payload) + PIR_GBC_PACKET_OVERHEAD - 1u, &encoded_length) == PIR_GBC_PACKET_BUFFER_TOO_SMALL);
    assert(pir_gbc_packet_encode(payload, PIR_GBC_MAX_PAYLOAD + 1u, encoded,
                                 sizeof(encoded), &encoded_length) == PIR_GBC_PACKET_INVALID_LENGTH);
    assert(pir_gbc_packet_decode(encoded, 3, &packet) == PIR_GBC_PACKET_INVALID_LENGTH);

    /* Reject unsupported commands, inconsistent lengths, and corrupted checksums. */
    encoded[0] = 0x00;
    assert(pir_gbc_packet_decode(encoded, encoded_length, &packet) == PIR_GBC_PACKET_UNSUPPORTED_COMMAND);
    assert(pir_gbc_packet_encode(payload, sizeof(payload), encoded,
                                 sizeof(encoded), &encoded_length) == PIR_GBC_PACKET_OK);
    assert(pir_gbc_packet_decode(encoded, encoded_length - 1, &packet) == PIR_GBC_PACKET_INVALID_LENGTH);
    encoded[2] ^= 0x01;
    assert(pir_gbc_packet_decode(encoded, encoded_length, &packet) == PIR_GBC_PACKET_CHECKSUM_MISMATCH);
    assert(strcmp(pir_gbc_packet_status_name(PIR_GBC_PACKET_OK), "OK") == 0);

    puts("PicoIR Game Boy IR packet tests passed");
    return 0;
}
