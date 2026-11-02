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
    puts("sampled IR edge conversion tests passed");
    return 0;
}
