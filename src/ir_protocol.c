#include "picoir/ir_protocol.h"
#include <ctype.h>
#include <string.h>

#define TOLERANCE_PCT 25u
#define MAX(a,b) ((a) > (b) ? (a) : (b))

static bool near_us(uint32_t actual, uint32_t expected, uint32_t pct) {
    uint32_t delta = actual > expected ? actual - expected : expected - actual;
    return (uint64_t)delta * 100u <= (uint64_t)expected * pct;
}

const char *pir_protocol_name(pir_protocol_t p) {
    switch (p) {
    case PIR_PROTO_NEC: return "NEC";
    case PIR_PROTO_NEC_EXT: return "NEC_EXT";
    case PIR_PROTO_SONY_SIRC12: return "SONY_SIRC12";
    case PIR_PROTO_SONY_SIRC15: return "SONY_SIRC15";
    case PIR_PROTO_SONY_SIRC20: return "SONY_SIRC20";
    case PIR_PROTO_RC5: return "RC5";
    case PIR_PROTO_RC6: return "RC6";
    case PIR_PROTO_SAMSUNG: return "SAMSUNG";
    case PIR_PROTO_PANASONIC: return "PANASONIC";
    case PIR_PROTO_JVC: return "JVC";
    default: return "UNKNOWN";
    }
}

