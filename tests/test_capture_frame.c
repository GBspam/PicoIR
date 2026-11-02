#include "picoir/capture_frame.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    pir_frame_event_t events[8];
    uint64_t indices[8];
    size_t count;
} event_log_t;

static void record_event(void *context, pir_frame_event_t event, uint64_t index) {
    event_log_t *log = (event_log_t *)context;
    assert(log->count < sizeof(log->events) / sizeof(log->events[0]));
    log->events[log->count] = event;
    log->indices[log->count] = index;
    log->count++;
}

static void pack_levels(const bool *levels, size_t count, uint32_t *words, size_t word_capacity) {
    size_t needed = (count + 31u) / 32u;
    assert(needed <= word_capacity);
    memset(words, 0, word_capacity * sizeof(*words));
    for (size_t i = 0; i < count; ++i) {
        if (levels[i]) words[i / 32u] |= 1u << (31u - (unsigned)(i % 32u));
    }
}

int main(void) {
    pir_frame_tracker_t tracker;
    event_log_t log = {0};
    uint32_t words[1];
    bool levels[32] = {false};

    assert(pir_frame_tracker_init(&tracker, 1000000u, true, 4u, 20u));
    for (size_t i = 8u; i < 11u; ++i) levels[i] = true;
    pack_levels(levels, 32u, words, 1u);
    assert(pir_frame_tracker_feed(&tracker, words, 1u, record_event, &log));
    assert(log.count == 2u);
    assert(log.events[0] == PIR_FRAME_STARTED && log.indices[0] == 8u);
    assert(log.events[1] == PIR_FRAME_ENDED && log.indices[1] == 14u);
    assert(!tracker.active);

    /* A continuous mark times out at the configured sample boundary. */
    memset(&log, 0, sizeof(log));
    memset(levels, 0, sizeof(levels));
    for (size_t i = 0; i < 20u; ++i) levels[i] = true;
    pack_levels(levels, 32u, words, 1u);
    pir_frame_tracker_reset(&tracker);
    assert(pir_frame_tracker_feed(&tracker, words, 1u, record_event, &log));
    assert(log.count == 2u);
    assert(log.events[0] == PIR_FRAME_STARTED && log.indices[0] == 0u);
    assert(log.events[1] == PIR_FRAME_TIMED_OUT && log.indices[1] == 20u);
    assert(!tracker.active);

    /* Active-low receiver output: low is mark; the following high gap ends it. */
    memset(&log, 0, sizeof(log));
    memset(levels, true, sizeof(levels));
    for (size_t i = 4u; i < 8u; ++i) levels[i] = false;
    pack_levels(levels, 32u, words, 1u);
    assert(pir_frame_tracker_init(&tracker, 1000000u, false, 4u, 20u));
    assert(pir_frame_tracker_feed(&tracker, words, 1u, record_event, &log));
    assert(log.count == 2u);
    assert(log.events[0] == PIR_FRAME_STARTED && log.indices[0] == 4u);
    assert(log.events[1] == PIR_FRAME_ENDED && log.indices[1] == 11u);

    /* Tracker state and absolute sample indices persist across DMA block feeds. */
    {
        bool stream_levels[64] = {false};
        uint32_t stream_words[2];
        event_log_t stream_log = {0};
        for (size_t i = 30u; i < 34u; ++i) stream_levels[i] = true;
        pack_levels(stream_levels, 64u, stream_words, 2u);
        assert(pir_frame_tracker_init(&tracker, 1000000u, true, 4u, 20u));
        assert(pir_frame_tracker_feed(&tracker, stream_words, 1u, record_event, &stream_log));
        assert(stream_log.count == 1u);
        assert(stream_log.events[0] == PIR_FRAME_STARTED && stream_log.indices[0] == 30u);
        assert(tracker.active && tracker.sample_index == 32u);
        assert(pir_frame_tracker_feed(&tracker, stream_words + 1u, 1u, record_event, &stream_log));
        assert(stream_log.count == 2u);
        assert(stream_log.events[1] == PIR_FRAME_ENDED && stream_log.indices[1] == 37u);
        assert(!tracker.active && tracker.sample_index == 64u);
    }

    assert(!pir_frame_tracker_init(&tracker, 0u, true, 4u, 20u));
    assert(!pir_frame_tracker_init(&tracker, 1000000u, true, 20u, 20u));
    assert(!pir_frame_tracker_feed(&tracker, NULL, 1u, record_event, &log));
    puts("capture frame tracker tests passed");
    return 0;
}
