#include "picoir/ir_protocol.h"
#include <assert.h>
#include <stdio.h>

static void fixture_edge(pir_edge_t *edges, size_t *count, bool mark, uint32_t duration) {
    edges[*count] = (pir_edge_t){ .duration_us = duration, .mark = mark };
    ++*count;
}

/* External spec-derived JVC vector: address 0x59, command 0x35.
 * IRP: {38k,525}<1,-1|1,-3>(16,-8,(D:8,F:8,1,-45)+), LSB-first. */
static size_t make_jvc_reference(pir_edge_t *edges, uint8_t address, uint8_t command) {
    size_t n = 0;
    fixture_edge(edges, &n, true, 8400);
    fixture_edge(edges, &n, false, 4200);
    uint16_t wire = (uint16_t)address | ((uint16_t)command << 8);
    for (unsigned bit = 0; bit < 16; ++bit) {
        fixture_edge(edges, &n, true, 525);
        fixture_edge(edges, &n, false, (wire & (1u << bit)) ? 1575 : 525);
    }
    fixture_edge(edges, &n, true, 525);
    fixture_edge(edges, &n, false, 40000);
    return n;
}

static size_t make_samsung_reference(pir_edge_t *edges, uint16_t address, uint8_t command) {
    size_t n = 0;
    uint32_t data = (uint32_t)address | ((uint32_t)command << 16) |
                    ((uint32_t)(uint8_t)~command << 24);
    fixture_edge(edges, &n, true, 4500);
    fixture_edge(edges, &n, false, 4500);
    for (unsigned bit = 0; bit < 32; ++bit) {
        fixture_edge(edges, &n, true, 560);
        fixture_edge(edges, &n, false, (data & (1u << bit)) ? 1690 : 560);
    }
    fixture_edge(edges, &n, true, 560);
    fixture_edge(edges, &n, false, 40000);
    return n;
}

static size_t make_kaseikyo_length_reference(pir_edge_t *edges) {
    size_t n = 0;
    fixture_edge(edges, &n, true, 3456);
    fixture_edge(edges, &n, false, 1728);
    for (unsigned bit = 0; bit < 48; ++bit) {
        fixture_edge(edges, &n, true, 432);
        fixture_edge(edges, &n, false, 432);
    }
    fixture_edge(edges, &n, true, 432);
    fixture_edge(edges, &n, false, 40000);
    return n;
}

static void assert_waveforms_equal(const pir_waveform_t *wave,
                                   const pir_edge_t *edges, size_t count) {
    assert(wave->count == count);
    for (size_t i = 0; i < count; ++i) {
        assert(wave->edges[i].mark == edges[i].mark);
        assert(wave->edges[i].duration_us == edges[i].duration_us);
    }
}