static int ascii_casecmp(const char *a, const char *b) {
    while (*a && *b) {
        int ca = tolower((unsigned char)*a++), cb = tolower((unsigned char)*b++);
        if (ca != cb) return ca - cb;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

bool pir_protocol_parse(const char *name, pir_protocol_t *out) {
    if (!name || !out) return false;
    for (int p = PIR_PROTO_NEC; p <= PIR_PROTO_JVC; ++p) {
        if (!ascii_casecmp(name, pir_protocol_name((pir_protocol_t)p))) {
            *out = (pir_protocol_t)p;
            return true;
        }
    }
    if (!ascii_casecmp(name, "SONY") || !ascii_casecmp(name, "SIRC")) {
        *out = PIR_PROTO_SONY_SIRC12; return true;
    }
    if (!ascii_casecmp(name, "NEC-EXT")) { *out = PIR_PROTO_NEC_EXT; return true; }
    return false;
}

static bool push(pir_waveform_t *w, bool mark, uint32_t us) {
    if (!w || !us) return false;
    if (w->count && w->edges[w->count - 1].mark == mark) {
        if (UINT32_MAX - w->edges[w->count - 1].duration_us < us) return false;
        w->edges[w->count - 1].duration_us += us;
        return true;
    }
    if (w->count >= PIR_MAX_EDGES) return false;
    w->edges[w->count++] = (pir_edge_t){ .duration_us = us, .mark = mark };
    return true;
}

static bool bit_lsb(pir_waveform_t *w, bool bit, uint32_t mark, uint32_t zero_space, uint32_t one_space) {
    return push(w, true, mark) && push(w, false, bit ? one_space : zero_space);
}

static bool encode_nec(uint32_t addr, uint32_t cmd, bool repeat, bool samsung, pir_waveform_t *w) {
    if (repeat) return push(w, true, 9000) && push(w, false, 2250) && push(w, true, 560) && push(w, false, 40000);
    uint8_t a0 = (uint8_t)addr, a1 = (uint8_t)(addr >> 8), c = (uint8_t)cmd;
    if (samsung) {
        if (!push(w, true, 4500) || !push(w, false, 4500)) return false;
        for (unsigned i=0;i<16;i++) if (!bit_lsb(w, (addr >> i)&1u, 560, 560, 1690)) return false;
        for (unsigned i=0;i<8;i++) if (!bit_lsb(w, (cmd >> i)&1u, 560, 560, 1690)) return false;
        for (unsigned i=0;i<8;i++) if (!bit_lsb(w, ((~cmd) >> i)&1u, 560, 560, 1690)) return false;
        return push(w, true, 560) && push(w, false, 40000);
    }
    if (!push(w, true, 9000) || !push(w, false, 4500)) return false;
    uint8_t bytes[4];
    if (a1 || addr > 0xFFu) { bytes[0]=a0; bytes[1]=a1; }
    else { bytes[0]=a0; bytes[1]=(uint8_t)~a0; }
    bytes[2]=c; bytes[3]=(uint8_t)~c;
    for (unsigned j=0;j<4;j++) for (unsigned i=0;i<8;i++)
        if (!bit_lsb(w, (bytes[j] >> i)&1u, 560, 560, 1690)) return false;
    return push(w, true, 560) && push(w, false, 40000);
}

static bool encode_sony(uint32_t addr, uint32_t cmd, unsigned bits, pir_waveform_t *w) {
    uint64_t data = ((uint64_t)addr << 7) | (cmd & 0x7Fu);
    if (bits == 15) data = ((uint64_t)addr << 7) | (cmd & 0x7Fu);
    if (bits == 20) data = ((uint64_t)addr << 7) | (cmd & 0x7Fu);
    if (!push(w, true, 2400) || !push(w, false, 600)) return false;
    for (unsigned i=0;i<bits;i++) {
        bool b = (data >> i) & 1u;
        if (!push(w, true, b ? 1200 : 600) || !push(w, false, 600)) return false;
    }
    return push(w, false, 45000);
}

/* Manchester helpers. A run is merged by push(); adjacent halves of the same
 * level therefore naturally become a 2T edge. */
static bool manchester_bit(pir_waveform_t *w, bool bit, uint32_t half_us) {
    /* RC5: logical one is mark then space; zero is space then mark. */
    if (bit) return push(w, true, half_us) && push(w, false, half_us);
    return push(w, false, half_us) && push(w, true, half_us);
}

static bool encode_rc5(uint32_t addr, uint32_t cmd, bool toggle, pir_waveform_t *w) {
    uint16_t frame = (uint16_t)((1u<<13) | (1u<<12) | ((toggle?1u:0u)<<11) |
                    ((addr & 0x1Fu)<<6) | (cmd & 0x3Fu));
    for (int i=13;i>=0;i--) if (!manchester_bit(w, (frame>>i)&1u, 889)) return false;
    return push(w, false, 30000);
}

static bool encode_rc6(uint32_t addr, uint32_t cmd, bool toggle, pir_waveform_t *w) {
    /* RC6 mode 0: leader 6T mark, 2T space; start bit then mode/toggle/address/command. */
    if (!push(w,true,2666) || !push(w,false,889)) return false;
    /* RC6 uses Manchester with a 2T toggle bit; represent the standard 16-bit mode-0 frame. */
    uint32_t bits = (1u<<15) | ((toggle?1u:0u)<<12) | ((addr&0xFFu)<<4) | (cmd&0xFu);
    for (int i=15;i>=0;i--) if (!manchester_bit(w,(bits>>i)&1u,444)) return false;
    return push(w,false,30000);
}

bool pir_encode(pir_protocol_t p, uint32_t address, uint32_t command,
                bool repeat, bool toggle, pir_waveform_t *out) {
    if (!out) return false;
    memset(out, 0, sizeof(*out));
    switch (p) {
    case PIR_PROTO_NEC: return encode_nec(address & 0xFFu, command & 0xFFu, repeat, false, out);
    case PIR_PROTO_NEC_EXT: return encode_nec(address & 0xFFFFu, command & 0xFFu, repeat, false, out);
    case PIR_PROTO_SAMSUNG: return encode_nec(address & 0xFFFFu, command & 0xFFu, repeat, true, out);
    case PIR_PROTO_SONY_SIRC12: return encode_sony(address & 0x1Fu, command & 0x7Fu, 12, out);
    case PIR_PROTO_SONY_SIRC15: return encode_sony(address & 0xFFu, command & 0x7Fu, 15, out);
    case PIR_PROTO_SONY_SIRC20: return encode_sony(address & 0x1FFFu, command & 0x7Fu, 20, out);
    case PIR_PROTO_RC5: return encode_rc5(address, command, toggle, out);
    case PIR_PROTO_RC6: return encode_rc6(address, command, toggle, out);
    case PIR_PROTO_PANASONIC: {
        /* Panasonic/Kaseikyo: 16-bit address + 8-bit command + 8-bit extended data.
         * The public API carries address and command; extended data is currently zero. */
        uint32_t data[2] = { address & 0xFFFFu, command & 0xFFu };
        if (!push(out,true,3456) || !push(out,false,1728)) return false;
        for (unsigned i=0;i<16;i++) if (!bit_lsb(out,(data[0]>>i)&1u,432,432,1296)) return false;
        for (unsigned i=0;i<8;i++) if (!bit_lsb(out,(data[1]>>i)&1u,432,432,1296)) return false;
        for (unsigned i=0;i<8;i++) if (!bit_lsb(out,0,432,432,1296)) return false;
        return push(out,true,432) && push(out,false,40000);
    }
    case PIR_PROTO_JVC: {
        if (!push(out,true,8400) || !push(out,false,4200)) return false;
        uint32_t data = ((address & 0xF) << 12) | ((command & 0xFF) << 4);
        for (unsigned i=0;i<16;i++) if (!bit_lsb(out,(data>>i)&1u,525,525,1575)) return false;
        return push(out,true,525) && push(out,false,40000);
    }
    default: return false;
    }
}

static bool mark_space(const pir_edge_t *e, size_t n, size_t i, bool mark, uint32_t expected) {
    return i<n && e[i].mark==mark && near_us(e[i].duration_us, expected, TOLERANCE_PCT);
}

static bool decode_pulse_distance(const pir_edge_t *e, size_t n, size_t start,
                                  uint32_t header_mark, uint32_t header_space,
                                  uint32_t bit_mark, uint32_t zero_space, uint32_t one_space,
                                  unsigned bits, uint64_t *data, size_t *end) {
    if (!mark_space(e,n,start,true,header_mark) || !mark_space(e,n,start+1,false,header_space)) return false;
    uint64_t value=0; size_t i=start+2;
    for (unsigned b=0;b<bits;b++,i+=2) {
        if (!mark_space(e,n,i,true,bit_mark) || i+1>=n || e[i+1].mark) return false;
        bool one = near_us(e[i+1].duration_us,one_space,TOLERANCE_PCT);
        bool zero = near_us(e[i+1].duration_us,zero_space,TOLERANCE_PCT);
        if (!one && !zero) return false;
        if (one) value |= (uint64_t)1u << b;
    }
    if (i<n && e[i].mark && near_us(e[i].duration_us,bit_mark,TOLERANCE_PCT)) i++;
    *data=value; *end=i; return true;
}

static bool decode_sony(const pir_edge_t *e,size_t n,size_t start,unsigned bits,uint64_t *data,size_t *end) {
    if (!mark_space(e,n,start,true,2400) || !mark_space(e,n,start+1,false,600)) return false;
    uint64_t v=0; size_t i=start+2;
    for (unsigned b=0;b<bits;b++,i+=2) {
        if (i+1>=n || !e[i].mark || e[i+1].mark || !near_us(e[i+1].duration_us,600,TOLERANCE_PCT)) return false;
        if (near_us(e[i].duration_us,1200,TOLERANCE_PCT)) v |= (uint64_t)1u<<b;
        else if (!near_us(e[i].duration_us,600,TOLERANCE_PCT)) return false;
    }
    *data=v; *end=i; return true;
}

bool pir_decode(const pir_edge_t *e, size_t n, pir_decode_result_t *out) {
    if (!e || !out || n<4) return false;
    memset(out,0,sizeof(*out));
    for (size_t s=0;s<n;s++) {
        uint64_t d=0; size_t end=0;
        if (mark_space(e,n,s,true,9000) && mark_space(e,n,s+1,false,2250) && mark_space(e,n,s+2,true,560)) {
            out->protocol=PIR_PROTO_NEC; out->repeat=true; out->confidence=950; return true;
        }
        if (decode_pulse_distance(e,n,s,9000,4500,560,560,1690,32,&d,&end)) {
            uint8_t a=(uint8_t)d, ai=(uint8_t)(d>>8), c=(uint8_t)(d>>16), ci=(uint8_t)(d>>24);
            if ((uint8_t)~c != ci) continue;
            out->protocol = ((uint8_t)~a == ai) ? PIR_PROTO_NEC : PIR_PROTO_NEC_EXT;
            out->address = out->protocol==PIR_PROTO_NEC ? a : ((uint32_t)ai<<8)|a;
            out->command=c; out->raw_data=d; out->bit_count=32; out->confidence=930; return true;
        }
        if (decode_pulse_distance(e,n,s,4500,4500,560,560,1690,32,&d,&end)) {
            out->protocol=PIR_PROTO_SAMSUNG; out->address=(uint32_t)(d&0xFFFFu);
            out->command=(uint32_t)((d>>16)&0xFFu); out->raw_data=d; out->bit_count=32; out->confidence=850; return true;
        }
        if (decode_sony(e,n,s,20,&d,&end)) {
            out->protocol=PIR_PROTO_SONY_SIRC20; out->command=(uint32_t)(d&0x7Fu);
            out->address=(uint32_t)(d>>7); out->raw_data=d; out->bit_count=20; out->confidence=900; return true;
        }
        if (decode_sony(e,n,s,15,&d,&end)) {
            out->protocol=PIR_PROTO_SONY_SIRC15; out->command=(uint32_t)(d&0x7Fu);
            out->address=(uint32_t)(d>>7); out->raw_data=d; out->bit_count=15; out->confidence=850; return true;
        }
        if (decode_sony(e,n,s,12,&d,&end)) {
            out->protocol=PIR_PROTO_SONY_SIRC12; out->command=(uint32_t)(d&0x7Fu);
            out->address=(uint32_t)(d>>7); out->raw_data=d; out->bit_count=12; out->confidence=850; return true;
        }
        if (decode_pulse_distance(e,n,s,8400,4200,525,525,1575,16,&d,&end)) {
            out->protocol=PIR_PROTO_JVC; out->address=(uint32_t)((d>>12)&0xFu);
            out->command=(uint32_t)((d>>4)&0xFFu); out->raw_data=d; out->bit_count=16; out->confidence=800; return true;
        }
        if (decode_pulse_distance(e,n,s,3456,1728,432,432,1296,32,&d,&end)) {
            out->protocol=PIR_PROTO_PANASONIC; out->address=(uint32_t)(d&0xFFFFu);
            out->command=(uint32_t)((d>>16)&0xFFu); out->raw_data=d; out->bit_count=32; out->confidence=750; return true;
        }
    }
    return false;
}
