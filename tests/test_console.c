#include "picoir/console.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef struct {char text[8192];size_t n;unsigned tx_count;pir_waveform_t last;} test_io_t;
static bool wr(void *p,const char *s,size_t n){test_io_t*t=p;if(t->n+n>=sizeof t->text)return false;memcpy(t->text+t->n,s,n);t->n+=n;t->text[t->n]=0;return true;}
static bool tx(void*p,const pir_waveform_t*w){test_io_t*t=p;t->last=*w;t->tx_count++;return true;}
static bool txraw(void*p,const pir_edge_t*e,size_t n,uint32_t c){(void)e;(void)n;(void)c;((test_io_t*)p)->tx_count++;return true;}
int main(void){pir_console_state_t s;pir_remote_db_t db;test_io_t t={0};pir_console_io_t io={wr,&t,tx,txraw,&t};pir_console_init(&s);pir_db_init(&db);
 assert(pir_console_execute(&s,&db,&io,"REMOTE SEND NEC 0x20 0x10"));assert(t.tx_count==1);assert(t.last.count>60);
 pir_decode_result_t d={.protocol=PIR_PROTO_NEC,.address=0x20,.command=0x10,.bit_count=32,.confidence=900};pir_console_set_decoded(&s,&d);
 assert(pir_console_execute(&s,&db,&io,"REMOTE SAVE TV_POWER"));assert(pir_db_find(&db,"tv_power"));
 assert(pir_console_execute(&s,&db,&io,"REMOTE SEND SAVED TV_POWER"));assert(t.tx_count==2);
 assert(pir_console_execute(&s,&db,&io,"REMOTE LIST"));assert(strstr(t.text,"TV_POWER"));
 assert(!pir_console_execute(&s,&db,&io,"REMOTE SEND UNKNOWN 1 2"));
 assert(pir_console_execute(&s,&db,&io,"REMOTE DELETE TV_POWER"));assert(db.count==0);
 puts("PicoIR console tests passed");return 0;}
