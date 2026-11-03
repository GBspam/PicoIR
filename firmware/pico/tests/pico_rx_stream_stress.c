#include "picoir/pico_backend.h"
#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "pico/stdlib.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define STRESS_RX_PIN 2u
#define STRESS_TX_PIN 3u
#define STRESS_SAMPLE_RATE_HZ 100000u
#define STRESS_WORDS_PER_BUFFER 1024u
#define STRESS_BLOCK_US ((uint32_t)(((uint64_t)STRESS_WORDS_PER_BUFFER * 32u * 1000000u) / STRESS_SAMPLE_RATE_HZ))

static pir_pico_backend_t backend;
static uint32_t buffers[2][STRESS_WORDS_PER_BUFFER];

static void fail(const char *message) {
    printf("FAIL: %s\n", message);
    fflush(stdout);
    while (true) tight_loop_contents();
}

static bool wait_for_block(const uint32_t **samples, size_t *words,
                           uint32_t *sequence, uint32_t timeout_us) {
    uint64_t deadline = time_us_64() + timeout_us;
    while (time_us_64() < deadline) {
        if (pir_pico_rx_stream_take(&backend, samples, words, sequence)) return true;
        tight_loop_contents();
    }
    return false;
}

static void start_stream(void) {
    if (!pir_pico_rx_stream_start(&backend, buffers[0], buffers[1],
                                  STRESS_WORDS_PER_BUFFER))
        fail("stream start failed");
}

