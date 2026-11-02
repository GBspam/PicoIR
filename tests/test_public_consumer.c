#include "picoir/capture_frame.h"
#include "picoir/console.h"
#include "picoir/gameboy_ir.h"
#include "picoir/gameboy_link.h"
#include "picoir/ir_protocol.h"
#include "picoir/ir_timing.h"
#include "picoir/remote_db.h"
#include "picoir/signal_analysis.h"

#include <assert.h>
#include <stdio.h>

int main(void) {
    pir_console_state_t console;
    pir_remote_db_t database;
    pir_protocol_t protocol = PIR_PROTO_UNKNOWN;

    pir_console_init(&console);
    pir_db_init(&database);
    assert(pir_protocol_parse("JVC", &protocol));
    assert(protocol == PIR_PROTO_JVC);
    assert(pir_protocol_name(protocol) != NULL);
    assert(database.count == 0);
    puts("PicoIR public-library consumer test passed");
    return 0;
}
