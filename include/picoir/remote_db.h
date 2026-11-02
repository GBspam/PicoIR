#ifndef PICOIR_REMOTE_DB_H
#define PICOIR_REMOTE_DB_H
#include "picoir/ir_protocol.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PIR_DB_MAX_ENTRIES 32u
#define PIR_DB_NAME_MAX 31u
#define PIR_DB_RAW_MAX_EDGES 128u

typedef enum { PIR_ENTRY_DECODED = 1, PIR_ENTRY_RAW = 2 } pir_entry_kind_t;
typedef struct {
    bool used;
    pir_entry_kind_t kind;
    char name[PIR_DB_NAME_MAX + 1u];
    pir_protocol_t protocol;
    uint32_t address, command;
    bool toggle;
    uint32_t carrier_hz;
    uint16_t edge_count;
    pir_edge_t edges[PIR_DB_RAW_MAX_EDGES];
} pir_remote_entry_t;
typedef struct { pir_remote_entry_t entries[PIR_DB_MAX_ENTRIES]; size_t count; } pir_remote_db_t;

typedef bool (*pir_write_fn)(void *context, const char *data, size_t length);
void pir_db_init(pir_remote_db_t *db);
bool pir_db_save_decoded(pir_remote_db_t *db, const char *name, const pir_decode_result_t *decoded);
bool pir_db_save_raw(pir_remote_db_t *db, const char *name, const pir_edge_t *edges, size_t count, uint32_t carrier_hz);
bool pir_db_delete(pir_remote_db_t *db, const char *name);
const pir_remote_entry_t *pir_db_find(const pir_remote_db_t *db, const char *name);
const pir_remote_entry_t *pir_db_at(const pir_remote_db_t *db, size_t index);
/* Stream exporters: callback is called with small chunks; no large output buffer. */
bool pir_db_export_csv(const pir_remote_db_t *db, pir_write_fn write, void *context);
bool pir_db_export_json(const pir_remote_db_t *db, pir_write_fn write, void *context);
#ifdef __cplusplus
}
#endif
#endif