int main(void) {
    stdio_init_all();
    sleep_ms(2000);
    printf("PicoIR RX stream stress test; sample_rate=%u Hz, block=%u us\n",
           STRESS_SAMPLE_RATE_HZ, STRESS_BLOCK_US);
    fflush(stdout);

    if (!pir_pico_backend_init(&backend, pio0, 0, 1, STRESS_RX_PIN, STRESS_TX_PIN,
                               STRESS_SAMPLE_RATE_HZ, 38000u))
        fail("backend init failed");
    gpio_pull_down(STRESS_RX_PIN);

    const uint32_t *samples = NULL;
    size_t words = 0;
    uint32_t first_sequence = 0;

    start_stream();
    if (!wait_for_block(&samples, &words, &first_sequence, STRESS_BLOCK_US * 3u))
        fail("first block timeout");
    if (words != STRESS_WORDS_PER_BUFFER) fail("unexpected block size");
    if (!pir_pico_rx_stream_release(&backend, samples, first_sequence))
        fail("fresh block lease unexpectedly stale");

    /* Delay the next DMA completion IRQ without blocking long enough to miss
     * an entire additional buffer interval. */
    busy_wait_us_32(STRESS_BLOCK_US - 10000u);
    uint32_t irq_state = save_and_disable_interrupts();
    busy_wait_us_32(20000u);
    restore_interrupts(irq_state);

    uint32_t next_sequence = 0;
    if (!wait_for_block(&samples, &words, &next_sequence, STRESS_BLOCK_US * 2u))
        fail("no block after delayed IRQ");
    if (next_sequence != first_sequence + 1u)
        fail("single delayed IRQ did not preserve the next sequence number");
    if (!pir_pico_rx_stream_release(&backend, samples, next_sequence))
        fail("post-IRQ block lease unexpectedly stale");
    pir_pico_rx_timing_stats_t timing;
    if (!pir_pico_rx_stream_get_timing_stats(&backend, &timing))
        fail("timing stats unavailable while streaming");
    if (timing.max_irq_lateness_us < 5000u)
        fail("delayed IRQ was not reflected in measured ISR lateness");
    if (timing.timing_fault_count != 0u)
        fail("bounded IRQ delay unexpectedly caused a chain restart");

    /* Mask interrupts across two block completions and verify chain recovery. */
    busy_wait_us_32(STRESS_BLOCK_US - 10000u);
    irq_state = save_and_disable_interrupts();
    busy_wait_us_32(STRESS_BLOCK_US + 20000u);
    restore_interrupts(irq_state);
    uint32_t delayed_oldest = 0u, delayed_newer = 0u;
    if (!wait_for_block(&samples, &words, &delayed_oldest, STRESS_BLOCK_US / 2u))
        fail("no pending block after coalesced DMA IRQs");
    if (!pir_pico_rx_stream_release(&backend, samples, delayed_oldest))
        fail("first coalesced block lease unexpectedly stale");
    if (!wait_for_block(&samples, &words, &delayed_newer, STRESS_BLOCK_US / 2u))
        fail("second pending block missing after coalesced DMA IRQs");
    if ((int32_t)(delayed_newer - delayed_oldest) <= 0)
        fail("coalesced DMA completions were not sequenced oldest-first");
    if (!pir_pico_rx_stream_release(&backend, samples, delayed_newer))
        fail("second coalesced block lease unexpectedly stale");
    if (!pir_pico_rx_stream_get_timing_stats(&backend, &timing) ||
        timing.timing_fault_count == 0u)
        fail("coalesced DMA IRQs did not register a timing fault");
    if (!wait_for_block(&samples, &words, &delayed_newer, STRESS_BLOCK_US * 2u))
        fail("DMA chain did not recover after coalesced IRQs");
    if (!pir_pico_rx_stream_release(&backend, samples, delayed_newer))
        fail("recovered stream lease unexpectedly stale");
    pir_pico_rx_stream_stop(&backend);
    if (pir_pico_rx_stream_take(&backend, &samples, &words, &next_sequence))
        fail("take succeeded after stop");

    start_stream();
    if (!wait_for_block(&samples, &words, &first_sequence, STRESS_BLOCK_US * 3u))
        fail("first block timeout after restart");
    if (first_sequence != 0u) fail("sequence did not reset on restart");
    if (!pir_pico_rx_stream_release(&backend, samples, first_sequence))
        fail("restart block lease unexpectedly stale");

    /* Leave both buffers unread across several rotations. Taking the two
     * surviving blocks must return them in oldest-to-newest sequence order. */
    busy_wait_us_32(STRESS_BLOCK_US * 3u);
    if (pir_pico_rx_stream_overruns(&backend) == 0u)
        fail("unread buffers did not increment overrun counter");
    uint32_t oldest_sequence = 0, newer_sequence = 0;
    if (!wait_for_block(&samples, &words, &oldest_sequence, STRESS_BLOCK_US))
        fail("no oldest pending block after consumer delay");
    if (!pir_pico_rx_stream_release(&backend, samples, oldest_sequence))
        fail("oldest pending block lease unexpectedly stale");
    if (!wait_for_block(&samples, &words, &newer_sequence, STRESS_BLOCK_US))
        fail("no second pending block after consumer delay");
    if ((int32_t)(newer_sequence - oldest_sequence) <= 0)
        fail("pending blocks were not returned oldest-first");
    if (!pir_pico_rx_stream_release(&backend, samples, newer_sequence))
        fail("second pending block lease unexpectedly stale");

    /* Hold a lease across multiple buffer rotations to force overwrite. */
    if (!wait_for_block(&samples, &words, &first_sequence, STRESS_BLOCK_US * 3u))
        fail("could not acquire delayed-consumer test block");
    busy_wait_us_32(STRESS_BLOCK_US * 3u);
    uint32_t overruns = pir_pico_rx_stream_overruns(&backend);
    if (overruns == 0u) fail("delayed consumer did not increment overrun counter");
    if (pir_pico_rx_stream_release(&backend, samples, first_sequence))
        fail("overwritten lease was incorrectly reported current");

    uint32_t recovered_sequence = 0;
    if (!wait_for_block(&samples, &words, &recovered_sequence, STRESS_BLOCK_US * 2u))
        fail("no available block after delayed consumer");
    if ((int32_t)(recovered_sequence - first_sequence) <= 0)
        fail("sequence did not advance after consumer delay");
    if (!pir_pico_rx_stream_release(&backend, samples, recovered_sequence))
        fail("recovered block lease unexpectedly stale");

    pir_pico_rx_stream_stop(&backend);
    pir_pico_backend_deinit(&backend);
    printf("PASS: bounded IRQ delay, coalesced IRQ recovery, sequence ordering, delayed-consumer overrun, stale lease, stop/restart; overruns=%lu max_irq_late_us=%lu timing_faults=%lu\n",
           (unsigned long)overruns, (unsigned long)timing.max_irq_lateness_us,
           (unsigned long)timing.timing_fault_count);
    fflush(stdout);
    while (true) tight_loop_contents();
}
