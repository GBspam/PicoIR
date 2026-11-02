#include "picoir/remote_db.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

static bool valid_name(const char *s) {
    if (!s || !*s || strlen(s) > PIR_DB_NAME_MAX) return false;
    for (; *s; ++s) if (!(isalnum((unsigned char)*s) || *s == '_' || *s == '-' || *s == '.')) return false;
    return true;
}
static bool same_name(const char *a, const char *b) {
    while (*a && *b) { if (tolower((unsigned char)*a++) != tolower((unsigned char)*b++)) return false; }
    return *a == *b;
}
static pir_remote_entry_t *slot_for(pir_remote_db_t *db, const char *name) {
    for (size_t i=0;i<PIR_DB_MAX_ENTRIES;i++) if (db->entries[i].used && same_name(db->entries[i].name,name)) return &db->entries[i];
    for (size_t i=0;i<PIR_DB_MAX_ENTRIES;i++) if (!db->entries[i].used) return &db->entries[i];
    return NULL;
}
void pir_db_init(pir_remote_db_t *db) { if (db) memset(db,0,sizeof(*db)); }
static void init_entry(pir_remote_db_t *db, pir_remote_entry_t *e, const char *name, pir_entry_kind_t kind) {
    if (!e->used) db->count++;
    memset(e,0,sizeof(*e)); e->used=true; e->kind=kind; strncpy(e->name,name,PIR_DB_NAME_MAX); e->name[PIR_DB_NAME_MAX]='\0';
}
bool pir_db_save_decoded(pir_remote_db_t *db, const char *name, const pir_decode_result_t *d) {
    if (!db || !d || d->protocol == PIR_PROTO_UNKNOWN || !valid_name(name)) return false;
    pir_remote_entry_t *e=slot_for(db,name); if (!e) return false;
    init_entry(db,e,name,PIR_ENTRY_DECODED); e->protocol=d->protocol; e->address=d->address; e->command=d->command; e->toggle=d->toggle; e->carrier_hz=d->carrier_hz; return true;
}
bool pir_db_save_raw(pir_remote_db_t *db, const char *name, const pir_edge_t *edges, size_t count, uint32_t carrier_hz) {
    if (!db || !valid_name(name) || !edges || count == 0 || count > PIR_DB_RAW_MAX_EDGES) return false;
    for (size_t i=0;i<count;i++) if (!edges[i].duration_us || (i && edges[i].mark == edges[i-1].mark)) return false;
    pir_remote_entry_t *e=slot_for(db,name); if (!e) return false;
    init_entry(db,e,name,PIR_ENTRY_RAW); e->carrier_hz=carrier_hz; e->edge_count=(uint16_t)count; memcpy(e->edges,edges,count*sizeof(*edges)); return true;
}
bool pir_db_delete(pir_remote_db_t *db, const char *name) {
    if (!db || !valid_name(name)) return false;
    for (size_t i=0;i<PIR_DB_MAX_ENTRIES;i++) if (db->entries[i].used && same_name(db->entries[i].name,name)) { memset(&db->entries[i],0,sizeof(db->entries[i])); db->count--; return true; }
    return false;
}
const pir_remote_entry_t *pir_db_find(const pir_remote_db_t *db, const char *name) {
    if (!db || !valid_name(name)) return NULL;
    for (size_t i=0;i<PIR_DB_MAX_ENTRIES;i++) if (db->entries[i].used && same_name(db->entries[i].name,name)) return &db->entries[i];
    return NULL;
}
const pir_remote_entry_t *pir_db_at(const pir_remote_db_t *db, size_t index) {
    if (!db) return NULL;
    for (size_t i=0;i<PIR_DB_MAX_ENTRIES;i++) if (db->entries[i].used) { if (!index) return &db->entries[i]; index--; }
    return NULL;
}
static bool emit(pir_write_fn w, void *ctx, const char *s) { return w(ctx,s,strlen(s)); }
bool pir_db_export_csv(const pir_remote_db_t *db, pir_write_fn w, void *ctx) {
    if (!db || !w || !emit(w,ctx,"name,kind,protocol,address,command,carrier_hz,edge_count\r\n")) return false;
    for (size_t i=0;i<PIR_DB_MAX_ENTRIES;i++) { const pir_remote_entry_t *e=&db->entries[i]; if (!e->used) continue;
        char b[192]; int n=snprintf(b,sizeof b,"%s,%s,%s,0x%llX,0x%llX,%llu,%u\r\n",e->name,e->kind==PIR_ENTRY_RAW?"RAW":"DECODED",pir_protocol_name(e->protocol),(unsigned long long)e->address,(unsigned long long)e->command,(unsigned long long)e->carrier_hz,(unsigned)e->edge_count);
        if (n < 0 || (size_t)n >= sizeof b || !w(ctx,b,(size_t)n)) return false;
    } return true;
}
static bool json_string(pir_write_fn w, void *ctx, const char *s) {
    if (!emit(w,ctx,"\"")) return false;
    for (;*s;s++) { char b[2]={0,0}; if (*s=='\"' || *s=='\\') { b[0]='\\'; b[1]=*s; if (!w(ctx,b,2)) return false; } else if ((unsigned char)*s < 0x20) { if (!emit(w,ctx,"?")) return false; } else { b[0]=*s; if (!w(ctx,b,1)) return false; } }
    return emit(w,ctx,"\"");
}
bool pir_db_export_json(const pir_remote_db_t *db, pir_write_fn w, void *ctx) {
    if (!db || !w || !emit(w,ctx,"{\"entries\":[")) return false; bool first=true;
    for (size_t i=0;i<PIR_DB_MAX_ENTRIES;i++) { const pir_remote_entry_t *e=&db->entries[i]; if (!e->used) continue;
        if (!first && !emit(w,ctx,",")) return false; first=false;
        if (!emit(w,ctx,"{\"name\":" ) || !json_string(w,ctx,e->name) || !emit(w,ctx,",\"kind\":" ) || !json_string(w,ctx,e->kind==PIR_ENTRY_RAW?"RAW":"DECODED") || !emit(w,ctx,",\"protocol\":" ) || !json_string(w,ctx,pir_protocol_name(e->protocol))) return false;
        char b[160]; int n=snprintf(b,sizeof b,",\"address\":%lu,\"command\":%lu,\"toggle\":%s,\"carrier_hz\":%lu,\"edges\":[",(unsigned long)e->address,(unsigned long)e->command,e->toggle?"true":"false",(unsigned long)e->carrier_hz); if (n<0 || (size_t)n>=sizeof b || !w(ctx,b,(size_t)n)) return false;
        for (size_t j=0;j<e->edge_count;j++) { n=snprintf(b,sizeof b,"%s{\"mark\":%s,\"duration_us\":%lu}",j?",":"",e->edges[j].mark?"true":"false",(unsigned long)e->edges[j].duration_us); if (n<0 || (size_t)n>=sizeof b || !w(ctx,b,(size_t)n)) return false; }
        if (!emit(w,ctx,"]}")) return false;
    } return emit(w,ctx,"]}");
}
