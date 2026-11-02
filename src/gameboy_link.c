#include "picoir/gameboy_link.h"
#include <stddef.h>

static pir_gbc_link_status_t check_args(pir_gbc_link_t *link,
                                        pir_gbc_link_action_t *action) {
    if (link == NULL || action == NULL) return PIR_GBC_LINK_INVALID_STATE;
    *action = PIR_GBC_LINK_ACTION_NONE;
    return PIR_GBC_LINK_OK;
}

void pir_gbc_link_reset(pir_gbc_link_t *link) {
    if (link == NULL) return;
    link->state = PIR_GBC_LINK_IDLE;
    link->packets_accepted = 0;
    link->payload_bytes_accepted = 0;
    link->packets_rejected = 0;
}

pir_gbc_link_status_t pir_gbc_link_connect(pir_gbc_link_t *link,
                                           pir_gbc_link_action_t *action) {
    pir_gbc_link_status_t status = check_args(link, action);
    if (status != PIR_GBC_LINK_OK) return status;
    if (link->state != PIR_GBC_LINK_IDLE && link->state != PIR_GBC_LINK_COMPLETE &&
        link->state != PIR_GBC_LINK_FAILED) return PIR_GBC_LINK_INVALID_STATE;
    link->packets_accepted = link->payload_bytes_accepted = link->packets_rejected = 0;
    link->state = PIR_GBC_LINK_WAIT_ECHO;
    *action = PIR_GBC_LINK_ACTION_SEND_CONNECT;
    return PIR_GBC_LINK_OK;
}

pir_gbc_link_status_t pir_gbc_link_on_echo(pir_gbc_link_t *link,
                                           pir_gbc_link_action_t *action) {
    pir_gbc_link_status_t status = check_args(link, action);
    if (status != PIR_GBC_LINK_OK) return status;
    if (link->state != PIR_GBC_LINK_WAIT_ECHO) return PIR_GBC_LINK_INVALID_STATE;
    link->state = PIR_GBC_LINK_READY;
    return PIR_GBC_LINK_OK;
}

pir_gbc_link_status_t pir_gbc_link_begin_receive(pir_gbc_link_t *link,
                                                 pir_gbc_link_action_t *action) {
    pir_gbc_link_status_t status = check_args(link, action);
    if (status != PIR_GBC_LINK_OK) return status;
    if (link->state != PIR_GBC_LINK_READY) return PIR_GBC_LINK_INVALID_STATE;
    link->state = PIR_GBC_LINK_RECEIVING;
    return PIR_GBC_LINK_OK;
}

pir_gbc_link_status_t pir_gbc_link_on_packet(pir_gbc_link_t *link,
                                             bool framing_valid,
                                             bool checksum_valid,
                                             uint8_t payload_length,
                                             pir_gbc_link_action_t *action) {
    pir_gbc_link_status_t status = check_args(link, action);
    if (status != PIR_GBC_LINK_OK) return status;
    if (link->state != PIR_GBC_LINK_RECEIVING) return PIR_GBC_LINK_INVALID_STATE;
    if (!framing_valid || !checksum_valid) {
        ++link->packets_rejected;
        *action = PIR_GBC_LINK_ACTION_NAK_PACKET;
        return PIR_GBC_LINK_BAD_PACKET;
    }
    if (payload_length == 0) {
        link->state = PIR_GBC_LINK_COMPLETE;
        *action = PIR_GBC_LINK_ACTION_TRANSFER_COMPLETE;
        return PIR_GBC_LINK_OK;
    }
    ++link->packets_accepted;
    link->payload_bytes_accepted += payload_length;
    *action = PIR_GBC_LINK_ACTION_ACK_PACKET;
    return PIR_GBC_LINK_OK;
}

pir_gbc_link_status_t pir_gbc_link_on_timeout(pir_gbc_link_t *link,
                                              pir_gbc_link_action_t *action) {
    pir_gbc_link_status_t status = check_args(link, action);
    if (status != PIR_GBC_LINK_OK) return status;
    if (link->state != PIR_GBC_LINK_WAIT_ECHO && link->state != PIR_GBC_LINK_RECEIVING)
        return PIR_GBC_LINK_INVALID_STATE;
    link->state = PIR_GBC_LINK_FAILED;
    *action = PIR_GBC_LINK_ACTION_ABORT;
    return PIR_GBC_LINK_TIMEOUT;
}

const char *pir_gbc_link_state_name(pir_gbc_link_state_t state) {
    switch (state) {
    case PIR_GBC_LINK_IDLE: return "IDLE";
    case PIR_GBC_LINK_WAIT_ECHO: return "WAIT_ECHO";
    case PIR_GBC_LINK_READY: return "READY";
    case PIR_GBC_LINK_RECEIVING: return "RECEIVING";
    case PIR_GBC_LINK_COMPLETE: return "COMPLETE";
    case PIR_GBC_LINK_FAILED: return "FAILED";
    default: return "UNKNOWN_STATE";
    }
}

const char *pir_gbc_link_status_name(pir_gbc_link_status_t status) {
    switch (status) {
    case PIR_GBC_LINK_OK: return "OK";
    case PIR_GBC_LINK_INVALID_STATE: return "INVALID_STATE";
    case PIR_GBC_LINK_BAD_PACKET: return "BAD_PACKET";
    case PIR_GBC_LINK_TIMEOUT: return "TIMEOUT";
    default: return "UNKNOWN_STATUS";
    }
}
