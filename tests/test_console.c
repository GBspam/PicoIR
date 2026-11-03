#include "picoir/console.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef struct {char text[8192];size_t n;unsigned tx_count;pir_waveform_t last;unsigned gb_tx_count;bool gb_connect[4];uint8_t gb_packet[PIR_GBC_CONSOLE_PACKET_MAX];size_t gb_packet_len;} test_io_t;
static bool wr(void *p,const char *s,size_t n){test_io_t*t=p;if(t->n+n>=sizeof t->text)return false;memcpy(t->text+t->n,s,n);t->n+=n;t->text[t->n]=0;return true;}
static bool tx(void*p,const pir_waveform_t*w){test_io_t*t=p;t->last=*w;t->tx_count++;return true;}
static bool txraw(void*p,const pir_edge_t*e,size_t n,uint32_t c){(void)e;(void)n;(void)c;((test_io_t*)p)->tx_count++;return true;}
static bool gbtx(void*p,const uint8_t*b,size_t n,bool connect,uint8_t*status){test_io_t*t=p;if(n>sizeof t->gb_packet)return false;memcpy(t->gb_packet,b,n);t->gb_packet_len=n;t->gb_connect[t->gb_tx_count<4?t->gb_tx_count:3]=connect;t->gb_tx_count++;*status=0;return true;}
static bool gbrx(void*p,uint8_t*b,size_t cap,size_t*n,uint32_t timeout){test_io_t*t=p;(void)timeout;if(t->gb_packet_len>cap)return false;memcpy(b,t->gb_packet,t->gb_packet_len);*n=t->gb_packet_len;return true;}
int main(void){pir_console_state_t s;pir_remote_db_t db;test_io_t t={0};pir_console_io_t io={.write=wr,.write_context=&t,.transmit=tx,.transmit_raw=txraw,.transmit_context=&t,.gameboy_transmit_packet=gbtx,.gameboy_receive_packet=gbrx,.gameboy_context=&t};pir_console_init(&s);pir_db_init(&db);
 assert(pir_console_execute(&s,&db,&io,"REMOTE SEND NEC 0x20 0x10"));assert(t.tx_count==1);assert(t.last.count>60);
 assert(!pir_console_execute(&s,&db,&io,"REMOTE SEND NEC 0x100 0x10"));assert(t.tx_count==1);assert(strstr(t.text,"ERROR: address/command out of range"));
 assert(!pir_console_execute(&s,&db,&io,"REMOTE SEND SONY_SIRC12 0x20 0x15"));assert(t.tx_count==1);
 pir_decode_result_t d={.protocol=PIR_PROTO_NEC,.address=0x20,.command=0x10,.bit_count=32,.confidence=900};pir_console_set_decoded(&s,&d);
 assert(pir_console_execute(&s,&db,&io,"REMOTE SAVE TV_POWER"));assert(pir_db_find(&db,"tv_power"));
 assert(pir_console_execute(&s,&db,&io,"REMOTE SEND SAVED TV_POWER"));assert(t.tx_count==2);
 assert(pir_console_execute(&s,&db,&io,"REMOTE LIST"));assert(strstr(t.text,"TV_POWER"));
 assert(!pir_console_execute(&s,&db,&io,"REMOTE SEND UNKNOWN 1 2"));
 assert(pir_console_execute(&s,&db,&io,"REMOTE DELETE TV_POWER"));assert(db.count==0);
 assert(pir_gbc_packet_encode((const uint8_t[]){0x7B},1,t.gb_packet,sizeof t.gb_packet,&t.gb_packet_len)==PIR_GBC_PACKET_OK);
 assert(pir_console_execute(&s,&db,&io,"GB MODE ON"));
 char gbline[160];snprintf(gbline,sizeof gbline,"GB SEND %02X%02X%02X%02X%02X",t.gb_packet[0],t.gb_packet[1],t.gb_packet[2],t.gb_packet[3],t.gb_packet[4]);
 assert(pir_console_execute(&s,&db,&io,gbline));assert(t.gb_tx_count==1&&t.gb_connect[0]);
 assert(pir_console_execute(&s,&db,&io,gbline));assert(t.gb_tx_count==2&&!t.gb_connect[1]);
 assert(pir_console_execute(&s,&db,&io,"GB RECEIVE 100"));assert(strstr(t.text,"GB PACKET 5A017B00D6"));
 assert(pir_console_execute(&s,&db,&io,"GB MODE OFF"));
 puts("PicoIR console tests passed");return 0;}
