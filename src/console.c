#include "picoir/console.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool out(const pir_console_io_t *io,const char *s){return io && io->write && io->write(io->write_context,s,strlen(s));}
static bool outf(const pir_console_io_t *io,const char *fmt,const char *a,const char *b,unsigned long x,unsigned long y){char buf[192];int n=snprintf(buf,sizeof buf,fmt,a,b,x,y);return n>=0&&(size_t)n<sizeof buf&&io->write(io->write_context,buf,(size_t)n);}
static int eq(const char *a,const char *b){while(*a&&*b){if(toupper((unsigned char)*a++)!=toupper((unsigned char)*b++))return 0;}return *a==*b;}
static bool parse_u32(const char *s,uint32_t *v){if(!s||!*s||!v||*s=='-')return false;char *end=NULL;unsigned long x=strtoul(s,&end,0);if(!end||*end||x>UINT32_MAX)return false;*v=(uint32_t)x;return true;}
static size_t split(char *s,char *argv[],size_t cap){size_t n=0;while(*s){while(isspace((unsigned char)*s))s++;if(!*s)break;if(n==cap)return cap+1;argv[n++]=s;while(*s&&!isspace((unsigned char)*s))s++;if(*s)*s++='\0';}return n;}
void pir_console_init(pir_console_state_t *s){if(s)memset(s,0,sizeof(*s));}
bool pir_console_set_capture(pir_console_state_t *s,const pir_edge_t *e,size_t n,uint32_t carrier){if(!s||!e||!n||n>PIR_CONSOLE_CAPTURE_MAX)return false;for(size_t i=0;i<n;i++)if(!e[i].duration_us||(i&&e[i].mark==e[i-1].mark))return false;memcpy(s->capture,e,n*sizeof(*e));s->capture_count=(uint16_t)n;s->capture_carrier_hz=carrier;s->have_capture=true;return true;}
void pir_console_set_decoded(pir_console_state_t *s,const pir_decode_result_t *d){if(!s)return;if(!d){s->have_decoded=false;memset(&s->decoded,0,sizeof(s->decoded));return;}s->decoded=*d;s->have_decoded=d->protocol!=PIR_PROTO_UNKNOWN;}
static bool cmd_remote_list(const pir_remote_db_t *db,const pir_console_io_t *io){
 if(!out(io,"NAME\tKIND\tPROTOCOL\tADDRESS\tCOMMAND\tCARRIER\n"))return false;
 for(size_t i=0;i<PIR_DB_MAX_ENTRIES;i++){
  const pir_remote_entry_t *e=pir_db_at(db,i); if(!e)break;
  char row[192]; int n=snprintf(row,sizeof row,"%s\t%s\t%s\t0x%lX\t0x%lX\t%lu\n",e->name,e->kind==PIR_ENTRY_RAW?"RAW":"DECODED",pir_protocol_name(e->protocol),(unsigned long)e->address,(unsigned long)e->command,(unsigned long)e->carrier_hz);
  if(n<0||(size_t)n>=sizeof row||!io->write(io->write_context,row,(size_t)n))return false;
 }
 return true;
}

bool pir_console_execute(pir_console_state_t *s,pir_remote_db_t *db,const pir_console_io_t *io,const char *line){
 if(!s||!db||!io||!io->write||!line)return false;size_t len=strlen(line);if(len>=PIR_CONSOLE_LINE_MAX)return out(io,"ERROR: line too long\n")&&false;
 char buf[PIR_CONSOLE_LINE_MAX];memcpy(buf,line,len+1);char *av[6];size_t n=split(buf,av,6);if(n==0)return true;if(n>6)return out(io,"ERROR: too many arguments\n")&&false;
 if(eq(av[0],"HELP")){return out(io,"HELP | REMOTE LIST | REMOTE SAVE name | REMOTE DELETE name | REMOTE SEND protocol address command [TOGGLE] | REMOTE SEND SAVED name | REMOTE LEARN START|STOP | RAW EXPORT CSV|JSON\n");}
 if(eq(av[0],"REMOTE")&&n>=2){
  if(eq(av[1],"LIST")&&n==2)return cmd_remote_list(db,io);
  if(eq(av[1],"LEARN")&&n==3){if(eq(av[2],"START")){s->learning=true;return out(io,"LEARNING ARMED\n");}if(eq(av[2],"STOP")){s->learning=false;return out(io,"LEARNING STOPPED\n");}}
  if(eq(av[1],"SAVE")&&n==3){if(s->have_decoded){if(!pir_db_save_decoded(db,av[2],&s->decoded))return out(io,"ERROR: cannot save decoded entry\n")&&false;return out(io,"SAVED DECODED REMOTE\n");}if(s->have_capture){if(!pir_db_save_raw(db,av[2],s->capture,s->capture_count,s->capture_carrier_hz))return out(io,"ERROR: cannot save raw capture\n")&&false;return out(io,"SAVED RAW CAPTURE\n");}return out(io,"ERROR: no decoded signal or capture\n")&&false;}
  if(eq(av[1],"DELETE")&&n==3){if(!pir_db_delete(db,av[2]))return out(io,"ERROR: entry not found\n")&&false;return out(io,"DELETED\n");}
  if(eq(av[1],"SEND")&&n==4&&eq(av[2],"SAVED")){const pir_remote_entry_t *e=pir_db_find(db,av[3]);if(!e)return out(io,"ERROR: entry not found\n")&&false;if(e->kind==PIR_ENTRY_RAW){if(!io->transmit_raw||!io->transmit_raw(io->transmit_context,e->edges,e->edge_count,e->carrier_hz))return out(io,"ERROR: raw transmit failed\n")&&false;}else{pir_waveform_t w;if(!pir_encode(e->protocol,e->address,e->command,false,e->toggle,&w)||!io->transmit||!io->transmit(io->transmit_context,&w))return out(io,"ERROR: transmit failed\n")&&false;}return out(io,"TRANSMIT COMPLETE\n");}
  if(eq(av[1],"SEND")&&(n==5||n==6)){pir_protocol_t p;uint32_t addr,cmd;bool toggle=n==6&&eq(av[5],"TOGGLE");if(n==6&&!toggle)return out(io,"ERROR: expected TOGGLE\n")&&false;if(!pir_protocol_parse(av[2],&p)||!parse_u32(av[3],&addr)||!parse_u32(av[4],&cmd))return out(io,"ERROR: invalid protocol/address/command\n")&&false;pir_waveform_t w;if(!pir_encode(p,addr,cmd,false,toggle,&w))return out(io,"ERROR: encode failed or protocol unsupported\n")&&false;if(!io->transmit||!io->transmit(io->transmit_context,&w))return out(io,"ERROR: transmitter unavailable or failed\n")&&false;return out(io,"TRANSMIT COMPLETE\n");}
 }
 if(eq(av[0],"RAW")&&n==3&&eq(av[1],"EXPORT")){if(eq(av[2],"CSV"))return pir_db_export_csv(db,io->write,io->write_context);if(eq(av[2],"JSON"))return pir_db_export_json(db,io->write,io->write_context);}
 return out(io,"ERROR: unknown or malformed command; use HELP\n")&&false;
}
