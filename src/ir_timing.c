#include "picoir/ir_timing.h"

static bool valid_level(pir_ir_level_t level) {
    return level == PIR_IR_LEVEL_INACTIVE || level == PIR_IR_LEVEL_ACTIVE;
}

pir_ir_timing_status_t pir_ir_timing_profile_validate(
    const pir_ir_timing_profile_t *profile) {
    if (profile == NULL || profile->windows == NULL || profile->window_count == 0)
        return PIR_IR_TIMING_INVALID_ARGUMENT;

    for (size_t i = 0; i < profile->window_count; ++i) {
        const pir_ir_timing_window_t *a = &profile->windows[i];
        if (!valid_level(a->level) || a->min_duration_us == 0 ||
            a->min_duration_us > a->max_duration_us)
            return PIR_IR_TIMING_INVALID_PROFILE;

        for (size_t j = i + 1; j < profile->window_count; ++j) {
            const pir_ir_timing_window_t *b = &profile->windows[j];
            if (!valid_level(b->level) || b->min_duration_us == 0 ||
                b->min_duration_us > b->max_duration_us)
                return PIR_IR_TIMING_INVALID_PROFILE;
            if (a->level == b->level &&
                a->min_duration_us <= b->max_duration_us &&
                b->min_duration_us <= a->max_duration_us)
                return PIR_IR_TIMING_INVALID_PROFILE;
        }
    }
    return PIR_IR_TIMING_OK;
}

pir_ir_timing_status_t pir_ir_classify_duration(
    const pir_ir_timing_profile_t *profile,
    pir_ir_level_t level,
    uint32_t duration_us,
    uint16_t *event_code_out) {
    if (profile == NULL || profile->windows == NULL ||
        profile->window_count == 0 || event_code_out == NULL ||
        !valid_level(level) || duration_us == 0)
        return PIR_IR_TIMING_INVALID_ARGUMENT;

    pir_ir_timing_status_t validation = pir_ir_timing_profile_validate(profile);
    if (validation != PIR_IR_TIMING_OK) return validation;

    size_t matches = 0;
    uint16_t matched_code = 0;
    for (size_t i = 0; i < profile->window_count; ++i) {
        const pir_ir_timing_window_t *window = &profile->windows[i];
        if (window->level == level && duration_us >= window->min_duration_us &&
            duration_us <= window->max_duration_us) {
            ++matches;
            matched_code = window->event_code;
        }
    }
    if (matches == 0) return PIR_IR_TIMING_NO_MATCH;
    if (matches > 1) return PIR_IR_TIMING_AMBIGUOUS;
    *event_code_out = matched_code;
    return PIR_IR_TIMING_OK;
}

const char *pir_ir_timing_status_name(pir_ir_timing_status_t status) {
    switch (status) {
    case PIR_IR_TIMING_OK: return "OK";
    case PIR_IR_TIMING_INVALID_ARGUMENT: return "INVALID_ARGUMENT";
    case PIR_IR_TIMING_INVALID_PROFILE: return "INVALID_PROFILE";
    case PIR_IR_TIMING_NO_MATCH: return "NO_MATCH";
    case PIR_IR_TIMING_AMBIGUOUS: return "AMBIGUOUS";
    default: return "UNKNOWN_STATUS";
    }
}
