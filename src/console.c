#include "picoir/console.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool out(const pir_console_io_t *io,const char *s){return io && io->write && io->write(io->write_context,s,strlen(s));}
static bool outf(const pir_console_io_t *io,const char *fmt,const char *a,const char *b,unsigned long x,unsigned long y){char buf[192];int n=snprintf(buf,sizeof buf,fmt,a,b,x,y);return n>=0&&(size_t)n<sizeof buf&&io->write(io->write_context,buf,(size_t)n);}
static int eq(const char *a,const char *b){while(*a&&*b){if(toupper((unsigned char)*a++)!=toupper((unsigned char)*b++))return 0;}return *a==*b;}
static bool parse_u32(const char *s,uint32_t *v){if(!s||!*s||!v||*s=='-')return false;char *end=NULL;unsigned long x=strtoul(s,&end,0);if(!end||*end||x>UINT32_MAX)return false;*v=(uint32_t)x;return true;}
static int hex_value(char c){if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;if(c>='A'&&c<='F')return c-'A'+10;return -1;}
static bool fits_bits(uint32_t value,unsigned bits){return bits<32u&&value<(1u<<bits);}
/* Console execution is serialized; keep the large waveform scratch off the
 * RP2040's small default stack. */
static pir_waveform_t command_waveform;

