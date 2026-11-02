#include "picoir/ir_protocol.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
    pir_waveform_t w;
    pir_decode_result_t d;
    assert(pir_encode(PIR_PROTO_NEC, 0x20, 0x10, false, false, &w));
    assert(w.count > 60);
    assert(pir_decode(w.edges, w.count, &d));
    assert(d.protocol == PIR_PROTO_NEC);
    assert(d.address == 0x20 && d.command == 0x10);

    assert(pir_encode(PIR_PROTO_NEC, 0x20, 0x10, true, false, &w));
    assert(pir_decode(w.edges, w.count, &d) && d.repeat);

    assert(pir_encode(PIR_PROTO_SONY_SIRC12, 0x05, 0x15, false, false, &w));
    assert(w.count >= 26);

    pir_protocol_t p;
    assert(pir_protocol_parse("nec_ext", &p) && p == PIR_PROTO_NEC_EXT);
    assert(pir_protocol_parse("sony_sirc20", &p) && p == PIR_PROTO_SONY_SIRC20);
    puts("PicoIR protocol unit tests passed");
    return 0;
}
