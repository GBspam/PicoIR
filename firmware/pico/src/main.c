#include "picoir/pico_backend.h"
#include "picoir/capture_frame.h"
#include "picoir/console.h"
#include "picoir/remote_db.h"
#include "picoir/signal_analysis.h"

#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "tusb.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define PIR_RX_PIN 2u
#define PIR_TX_PIN 3u
#define PIR_SAMPLE_RATE_HZ 100000u
#define PIR_CARRIER_HZ 38000u
#define PIR_RX_WORDS_PER_BUFFER 256u
#define PIR_HISTORY_WORDS 4096u
#define PIR_FRAME_MAX_SAMPLES 32768u
#define PIR_FRAME_WORDS ((PIR_FRAME_MAX_SAMPLES + 31u) / 32u)
#define PIR_FRAME_GAP_US 12000u
#define PIR_FRAME_TIMEOUT_US 180000u
#define PIR_RX_MARK_LEVEL false /* Typical demodulating IR receiver output is active-low. */

static pir_pico_backend_t backend;
static uint32_t rx_buffers[2][PIR_RX_WORDS_PER_BUFFER];
static uint32_t rx_block_copy[PIR_RX_WORDS_PER_BUFFER];
static uint32_t sample_history[PIR_HISTORY_WORDS];
static uint32_t frame_words[PIR_FRAME_WORDS];
static pir_edge_t frame_edges[PIR_MAX_EDGES];
static pir_pico_tx_run_t tx_runs[PIR_PICO_TX_MAX_RUNS];
static pir_frame_tracker_t frame_tracker;
static pir_console_state_t console_state;
static pir_remote_db_t remote_db;
static uint32_t expected_sequence;
static bool have_expected_sequence;
static uint64_t current_frame_start;
static bool frame_capture_active;
static uint32_t binary_dropped_blocks;

static bool console_write(void *context, const char *data, size_t length) {
    (void)context;
    while (length != 0u) {
        if (!tud_cdc_n_connected(1)) return true;
        uint32_t available = tud_cdc_n_write_available(1);
        if (available == 0u) {
            tud_cdc_n_write_flush(1);
            tud_task();
            tight_loop_contents();
            continue;
        }
        uint32_t chunk = length < available ? (uint32_t)length : available;
        uint32_t written = tud_cdc_n_write(1, data, chunk);
        if (written == 0u) {
            tud_task();
            tight_loop_contents();
            continue;
        }
        data += written;
        length -= written;
        tud_cdc_n_write_flush(1);
        tud_task();
    }
    return true;
}

static void report_line(const char *line) {
    (void)console_write(NULL, line, strlen(line));
}

static bool line_equals(const char *a, const char *b) {
    while (*a && *b) {
        char ca = *a++, cb = *b++;
        if (ca >= 'a' && ca <= 'z') ca = (char)(ca - 'a' + 'A');
        if (cb >= 'a' && cb <= 'z') cb = (char)(cb - 'a' + 'A');
        if (ca != cb) return false;
    }
    return *a == *b;
}

static void report_status(void) {
    pir_pico_rx_timing_stats_t stats;
    char line[256];
    if (!pir_pico_rx_stream_get_timing_stats(&backend, &stats)) {
        report_line("STATUS rx=stopped\n");
        return;
    }
    int n = snprintf(line, sizeof(line),
        "STATUS sample_rate_hz=%lu block_period_us=%lu blocks=%llu "
        "dma_overruns=%lu timing_faults=%lu irq_min_us=%lu irq_max_us=%lu "
        "irq_max_late_us=%lu binary_dropped_blocks=%lu\n",
        (unsigned long)stats.effective_sample_rate_hz,
        (unsigned long)stats.block_period_us,
        (unsigned long long)stats.blocks_completed,
        (unsigned long)stats.overrun_count,
        (unsigned long)stats.timing_fault_count,
        (unsigned long)stats.min_irq_interval_us,
        (unsigned long)stats.max_irq_interval_us,
        (unsigned long)stats.max_irq_lateness_us,
        (unsigned long)binary_dropped_blocks);
    if (n > 0 && (size_t)n < sizeof(line)) (void)console_write(NULL, line, (size_t)n);
}

static bool restart_receiver(void) {
    pir_frame_tracker_reset(&frame_tracker);
    frame_capture_active = false;
    have_expected_sequence = false;
    return pir_pico_rx_stream_start(&backend, rx_buffers[0], rx_buffers[1],
                                    PIR_RX_WORDS_PER_BUFFER);
}

