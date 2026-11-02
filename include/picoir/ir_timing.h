#ifndef PICOIR_IR_TIMING_H
#define PICOIR_IR_TIMING_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Generic duration-to-event classifier. It intentionally contains no built-in
 * protocol timings: a profile must provide measured or sourced inclusive
 * duration windows. `level` is the normalized digital input level after any
 * receiver inversion has been accounted for by the capture backend.
 */
typedef enum {
    PIR_IR_LEVEL_INACTIVE = 0,
    PIR_IR_LEVEL_ACTIVE = 1
} pir_ir_level_t;

typedef struct {
    pir_ir_level_t level;
    uint32_t min_duration_us; /* inclusive */
    uint32_t max_duration_us; /* inclusive */
    uint16_t event_code;      /* caller-defined symbol/event identifier */
} pir_ir_timing_window_t;

typedef struct {
    const pir_ir_timing_window_t *windows;
    size_t window_count;
} pir_ir_timing_profile_t;

typedef enum {
    PIR_IR_TIMING_OK = 0,
    PIR_IR_TIMING_INVALID_ARGUMENT,
    PIR_IR_TIMING_INVALID_PROFILE,
    PIR_IR_TIMING_NO_MATCH,
    PIR_IR_TIMING_AMBIGUOUS
} pir_ir_timing_status_t;

/* Validate bounds and reject overlapping windows at the same signal level. */
pir_ir_timing_status_t pir_ir_timing_profile_validate(
    const pir_ir_timing_profile_t *profile);

/* Classify one measured level-duration pair using an explicit profile. */
pir_ir_timing_status_t pir_ir_classify_duration(
    const pir_ir_timing_profile_t *profile,
    pir_ir_level_t level,
    uint32_t duration_us,
    uint16_t *event_code_out);

const char *pir_ir_timing_status_name(pir_ir_timing_status_t status);

#ifdef __cplusplus
}
#endif
#endif
