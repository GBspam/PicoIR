#include "picoir/ir_timing.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    const pir_ir_timing_window_t windows[] = {
        { PIR_IR_LEVEL_ACTIVE, 100, 150, 10 },
        { PIR_IR_LEVEL_ACTIVE, 151, 200, 11 },
        { PIR_IR_LEVEL_INACTIVE, 300, 400, 20 }
    };
    const pir_ir_timing_profile_t profile = { windows, 3 };
    uint16_t event = 0xFFFF;

    assert(pir_ir_timing_profile_validate(&profile) == PIR_IR_TIMING_OK);
    assert(pir_ir_classify_duration(&profile, PIR_IR_LEVEL_ACTIVE, 100, &event) == PIR_IR_TIMING_OK);
    assert(event == 10); /* inclusive lower boundary */
    assert(pir_ir_classify_duration(&profile, PIR_IR_LEVEL_ACTIVE, 150, &event) == PIR_IR_TIMING_OK);
    assert(event == 10); /* inclusive upper boundary */
    assert(pir_ir_classify_duration(&profile, PIR_IR_LEVEL_ACTIVE, 151, &event) == PIR_IR_TIMING_OK);
    assert(event == 11); /* adjacent non-overlapping window */
    assert(pir_ir_classify_duration(&profile, PIR_IR_LEVEL_INACTIVE, 400, &event) == PIR_IR_TIMING_OK);
    assert(event == 20);
    assert(pir_ir_classify_duration(&profile, PIR_IR_LEVEL_ACTIVE, 99, &event) == PIR_IR_TIMING_NO_MATCH);
    assert(pir_ir_classify_duration(&profile, PIR_IR_LEVEL_INACTIVE, 250, &event) == PIR_IR_TIMING_NO_MATCH);

    const pir_ir_timing_window_t overlap[] = {
        { PIR_IR_LEVEL_ACTIVE, 10, 20, 1 },
        { PIR_IR_LEVEL_ACTIVE, 20, 30, 2 }
    };
    const pir_ir_timing_profile_t bad_overlap = { overlap, 2 };
    assert(pir_ir_timing_profile_validate(&bad_overlap) == PIR_IR_TIMING_INVALID_PROFILE);
    assert(pir_ir_classify_duration(&bad_overlap, PIR_IR_LEVEL_ACTIVE, 20, &event) == PIR_IR_TIMING_INVALID_PROFILE);

    const pir_ir_timing_window_t invalid_bounds[] = {
        { PIR_IR_LEVEL_ACTIVE, 30, 10, 1 }
    };
    const pir_ir_timing_profile_t bad_bounds = { invalid_bounds, 1 };
    assert(pir_ir_timing_profile_validate(&bad_bounds) == PIR_IR_TIMING_INVALID_PROFILE);
    const pir_ir_timing_window_t zero_min[] = {
        { PIR_IR_LEVEL_ACTIVE, 0, 10, 1 }
    };
    const pir_ir_timing_profile_t bad_zero = { zero_min, 1 };
    assert(pir_ir_timing_profile_validate(&bad_zero) == PIR_IR_TIMING_INVALID_PROFILE);

    assert(pir_ir_timing_profile_validate(NULL) == PIR_IR_TIMING_INVALID_ARGUMENT);
    assert(pir_ir_classify_duration(&profile, PIR_IR_LEVEL_ACTIVE, 100, NULL) == PIR_IR_TIMING_INVALID_ARGUMENT);
    assert(pir_ir_classify_duration(&profile, (pir_ir_level_t)2, 100, &event) == PIR_IR_TIMING_INVALID_ARGUMENT);
    assert(pir_ir_classify_duration(&profile, PIR_IR_LEVEL_ACTIVE, 0, &event) == PIR_IR_TIMING_INVALID_ARGUMENT);
    assert(strcmp(pir_ir_timing_status_name(PIR_IR_TIMING_NO_MATCH), "NO_MATCH") == 0);

    puts("PicoIR IR timing classifier tests passed");
    return 0;
}
