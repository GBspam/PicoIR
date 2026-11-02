#ifndef PICOIR_GAMEBOY_IR_H
#define PICOIR_GAMEBOY_IR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Game Boy Color packet format described in Nintendo's Game Boy Programming
 * Manual. This is a packet codec, not a claim that every GBC game uses this
 * format. Game-specific pulse protocols need separate profiles.
 */
#define PIR_GBC_MAX_PAYLOAD 255u
#define PIR_GBC_PACKET_OVERHEAD 4u /* command + length + 16-bit checksum */
#define PIR_GBC_MAX_PACKET_SIZE (PIR_GBC_MAX_PAYLOAD + PIR_GBC_PACKET_OVERHEAD)
/* CDC1 line protocol currently fits packets with at most 68 payload bytes. */
#define PIR_GBC_CONSOLE_MAX_PAYLOAD 68u
#define PIR_GBC_CONSOLE_PACKET_MAX (PIR_GBC_CONSOLE_MAX_PAYLOAD + PIR_GBC_PACKET_OVERHEAD)
#define PIR_GBC_RAW_DATA_COMMAND 0x5Au

typedef enum {
    PIR_GBC_PACKET_OK = 0,
    PIR_GBC_PACKET_INVALID_ARGUMENT,
    PIR_GBC_PACKET_INVALID_LENGTH,
    PIR_GBC_PACKET_UNSUPPORTED_COMMAND,
    PIR_GBC_PACKET_CHECKSUM_MISMATCH,
    PIR_GBC_PACKET_BUFFER_TOO_SMALL
} pir_gbc_packet_status_t;

typedef struct {
    uint8_t command;
    uint8_t length;
    uint8_t data[PIR_GBC_MAX_PAYLOAD];
    uint16_t checksum;
    bool complete; /* true when length == 0 */
} pir_gbc_packet_t;

/*
 * Encode a packet to bytes: command, length, payload, checksum high byte,
 * checksum low byte. The checksum is the 16-bit sum of command, length, and
 * every payload byte. Bits are transmitted MSB-first by the GBC link layer;
 * pulse timing and synchronization are outside this byte-level API.
 */
pir_gbc_packet_status_t pir_gbc_packet_encode(
    const uint8_t *data, size_t length,
    uint8_t *out, size_t out_capacity, size_t *out_length);

/* Decode one complete byte-level packet and verify command, length, checksum. */
pir_gbc_packet_status_t pir_gbc_packet_decode(
    const uint8_t *bytes, size_t byte_count, pir_gbc_packet_t *out);

const char *pir_gbc_packet_status_name(pir_gbc_packet_status_t status);

#ifdef __cplusplus
}
#endif
#endif