int main(void) {
    pir_waveform_t w;
    pir_decode_result_t d;
    pir_edge_t fixture[PIR_MAX_EDGES];
    size_t fixture_count;

    assert(pir_encode(PIR_PROTO_NEC, 0x20, 0x10, false, false, &w));
    assert(w.count > 60 && pir_decode(w.edges, w.count, &d));
    assert(d.protocol == PIR_PROTO_NEC && d.address == 0x20 && d.command == 0x10);
    assert(pir_encode(PIR_PROTO_NEC, 0x20, 0x10, true, false, &w));
    assert(pir_decode(w.edges, w.count, &d) && d.repeat);

    assert(pir_encode(PIR_PROTO_NEC, 0x20, 0x10, false, false, &w));
    w.edges[2u + 24u * 2u + 1u].duration_us =
        w.edges[2u + 24u * 2u + 1u].duration_us == 560u ? 1690u : 560u;
    assert(!pir_decode(w.edges, w.count, &d));

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
        assert(pir_decode(w.edges, w.count, &d));
        assert(d.protocol == sony_cases[i].protocol);
        assert(d.address == sony_cases[i].address && d.command == sony_cases[i].command);
        assert(d.bit_count == sony_cases[i].bits);
    }

    assert(pir_encode(PIR_PROTO_RC5, 0x12, 0x2D, false, true, &w));
    assert(pir_decode(w.edges, w.count, &d));
    assert(d.protocol == PIR_PROTO_RC5 && d.address == 0x12 && d.command == 0x2D && d.toggle);
    assert(d.bit_count == 14);
    assert(pir_encode(PIR_PROTO_RC5, 0x12, 0x65, false, false, &w));
    assert(pir_decode(w.edges, w.count, &d));
    assert(d.protocol == PIR_PROTO_RC5 && d.address == 0x12 && d.command == 0x65);

    assert(pir_encode(PIR_PROTO_NEC_EXT, 0x0020, 0x10, false, false, &w));
    assert(pir_decode(w.edges, w.count, &d));
    assert(d.protocol == PIR_PROTO_NEC_EXT && d.address == 0x0020 && d.command == 0x10);
    assert(pir_encode(PIR_PROTO_NEC_EXT, 0x1234, 0x56, false, false, &w));
    assert(pir_decode(w.edges, w.count, &d));
    assert(d.protocol == PIR_PROTO_NEC_EXT && d.address == 0x1234 && d.command == 0x56);

    fixture_count = make_jvc_reference(fixture, 0x59, 0x35);
    assert(pir_decode(fixture, fixture_count, &d));
    assert(d.protocol == PIR_PROTO_JVC && d.address == 0x59 && d.command == 0x35);
    assert(pir_encode(PIR_PROTO_JVC, 0x59, 0x35, false, false, &w));
    assert_waveforms_equal(&w, fixture, fixture_count);
    assert(pir_decode(w.edges, w.count, &d));
    assert(d.protocol == PIR_PROTO_JVC && d.address == 0x59 && d.command == 0x35);
    fixture[0].duration_us = 1000;
    assert(!pir_decode(fixture, fixture_count, &d));

    fixture_count = make_samsung_reference(fixture, 0x1234, 0x56);
    assert(pir_decode(fixture, fixture_count, &d));
    assert(d.protocol == PIR_PROTO_SAMSUNG && d.address == 0x1234 && d.command == 0x56);
    assert(pir_encode(PIR_PROTO_SAMSUNG, 0x1234, 0x56, false, false, &w));
    assert_waveforms_equal(&w, fixture, fixture_count);
    assert(pir_encode(PIR_PROTO_SAMSUNG, 0x1234, 0x56, true, false, &w));
    assert_waveforms_equal(&w, fixture, fixture_count);
    fixture[2u + 24u * 2u + 1u].duration_us =
        fixture[2u + 24u * 2u + 1u].duration_us == 560u ? 1690u : 560u;
    assert(!pir_decode(fixture, fixture_count, &d));

    assert(pir_encode(PIR_PROTO_PANASONIC, 0x1234, 0x56, false, false, &w));
    assert(pir_decode(w.edges, w.count, &d));
    assert(d.protocol == PIR_PROTO_PANASONIC && d.address == 0x1234 && d.command == 0x56);
    fixture_count = make_kaseikyo_length_reference(fixture);
    assert(!pir_decode(fixture, fixture_count, &d));

    assert(!pir_decode(NULL, 0, &d));
    assert(!pir_decode(fixture, 3, &d));

    assert(pir_encode(PIR_PROTO_RC6, 0xA5, 0xC3, false, true, &w));
    assert(w.edges[0].mark && w.edges[0].duration_us == 2666);
    assert(!w.edges[1].mark && w.edges[1].duration_us == 889);
    assert(pir_decode(w.edges, w.count, &d));
    assert(d.protocol == PIR_PROTO_RC6 && d.address == 0xA5 &&
           d.command == 0xC3 && d.toggle && d.bit_count == 21);

    pir_protocol_t p;
    assert(pir_protocol_parse("nec_ext", &p) && p == PIR_PROTO_NEC_EXT);
    assert(pir_protocol_parse("sony_sirc20", &p) && p == PIR_PROTO_SONY_SIRC20);
    puts("PicoIR protocol unit tests passed");
    return 0;
}
