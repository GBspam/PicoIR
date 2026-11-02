#include "picoir/gameboy_link.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
    pir_gbc_link_t link;
    pir_gbc_link_action_t action;
    pir_gbc_link_reset(&link);
    assert(link.state == PIR_GBC_LINK_IDLE);
    assert(pir_gbc_link_begin_receive(&link, &action) == PIR_GBC_LINK_INVALID_STATE);

    assert(pir_gbc_link_connect(&link, &action) == PIR_GBC_LINK_OK);
    assert(action == PIR_GBC_LINK_ACTION_SEND_CONNECT);
    assert(link.state == PIR_GBC_LINK_WAIT_ECHO);
    assert(pir_gbc_link_on_echo(&link, &action) == PIR_GBC_LINK_OK);
    assert(link.state == PIR_GBC_LINK_READY);
    assert(pir_gbc_link_begin_receive(&link, &action) == PIR_GBC_LINK_OK);

    assert(pir_gbc_link_on_packet(&link, true, false, 3, &action) == PIR_GBC_LINK_BAD_PACKET);
    assert(action == PIR_GBC_LINK_ACTION_NAK_PACKET);
    assert(link.state == PIR_GBC_LINK_RECEIVING);
    assert(link.packets_rejected == 1);
    assert(link.packets_accepted == 0);

    assert(pir_gbc_link_on_packet(&link, true, true, 3, &action) == PIR_GBC_LINK_OK);
    assert(action == PIR_GBC_LINK_ACTION_ACK_PACKET);
    assert(link.packets_accepted == 1 && link.payload_bytes_accepted == 3);
    assert(pir_gbc_link_on_packet(&link, false, true, 1, &action) == PIR_GBC_LINK_BAD_PACKET);
    assert(action == PIR_GBC_LINK_ACTION_NAK_PACKET);
    assert(link.packets_rejected == 2);

    assert(pir_gbc_link_on_packet(&link, true, true, 0, &action) == PIR_GBC_LINK_OK);
    assert(action == PIR_GBC_LINK_ACTION_TRANSFER_COMPLETE);
    assert(link.state == PIR_GBC_LINK_COMPLETE);
    assert(pir_gbc_link_on_packet(&link, true, true, 1, &action) == PIR_GBC_LINK_INVALID_STATE);

    assert(pir_gbc_link_connect(&link, &action) == PIR_GBC_LINK_OK);
    assert(pir_gbc_link_on_timeout(&link, &action) == PIR_GBC_LINK_TIMEOUT);
    assert(action == PIR_GBC_LINK_ACTION_ABORT && link.state == PIR_GBC_LINK_FAILED);
    assert(pir_gbc_link_connect(&link, &action) == PIR_GBC_LINK_OK);
    assert(link.packets_accepted == 0 && link.payload_bytes_accepted == 0 && link.packets_rejected == 0);
    assert(pir_gbc_link_on_echo(&link, &action) == PIR_GBC_LINK_OK);
    assert(pir_gbc_link_begin_receive(&link, &action) == PIR_GBC_LINK_OK);
    assert(pir_gbc_link_on_timeout(&link, &action) == PIR_GBC_LINK_TIMEOUT);
    assert(link.state == PIR_GBC_LINK_FAILED);

    assert(pir_gbc_link_on_echo(NULL, &action) == PIR_GBC_LINK_INVALID_STATE);
    assert(pir_gbc_link_on_echo(&link, NULL) == PIR_GBC_LINK_INVALID_STATE);
    puts("Game Boy link state machine tests passed");
    return 0;
}