static bool transmit_edges(const pir_edge_t *edges, size_t count, uint32_t carrier_hz) {
    if (!edges || count == 0u || count > PIR_PICO_TX_MAX_RUNS) return false;
    pir_pico_rx_stream_stop(&backend);
    pir_frame_tracker_reset(&frame_tracker);
    frame_capture_active = false;
    have_expected_sequence = false;

    for (size_t i = 0; i < count; ++i) {
        if (edges[i].duration_us == 0u) {
            (void)restart_receiver();
            return false;
        }
        tx_runs[i].duration_us = edges[i].duration_us;
        tx_runs[i].carrier_on = edges[i].mark;
    }

    uint32_t requested_carrier = carrier_hz ? carrier_hz : PIR_CARRIER_HZ;
    if (!pir_pico_tx_set_carrier_hz(&backend, requested_carrier)) {
        (void)restart_receiver();
        return false;
    }
    bool started = pir_pico_tx_start(&backend, tx_runs, count);
    if (!started) {
        (void)restart_receiver();
        return false;
    }
    uint64_t deadline = time_us_64();
    for (size_t i = 0; i < count; ++i) deadline += edges[i].duration_us;
    deadline += 250000u;
    bool done = false;
    while (time_us_64() < deadline) {
        tud_task();
        if (pir_pico_tx_done(&backend)) {
            done = true;
            break;
        }
        tight_loop_contents();
    }
    if (!done) pir_pico_tx_abort(&backend);
    bool rx_restarted = restart_receiver();
    return done && rx_restarted;
}

static bool transmit_waveform(void *context, const pir_waveform_t *waveform) {
    (void)context;
    if (!waveform) return false;
    return transmit_edges(waveform->edges, waveform->count,
                          waveform->carrier_hz ? waveform->carrier_hz : PIR_CARRIER_HZ);
}

static bool transmit_raw(void *context, const pir_edge_t *edges, size_t count,
                         uint32_t carrier_hz) {
    (void)context;
    return transmit_edges(edges, count, carrier_hz ? carrier_hz : PIR_CARRIER_HZ);
}

static bool binary_stream_block(const uint32_t *words, size_t count) {
    if (!tud_cdc_n_connected(0)) return true;
    size_t bytes = count * sizeof(uint32_t);
    if (tud_cdc_n_write_available(0) < bytes) {
        if (binary_dropped_blocks != UINT32_MAX) ++binary_dropped_blocks;
        return false;
    }
    uint32_t written = tud_cdc_n_write(0, (void const *)words, (uint32_t)bytes);
    if (written != bytes) {
        if (binary_dropped_blocks != UINT32_MAX) ++binary_dropped_blocks;
        return false;
    }
    tud_cdc_n_write_flush(0);
    return true;
}

static void history_sample(uint64_t sample_index, bool *raw_level) {
    uint64_t word_index = sample_index / 32u;
    size_t slot = (size_t)(word_index % PIR_HISTORY_WORDS);
    uint32_t word = sample_history[slot];
    unsigned bit = 31u - (unsigned)(sample_index % 32u);
    *raw_level = ((word >> bit) & 1u) != 0u;
}

static void finish_frame(uint64_t end_sample) {
    if (!frame_capture_active || end_sample < current_frame_start) return;
    uint64_t sample_count = end_sample - current_frame_start + 1u;
    if (sample_count == 0u || sample_count > PIR_FRAME_MAX_SAMPLES) {
        frame_capture_active = false;
        return;
    }

    size_t words = (size_t)((sample_count + 31u) / 32u);
    uint32_t pad_word = PIR_RX_MARK_LEVEL ? 0u : UINT32_MAX;
    for (size_t i = 0; i < words; ++i) frame_words[i] = pad_word;
    for (uint64_t i = 0; i < sample_count; ++i) {
        bool raw = false;
        history_sample(current_frame_start + i, &raw);
        size_t wi = (size_t)(i / 32u);
        unsigned shift = 31u - (unsigned)(i % 32u);
        if (raw) frame_words[wi] |= 1u << shift;
        else frame_words[wi] &= ~(1u << shift);
    }

    size_t edge_count = 0;
    pir_decode_result_t decoded;
    bool decoded_ok = pir_decode_samples(frame_words, words, backend.sample_rate_hz,
                                         PIR_RX_MARK_LEVEL, frame_edges,
                                         PIR_MAX_EDGES, &edge_count, &decoded);
    if (console_state.learning) {
        pir_console_set_decoded(&console_state, decoded_ok ? &decoded : NULL);
        if (edge_count > 0u && edge_count <= PIR_CONSOLE_CAPTURE_MAX)
            (void)pir_console_set_capture(&console_state, frame_edges, edge_count, 0u);
        if (decoded_ok) {
            char line[160];
            int n = snprintf(line, sizeof(line),
                "RX %s address=0x%lX command=0x%lX confidence=%u/1000\n",
                pir_protocol_name(decoded.protocol), (unsigned long)decoded.address,
                (unsigned long)decoded.command, (unsigned)decoded.confidence);
            if (n > 0 && (size_t)n < sizeof(line)) report_line(line);
        } else {
            report_line("RX frame captured; protocol not decoded\n");
        }
    }
    frame_capture_active = false;
}

