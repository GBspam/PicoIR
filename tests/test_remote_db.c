#include "picoir/remote_db.h"
#include "picoir/signal_analysis.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef struct { char b[8192]; size_t n; } sink_t;
static bool sink(void *p,const char *s,size_t n){ sink_t *x=p; if(x->n+n>=sizeof x->b)return false; memcpy(x->b+x->n,s,n);x->n+=n;x->b[x->n]=0;return true; }
int main(void){
 pir_remote_db_t db; pir_db_init(&db); pir_waveform_t w; pir_decode_result_t d;
 assert(pir_encode(PIR_PROTO_NEC,0x20,0x10,false,false,&w)); assert(pir_decode(w.edges,w.count,&d));
 assert(pir_db_save_decoded(&db,"TV_POWER",&d)); assert(pir_db_find(&db,"tv_power"));
 assert(!pir_db_save_decoded(&db,"bad name",&d));
 pir_edge_t raw[]={{9000,true},{4500,false},{560,true},{560,false}};
 assert(pir_db_save_raw(&db,"RAW_TEST",raw,4,38000)); assert(db.count==2);
 sink_t s={0}; assert(pir_db_export_csv(&db,sink,&s)); assert(strstr(s.b,"TV_POWER,DECODED,NEC"));
 memset(&s,0,sizeof s); assert(pir_db_export_json(&db,sink,&s)); assert(strstr(s.b,"\"name\":\"RAW_TEST\""));
 assert(pir_db_delete(&db,"TV_POWER") && db.count==1);
 uint32_t periods[]={26300,26320,26310,50000,26290,26305}; pir_carrier_result_t c;
 assert(pir_measure_carrier_periods(periods,6,&c)); assert(c.frequency_hz>37000 && c.frequency_hz<39000);
 puts("PicoIR database/analyzer tests passed"); return 0;
}
