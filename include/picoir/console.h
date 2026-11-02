#ifndef PICOIR_CONSOLE_H
#define PICOIR_CONSOLE_H
#include "picoir/remote_db.h"
#include "picoir/gameboy_ir.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PIR_CONSOLE_LINE_MAX 160u
#define PIR_CONSOLE_CAPTURE_MAX PIR_DB_RAW_MAX_EDGES

typedef struct {
    bool have_decoded;
    pir_decode_result_t decoded;
    bool have_capture;
    uint16_t capture_count;
    uint32_t capture_carrier_hz;
    pir_edge_t capture[PIR_CONSOLE_CAPTURE_MAX];
    bool learning;
    bool gameboy_mode;
    bool gameboy_connected;
} pir_console_state_t;
typedef struct {
    pir_write_fn write;
    void *write_context;
    /* Backend callback must consume/copy the waveform before returning. */
    bool (*transmit)(void *context, const pir_waveform_t *waveform);
    bool (*transmit_raw)(void *context, const pir_edge_t *edges, size_t count, uint32_t carrier_hz);
    void *transmit_context;
    /* Game Boy callbacks operate on a complete byte-level packet. The backend
     * owns pulse timing, connector echo, status response, and hardware timeouts.
     * Return false when no valid receiver status/packet was obtained. */
    bool (*gameboy_transmit_packet)(void *context, const uint8_t *packet,
                                    size_t packet_length, bool connect,
                                    uint8_t *receiver_status);
    bool (*gameboy_receive_packet)(void *context, uint8_t *packet,
                                   size_t packet_capacity, size_t *packet_length,
                                   uint32_t timeout_ms);
    void *gameboy_context;
} pir_console_io_t;

void pir_console_init(pir_console_state_t *state);
/* Capture is copied into bounded state. A capture too large is rejected, never truncated. */
bool pir_console_set_capture(pir_console_state_t *state, const pir_edge_t *edges, size_t count, uint32_t carrier_hz);
void pir_console_set_decoded(pir_console_state_t *state, const pir_decode_result_t *decoded);
/* Executes one complete CDC1 command line. Returns false for malformed/unsupported commands. */
bool pir_console_execute(pir_console_state_t *state, pir_remote_db_t *db, const pir_console_io_t *io, const char *line);
#endif