static void frame_event(void *context, pir_frame_event_t event, uint64_t sample_index) {
    (void)context;
    if (event == PIR_FRAME_STARTED) {
        current_frame_start = sample_index;
        frame_capture_active = true;
        if (console_state.learning) {
            pir_console_set_decoded(&console_state, NULL);
            console_state.have_capture = false;
        }
    } else if (event == PIR_FRAME_ENDED) {
        finish_frame(sample_index);
    } else if (event == PIR_FRAME_TIMED_OUT) {
        frame_capture_active = false;
        if (console_state.learning) report_line("RX frame timed out\n");
    }
}

static void process_rx_block(const uint32_t *words, size_t count, uint32_t sequence) {
    if (have_expected_sequence && sequence != expected_sequence) {
        pir_frame_tracker_reset(&frame_tracker);
        frame_capture_active = false;
        report_line("RX DMA sequence gap; frame state reset\n");
    }
    expected_sequence = sequence + 1u;
    have_expected_sequence = true;

    uint64_t base_sample = frame_tracker.sample_index;
    size_t base_word = (size_t)((base_sample / 32u) % PIR_HISTORY_WORDS);
    for (size_t i = 0; i < count; ++i)
        sample_history[(base_word + i) % PIR_HISTORY_WORDS] = words[i];

    if (!pir_frame_tracker_feed(&frame_tracker, words, count, frame_event, NULL)) {
        pir_frame_tracker_reset(&frame_tracker);
        frame_capture_active = false;
        report_line("ERROR RX frame tracker rejected a sample block\n");
    }
}

static void service_console_input(void) {
    static char line[PIR_CONSOLE_LINE_MAX];
    static size_t used;
    static bool discard_until_newline;
    while (tud_cdc_n_available(1) != 0u) {
        uint8_t ch;
        if (tud_cdc_n_read(1, &ch, 1u) != 1u) break;
        if (ch == '\r') continue;
        if (ch == '\n') {
            if (discard_until_newline) {
                report_line("ERROR: line too long\n");
                discard_until_newline = false;
                used = 0u;
                continue;
            }
            line[used] = '\0';
            if (used != 0u) {
                if (line_equals(line, "STATUS")) report_status();
                else {
                    pir_console_io_t io = {
                        .write = console_write,
                        .transmit = transmit_waveform,
                        .transmit_raw = transmit_raw
                    };
                    (void)pir_console_execute(&console_state, &remote_db, &io, line);
                    if (line_equals(line, "HELP"))
                        report_line("STATUS reports sample/DMA timing and CDC0 dropped blocks\n");
                }
            }
            used = 0u;
            continue;
        }
        if (discard_until_newline) continue;
        if (used + 1u >= sizeof(line)) {
            discard_until_newline = true;
            used = 0u;
        } else {
            line[used++] = (char)ch;
        }
    }
}

int main(void) {
    (void)tud_init(0);
    pir_console_init(&console_state);
    pir_db_init(&remote_db);

    if (!pir_pico_backend_init(&backend, pio0, 0u, 1u, PIR_RX_PIN, PIR_TX_PIN,
                               PIR_SAMPLE_RATE_HZ, PIR_CARRIER_HZ)) {
        while (true) {
            tud_task();
            tight_loop_contents();
        }
    }
    gpio_pull_up(PIR_RX_PIN);
    if (!pir_frame_tracker_init(&frame_tracker, backend.sample_rate_hz,
                                PIR_RX_MARK_LEVEL, PIR_FRAME_GAP_US,
                                PIR_FRAME_TIMEOUT_US) ||
        !pir_pico_rx_stream_start(&backend, rx_buffers[0], rx_buffers[1],
                                  PIR_RX_WORDS_PER_BUFFER)) {
        pir_pico_backend_deinit(&backend);
        while (true) {
            tud_task();
            tight_loop_contents();
        }
    }

    while (true) {
        tud_task();
        service_console_input();

        const uint32_t *samples = NULL;
        size_t words = 0u;
        uint32_t sequence = 0u;
        if (pir_pico_rx_stream_take(&backend, &samples, &words, &sequence)) {
            if (words == PIR_RX_WORDS_PER_BUFFER) {
                memcpy(rx_block_copy, samples, words * sizeof(uint32_t));
                bool lease_current = pir_pico_rx_stream_release(&backend, samples, sequence);
                if (lease_current) {
                    (void)binary_stream_block(rx_block_copy, words);
                    process_rx_block(rx_block_copy, words, sequence);
                } else {
                    pir_frame_tracker_reset(&frame_tracker);
                    frame_capture_active = false;
                    have_expected_sequence = false;
                    report_line("RX DMA buffer overwritten during copy; frame state reset\n");
                }
            } else {
                (void)pir_pico_rx_stream_release(&backend, samples, sequence);
                pir_frame_tracker_reset(&frame_tracker);
                frame_capture_active = false;
                have_expected_sequence = false;
            }
        } else {
            tight_loop_contents();
        }
    }
}
