#ifndef PICOIR_PICO_BACKEND_H
#define PICOIR_PICO_BACKEND_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "hardware/pio.h"
#ifdef __cplusplus
extern "C" {
#endif
#define PIR_PICO_TX_MAX_RUNS 256u
#define PIR_PICO_RX_STREAM_BUFFERS 2u
#define PIR_PICO_RX_STREAM_MIN_WORDS 8u
typedef struct {
    PIO pio; uint rx_sm; uint tx_sm;
    int rx_dma_channel; int tx_dma_channel;
    int rx_stream_dma[PIR_PICO_RX_STREAM_BUFFERS];
    uint32_t *rx_stream_buffers[PIR_PICO_RX_STREAM_BUFFERS];
    size_t rx_stream_words;
    volatile bool rx_stream_ready[PIR_PICO_RX_STREAM_BUFFERS];
    volatile bool rx_stream_in_use[PIR_PICO_RX_STREAM_BUFFERS];
    volatile uint32_t rx_stream_sequence[PIR_PICO_RX_STREAM_BUFFERS];
    volatile uint32_t rx_stream_overrun_count;
    volatile uint32_t rx_stream_next_sequence;
    uint rx_offset; uint tx_offset; uint rx_pin; uint tx_pin;
    uint32_t sample_rate_hz; uint32_t carrier_hz;
    uint64_t tx_started_us; uint64_t tx_expected_done_us;
    uint32_t tx_words[PIR_PICO_TX_MAX_RUNS];
    bool initialized; bool rx_running; bool rx_streaming; bool tx_running;
} pir_pico_backend_t;
/* Use an unused backend object and dedicated, unclaimed state machines. */
bool pir_pico_backend_init(pir_pico_backend_t *backend, PIO pio,
                           uint rx_sm, uint tx_sm, uint rx_pin, uint tx_pin,
                           uint32_t sample_rate_hz, uint32_t carrier_hz);
/* Stops activity, removes owned PIO programs, and releases claimed SMs. */
void pir_pico_backend_deinit(pir_pico_backend_t *backend);
bool pir_pico_rx_start(pir_pico_backend_t *backend,uint32_t *sample_words,size_t word_count);
bool pir_pico_rx_done(pir_pico_backend_t *backend);
void pir_pico_rx_abort(pir_pico_backend_t *backend);
/* Continuous two-buffer DMA capture. Buffers must be aligned, non-overlapping,
 * and remain allocated until stream_stop(). Use at least 8 words per buffer.
 * Each successful take creates a lease: call release with its pointer and
 * sequence after processing. Release returns false if DMA overwrote that block. */
bool pir_pico_rx_stream_start(pir_pico_backend_t *backend,
                              uint32_t *buffer0, uint32_t *buffer1,
                              size_t words_per_buffer);
bool pir_pico_rx_stream_take(pir_pico_backend_t *backend,
                             const uint32_t **samples, size_t *word_count,
                             uint32_t *sequence);
bool pir_pico_rx_stream_release(pir_pico_backend_t *backend,
                                const uint32_t *samples, uint32_t sequence);
uint32_t pir_pico_rx_stream_overruns(pir_pico_backend_t *backend);
void pir_pico_rx_stream_stop(pir_pico_backend_t *backend);
typedef struct { uint32_t duration_us; bool carrier_on; } pir_pico_tx_run_t;
bool pir_pico_tx_start(pir_pico_backend_t *backend,const pir_pico_tx_run_t *runs,size_t run_count);
bool pir_pico_tx_done(pir_pico_backend_t *backend);
void pir_pico_tx_abort(pir_pico_backend_t *backend);
#ifdef __cplusplus
}
#endif
#endif
