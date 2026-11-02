#ifndef PICOIR_CAPTURE_FRAME_H
#define PICOIR_CAPTURE_FRAME_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PIR_FRAME_STARTED = 1,
    PIR_FRAME_ENDED = 2,
    PIR_FRAME_TIMED_OUT = 3
} pir_frame_event_t;

typedef void (*pir_frame_event_callback_t)(void *context,
                                           pir_frame_event_t event,
                                           uint64_t sample_index);

typedef struct {
    uint32_t sample_rate_hz;
    bool mark_level;
    uint64_t frame_gap_samples;
    uint64_t frame_timeout_samples;
    uint64_t sample_index;
    uint64_t frame_start_sample;
    uint64_t idle_samples;
    bool active;
} pir_frame_tracker_t;

/* Durations are rounded up to whole samples. Timeout must exceed frame gap. */
bool pir_frame_tracker_init(pir_frame_tracker_t *tracker,
                            uint32_t sample_rate_hz,
                            bool mark_level,
                            uint32_t frame_gap_us,
                            uint32_t frame_timeout_us);
void pir_frame_tracker_reset(pir_frame_tracker_t *tracker);

/* Consume packed samples in chronological MSB-first order. The callback is
 * invoked synchronously for start/end/timeout events; do not re-enter tracker. */
bool pir_frame_tracker_feed(pir_frame_tracker_t *tracker,
                            const uint32_t *sample_words,
                            size_t word_count,
                            pir_frame_event_callback_t callback,
                            void *context);

#ifdef __cplusplus
}
#endif
#endif
