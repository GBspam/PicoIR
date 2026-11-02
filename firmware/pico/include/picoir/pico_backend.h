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
typedef struct {
    PIO pio; uint rx_sm; uint tx_sm;
    int rx_dma_channel; int tx_dma_channel;
    uint rx_offset; uint tx_offset; uint rx_pin; uint tx_pin;
    uint32_t sample_rate_hz; uint32_t carrier_hz;
    uint64_t tx_started_us; uint64_t tx_expected_done_us;
    uint32_t tx_words[PIR_PICO_TX_MAX_RUNS];
    bool initialized; bool rx_running; bool tx_running;
} pir_pico_backend_t;
bool pir_pico_backend_init(pir_pico_backend_t *backend, PIO pio,
                           uint rx_sm, uint tx_sm, uint rx_pin, uint tx_pin,
                           uint32_t sample_rate_hz, uint32_t carrier_hz);
bool pir_pico_rx_start(pir_pico_backend_t *backend,uint32_t *sample_words,size_t word_count);
bool pir_pico_rx_done(pir_pico_backend_t *backend);
void pir_pico_rx_abort(pir_pico_backend_t *backend);
typedef struct { uint32_t duration_us; bool carrier_on; } pir_pico_tx_run_t;
bool pir_pico_tx_start(pir_pico_backend_t *backend,const pir_pico_tx_run_t *runs,size_t run_count);
bool pir_pico_tx_done(pir_pico_backend_t *backend);
void pir_pico_tx_abort(pir_pico_backend_t *backend);
#ifdef __cplusplus
}
#endif
#endif
