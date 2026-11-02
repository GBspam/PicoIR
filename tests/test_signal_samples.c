#include "picoir/signal_analysis.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    pir_edge_t edges[8]; size_t count = 0;
    const uint32_t one_word[] = {0xffff0000u};
    assert(pir_samples_to_edges(one_word,1,1000000u,true,edges,8,&count));
    assert(count==2 && edges[0].mark && edges[0].duration_us==16u);
    assert(!edges[1].mark && edges[1].duration_us==16u);
    const uint32_t two_words[] = {0xffffffffu,0x00000000u};
    assert(pir_samples_to_edges(two_words,2,1000000u,true,edges,8,&count));
    assert(count==2 && edges[0].mark && edges[0].duration_us==32u);
    assert(!edges[1].mark && edges[1].duration_us==32u);
    const uint32_t active_low[] = {0x00000000u};
    assert(pir_samples_to_edges(active_low,1,500000u,false,edges,8,&count));
    assert(count==1 && edges[0].mark && edges[0].duration_us==64u);
    assert(!pir_samples_to_edges(one_word,1,0u,true,edges,8,&count));
    assert(!pir_samples_to_edges(one_word,1,2000000u,true,edges,8,&count));
    assert(!pir_samples_to_edges(two_words,2,1000000u,true,edges,1,&count));
    /* Exercise the complete sampled-capture -> edges -> protocol decode seam.
     * Synthetic 1 MHz samples preserve the encoded waveform's microsecond runs. */
    pir_waveform_t waveform;
    pir_decode_result_t decoded;
    static uint32_t packed[4096];
    pir_edge_t decoded_edges[PIR_MAX_EDGES];
    size_t sample_index = 100u; /* leading idle space before the first mark */
    assert(pir_encode(PIR_PROTO_NEC, 0x20u, 0x10u, false, false, &waveform));
    for (size_t i = 0; i < waveform.count; ++i) {
        for (uint32_t j = 0; j < waveform.edges[i].duration_us; ++j) {
            assert(sample_index < sizeof(packed) / sizeof(packed[0]) * 32u);
            if (waveform.edges[i].mark)
                packed[sample_index / 32u] |= 1u << (31u - (unsigned)(sample_index % 32u));
            ++sample_index;
        }
    }
    size_t packed_count = (sample_index + 31u) / 32u;
    size_t decoded_count = 0;
    assert(pir_decode_samples(packed, packed_count, 1000000u, true,
                              decoded_edges, PIR_MAX_EDGES, &decoded_count, &decoded));
    assert(decoded.protocol == PIR_PROTO_NEC);
    assert(decoded.address == 0x20u && decoded.command == 0x10u);
    assert(decoded_count > 60u);
    assert(!pir_decode_samples(NULL, 0, 1000000u, true,
                               decoded_edges, PIR_MAX_EDGES, &decoded_count, &decoded));

    puts("sampled IR edge conversion and decode tests passed");
    return 0;
}
