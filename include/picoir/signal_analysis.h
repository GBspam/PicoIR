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
#endif
