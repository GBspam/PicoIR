#ifndef PICOIR_IR_PROTOCOL_H
#define PICOIR_IR_PROTOCOL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PIR_MAX_EDGES 512u
#define PIR_MAX_FRAME_BITS 64u

typedef enum {
    PIR_PROTO_UNKNOWN = 0,
    PIR_PROTO_NEC,
    PIR_PROTO_NEC_EXT,
    PIR_PROTO_SONY_SIRC12,
    PIR_PROTO_SONY_SIRC15,
    PIR_PROTO_SONY_SIRC20,
    PIR_PROTO_RC5,
    PIR_PROTO_RC6,
    PIR_PROTO_SAMSUNG,
    PIR_PROTO_PANASONIC,
    PIR_PROTO_JVC
} pir_protocol_t;

typedef struct {
    uint32_t duration_us;
    bool mark; /* true = carrier/LED on, false = space/LED off */
} pir_edge_t;

typedef struct {
    pir_protocol_t protocol;
    uint32_t address;
    uint32_t command;
    uint64_t raw_data;
    uint8_t bit_count;
    bool repeat;
    bool toggle;
    uint32_t carrier_hz; /* zero when not measured */
    uint16_t confidence; /* 0..1000 */
} pir_decode_result_t;

typedef struct {
    pir_edge_t edges[PIR_MAX_EDGES];
    size_t count;
    uint32_t carrier_hz;
} pir_waveform_t;

const char *pir_protocol_name(pir_protocol_t protocol);
bool pir_protocol_parse(const char *name, pir_protocol_t *out);

/* Encoders append alternating mark/space durations to out. */
bool pir_encode(pir_protocol_t protocol, uint32_t address, uint32_t command,
                bool repeat, bool toggle, pir_waveform_t *out);

/* Decode a captured mark/space waveform. The decoder is allocation-free. */
bool pir_decode(const pir_edge_t *edges, size_t count,
                pir_decode_result_t *out);

/* Convenience helper for protocol database/export tools. */
const char *pir_protocol_name(pir_protocol_t protocol);

#ifdef __cplusplus
}
#endif
#endif