static bool protocol_args_fit(pir_protocol_t p,uint32_t address,uint32_t command){
 unsigned address_bits=0,command_bits=0;
 switch(p){
 case PIR_PROTO_NEC: address_bits=8;command_bits=8;break;
 case PIR_PROTO_NEC_EXT: address_bits=16;command_bits=8;break;
 case PIR_PROTO_SONY_SIRC12: address_bits=5;command_bits=7;break;
 case PIR_PROTO_SONY_SIRC15: address_bits=8;command_bits=7;break;
 case PIR_PROTO_SONY_SIRC20: address_bits=13;command_bits=7;break;
 case PIR_PROTO_RC5: address_bits=5;command_bits=7;break;
 case PIR_PROTO_RC6: address_bits=8;command_bits=8;break;
 case PIR_PROTO_SAMSUNG: address_bits=16;command_bits=8;break;
 case PIR_PROTO_PANASONIC: address_bits=16;command_bits=8;break;
 case PIR_PROTO_JVC: address_bits=8;command_bits=8;break;
 default: return false;
 }
 return fits_bits(address,address_bits)&&fits_bits(command,command_bits);
}
static bool parse_hex_bytes(const char *s,uint8_t *out,size_t cap,size_t *count){size_t n=strlen(s);if((n&1u)||n/2u>cap)return false;for(size_t i=0;i<n/2u;i++){int hi=hex_value(s[2u*i]),lo=hex_value(s[2u*i+1u]);if(hi<0||lo<0)return false;out[i]=(uint8_t)((hi<<4)|lo);}*count=n/2u;return true;}
static bool print_hex_packet(const pir_console_io_t *io,const uint8_t *bytes,size_t n){char line[2u*PIR_GBC_CONSOLE_PACKET_MAX+16u];int k=snprintf(line,sizeof line,"GB PACKET ");if(k<0)return false;size_t used=(size_t)k;for(size_t i=0;i<n;i++){if(used+2u>=sizeof line)return false;int m=snprintf(line+used,sizeof line-used,"%02X",bytes[i]);if(m!=2)return false;used+=2u;}if(used+1u>=sizeof line)return false;line[used++]='\n';return io->write(io->write_context,line,used);}
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
 if(eq(av[0],"HELP")){return out(io,"HELP | REMOTE LIST | REMOTE SAVE name | REMOTE DELETE name | REMOTE SEND protocol address command [TOGGLE] | REMOTE SEND SAVED name | REMOTE LEARN START|STOP | RAW EXPORT CSV|JSON | GB MODE ON|OFF | GB SEND HEX | GB RECEIVE timeout_ms\n");}
 if(eq(av[0],"REMOTE")&&n>=2){
  if(eq(av[1],"LIST")&&n==2)return cmd_remote_list(db,io);
  if(eq(av[1],"LEARN")&&n==3){if(eq(av[2],"START")){s->learning=true;return out(io,"LEARNING ARMED\n");}if(eq(av[2],"STOP")){s->learning=false;return out(io,"LEARNING STOPPED\n");}}
  if(eq(av[1],"SAVE")&&n==3){if(s->have_decoded){if(!pir_db_save_decoded(db,av[2],&s->decoded))return out(io,"ERROR: cannot save decoded entry\n")&&false;return out(io,"SAVED DECODED REMOTE\n");}if(s->have_capture){if(!pir_db_save_raw(db,av[2],s->capture,s->capture_count,s->capture_carrier_hz))return out(io,"ERROR: cannot save raw capture\n")&&false;return out(io,"SAVED RAW CAPTURE\n");}return out(io,"ERROR: no decoded signal or capture\n")&&false;}
  if(eq(av[1],"DELETE")&&n==3){if(!pir_db_delete(db,av[2]))return out(io,"ERROR: entry not found\n")&&false;return out(io,"DELETED\n");}
  if(eq(av[1],"SEND")&&n==4&&eq(av[2],"SAVED")){const pir_remote_entry_t *e=pir_db_find(db,av[3]);if(!e)return out(io,"ERROR: entry not found\n")&&false;if(e->kind==PIR_ENTRY_RAW){if(!io->transmit_raw||!io->transmit_raw(io->transmit_context,e->edges,e->edge_count,e->carrier_hz))return out(io,"ERROR: raw transmit failed\n")&&false;}else{if(!pir_encode(e->protocol,e->address,e->command,false,e->toggle,&command_waveform)||!io->transmit||!io->transmit(io->transmit_context,&command_waveform))return out(io,"ERROR: transmit failed\n")&&false;}return out(io,"TRANSMIT COMPLETE\n");}
  if(eq(av[1],"SEND")&&(n==5||n==6)){pir_protocol_t p;uint32_t addr,cmd;bool toggle=n==6&&eq(av[5],"TOGGLE");if(n==6&&!toggle)return out(io,"ERROR: expected TOGGLE\n")&&false;if(!pir_protocol_parse(av[2],&p)||!parse_u32(av[3],&addr)||!parse_u32(av[4],&cmd))return out(io,"ERROR: invalid protocol/address/command\n")&&false;if(!protocol_args_fit(p,addr,cmd))return out(io,"ERROR: address/command out of range\n")&&false;if(!pir_encode(p,addr,cmd,false,toggle,&command_waveform))return out(io,"ERROR: encode failed or protocol unsupported\n")&&false;if(!io->transmit||!io->transmit(io->transmit_context,&command_waveform))return out(io,"ERROR: transmitter unavailable or failed\n")&&false;return out(io,"TRANSMIT COMPLETE\n");}
 }
 if(eq(av[0],"GB")&&n>=2){
  if(eq(av[1],"MODE")&&n==3){
   if(eq(av[2],"OFF")){s->gameboy_mode=false;s->gameboy_connected=false;return out(io,"GB MODE OFF\n");}
   if(eq(av[2],"ON")){if(!io->gameboy_transmit_packet||!io->gameboy_receive_packet)return out(io,"GB ERROR BACKEND_UNAVAILABLE\n")&&false;s->gameboy_mode=true;s->gameboy_connected=false;return out(io,"GB MODE ON\n");}
  }
  if(eq(av[1],"SEND")&&n==3){
   if(!s->gameboy_mode)return out(io,"GB ERROR MODE_OFF\n")&&false;
   uint8_t packet[PIR_GBC_CONSOLE_PACKET_MAX];size_t packet_len=0;pir_gbc_packet_t decoded;
   if(!parse_hex_bytes(av[2],packet,sizeof packet,&packet_len)||pir_gbc_packet_decode(packet,packet_len,&decoded)!=PIR_GBC_PACKET_OK)return out(io,"GB ERROR INVALID_PACKET\n")&&false;
   uint8_t status=0xFF;bool first=!s->gameboy_connected;
   if(!io->gameboy_transmit_packet(io->gameboy_context,packet,packet_len,first,&status))return out(io,"GB ERROR TIMEOUT\n")&&false;
   s->gameboy_connected=true;
   if(status==0x00)return out(io,"GB STATUS OK\n");
   if(status==0x01)return out(io,"GB STATUS CHECKSUM_ERROR\n");
   return out(io,"GB ERROR REMOTE_STATUS\n")&&false;
  }
  if(eq(av[1],"RECEIVE")&&n==3){
   if(!s->gameboy_mode)return out(io,"GB ERROR MODE_OFF\n")&&false;
   uint32_t timeout=0;if(!parse_u32(av[2],&timeout)||timeout==0||timeout>60000u)return out(io,"GB ERROR INVALID_TIMEOUT\n")&&false;
   uint8_t packet[PIR_GBC_CONSOLE_PACKET_MAX];size_t packet_len=0;pir_gbc_packet_t decoded;
   if(!io->gameboy_receive_packet(io->gameboy_context,packet,sizeof packet,&packet_len,timeout))return out(io,"GB ERROR TIMEOUT\n")&&false;
   if(pir_gbc_packet_decode(packet,packet_len,&decoded)!=PIR_GBC_PACKET_OK)return out(io,"GB ERROR INVALID_RECEIVED_PACKET\n")&&false;
   return print_hex_packet(io,packet,packet_len);
  }
 }
 if(eq(av[0],"RAW")&&n==3&&eq(av[1],"EXPORT")){if(eq(av[2],"CSV"))return pir_db_export_csv(db,io->write,io->write_context);if(eq(av[2],"JSON"))return pir_db_export_json(db,io->write,io->write_context);}
 return out(io,"ERROR: unknown or malformed command; use HELP\n")&&false;
}
