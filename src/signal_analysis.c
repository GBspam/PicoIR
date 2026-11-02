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
