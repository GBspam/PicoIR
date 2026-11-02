#ifndef PICOIR_SIGNAL_ANALYSIS_H
#define PICOIR_SIGNAL_ANALYSIS_H
#include "picoir/ir_protocol.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct { uint32_t frequency_hz; uint16_t confidence; uint32_t period_count; } pir_carrier_result_t;
/* Estimate carrier from rising-edge-to-rising-edge period samples in nanoseconds.
 * This expects actual carrier-cycle timing, not ordinary demodulated receiver edges. */
bool pir_measure_carrier_periods(const uint32_t *period_ns, size_t count, pir_carrier_result_t *out);
/* Analyze an edge capture, report protocol decode if available and total duration. */
bool pir_analyze_capture(const pir_edge_t *edges, size_t count, pir_decode_result_t *decoded, uint64_t *duration_us);
/* Convert PIO-packed digital samples to mark/space runs. Samples are ordered
 * oldest-first in bit 31 of each word. mark_level says which raw GPIO level
 * represents an IR mark. Durations are rounded to nearest microsecond. */
bool pir_samples_to_edges(const uint32_t *sample_words, size_t word_count,
                          uint32_t sample_rate_hz, bool mark_level,
                          pir_edge_t *edges, size_t edge_capacity,
                          size_t *edge_count);
/* Convert sampled GPIO words to edges and decode the resulting waveform.
 * Leading inactive/space runs are skipped for decoding. edge_count is set after
 * successful conversion, even if no protocol matches. */
bool pir_decode_samples(const uint32_t *sample_words, size_t word_count,
                        uint32_t sample_rate_hz, bool mark_level,
                        pir_edge_t *edges, size_t edge_capacity,
                        size_t *edge_count, pir_decode_result_t *decoded);
#endif
