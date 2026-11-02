#include "picoir/signal_analysis.h"
#include <string.h>
#include <stdlib.h>

bool pir_measure_carrier_periods(const uint32_t *p, size_t n, pir_carrier_result_t *out) {
    if (!p || !out || n < 4) return false;
    uint32_t v[256]; if (n > 256) n=256;
    size_t used=0; for (size_t i=0;i<n;i++) if (p[i]>=1000 && p[i]<=100000) v[used++]=p[i];
    if (used<4) return false;
    for (size_t i=1;i<used;i++) { uint32_t x=v[i]; size_t j=i; while(j && v[j-1]>x) {v[j]=v[j-1];j--;} v[j]=x; }
    uint32_t med=v[used/2]; if (!med) return false;
    size_t inliers=0; for (size_t i=0;i<used;i++) { uint32_t d=v[i]>med?v[i]-med:med-v[i]; if ((uint64_t)d*100u <= (uint64_t)med*20u) inliers++; }
    uint64_t hz=1000000000ull/med; if (hz<1000 || hz>1000000) return false;
    out->frequency_hz=(uint32_t)hz; out->confidence=(uint16_t)((inliers*1000u)/used); out->period_count=(uint32_t)used; return true;
}
bool pir_analyze_capture(const pir_edge_t *e, size_t n, pir_decode_result_t *d, uint64_t *duration) {
    if (!e || !n || !d || !duration) return false;
    uint64_t total=0; for(size_t i=0;i<n;i++) { if (!e[i].duration_us || UINT64_MAX-total<e[i].duration_us) return false; total+=e[i].duration_us; }
    *duration=total; memset(d,0,sizeof(*d)); return pir_decode(e,n,d);
}


bool pir_samples_to_edges(const uint32_t *words, size_t word_count,
                          uint32_t sample_rate_hz, bool mark_level,
                          pir_edge_t *edges, size_t edge_capacity,
                          size_t *edge_count) {
    if (!words || word_count == 0 || sample_rate_hz == 0 ||
        sample_rate_hz > 1000000u || !edges || edge_capacity == 0 || !edge_count)
        return false;
    size_t produced = 0;
    uint64_t run_samples = 0;
    bool run_mark = (((words[0] >> 31) & 1u) != 0u) == mark_level;
    if ((uint64_t)word_count > UINT64_MAX / 32u / 1000000u) return false;
    const uint64_t total_samples = (uint64_t)word_count * 32u;
    for (uint64_t i = 0; i < total_samples; ++i) {
        size_t word_index = (size_t)(i / 32u);
        unsigned bit_index = 31u - (unsigned)(i % 32u);
        bool raw_level = ((words[word_index] >> bit_index) & 1u) != 0u;
        bool is_mark = raw_level == mark_level;
        if (is_mark == run_mark) { ++run_samples; continue; }
        if (produced >= edge_capacity) return false;
        uint64_t duration = (run_samples * 1000000ull + sample_rate_hz / 2u) / sample_rate_hz;
        if (duration == 0) duration = 1;
        if (duration > UINT32_MAX) return false;
        edges[produced].duration_us = (uint32_t)duration;
        edges[produced].mark = run_mark;
        ++produced;
        run_mark = is_mark;
        run_samples = 1;
    }
    if (produced >= edge_capacity) return false;
    uint64_t duration = (run_samples * 1000000ull + sample_rate_hz / 2u) / sample_rate_hz;
    if (duration == 0) duration = 1;
    if (duration > UINT32_MAX) return false;
    edges[produced].duration_us = (uint32_t)duration;
    edges[produced].mark = run_mark;
    ++produced;
    *edge_count = produced;
    return true;
}


bool pir_decode_samples(const uint32_t *sample_words, size_t word_count,
                        uint32_t sample_rate_hz, bool mark_level,
                        pir_edge_t *edges, size_t edge_capacity,
                        size_t *edge_count, pir_decode_result_t *decoded) {
    if (!decoded || !pir_samples_to_edges(sample_words, word_count,
                                           sample_rate_hz, mark_level,
                                           edges, edge_capacity, edge_count))
        return false;
    return pir_decode(edges, *edge_count, decoded);
}
