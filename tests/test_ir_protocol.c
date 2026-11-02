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

    /* Sony SIRC's final data space is followed by a long inter-frame gap.
     * The edge builder merges those adjacent spaces, which the decoder must
     * accept without weakening validation of the preceding bit spaces. */
    const struct {
        pir_protocol_t protocol;
        uint32_t address;
        uint32_t command;
        uint8_t bits;
    } sony_cases[] = {
        { PIR_PROTO_SONY_SIRC12, 0x05, 0x15, 12 },
        { PIR_PROTO_SONY_SIRC15, 0xA6, 0x35, 15 },
        { PIR_PROTO_SONY_SIRC20, 0x1234, 0x55, 20 },
    };
    for (size_t i = 0; i < sizeof(sony_cases) / sizeof(sony_cases[0]); ++i) {
        assert(pir_encode(sony_cases[i].protocol, sony_cases[i].address,
                          sony_cases[i].command, false, false, &w));
        assert(w.count >= (size_t)sony_cases[i].bits * 2u + 2u);
        assert(pir_decode(w.edges, w.count, &d));
        assert(d.protocol == sony_cases[i].protocol);
        assert(d.address == sony_cases[i].address);
        assert(d.command == sony_cases[i].command);
        assert(d.bit_count == sony_cases[i].bits);
    }

    /* RC5 must round-trip address, command and toggle through Manchester runs. */
    assert(pir_encode(PIR_PROTO_RC5, 0x12, 0x2D, false, true, &w));
    assert(pir_decode(w.edges, w.count, &d));
    assert(d.protocol == PIR_PROTO_RC5);
    assert(d.address == 0x12 && d.command == 0x2D && d.toggle);
    assert(d.bit_count == 14);

    /* RC6 mode 0 carries a full 8-bit address and 8-bit command. */
    assert(pir_encode(PIR_PROTO_RC6, 0xA5, 0xC3, false, true, &w));
    assert(w.count > 20);
    assert(w.edges[0].mark && w.edges[0].duration_us == 2666);
    assert(!w.edges[1].mark && w.edges[1].duration_us == 889);

    assert(pir_decode(w.edges, w.count, &d));
    assert(d.protocol == PIR_PROTO_RC6);
    assert(d.address == 0xA5 && d.command == 0xC3 && d.toggle);
    assert(d.bit_count == 21);

    pir_protocol_t p;
    assert(pir_protocol_parse("nec_ext", &p) && p == PIR_PROTO_NEC_EXT);
    assert(pir_protocol_parse("sony_sirc20", &p) && p == PIR_PROTO_SONY_SIRC20);
    puts("PicoIR protocol unit tests passed");
    return 0;
}
