#ifndef PICOIR_GAMEBOY_LINK_H
#define PICOIR_GAMEBOY_LINK_H

#include <stdbool.h>
#include <stdint.h>
#include "picoir/gameboy_ir.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PIR_GBC_LINK_IDLE = 0,
    PIR_GBC_LINK_WAIT_ECHO,
    PIR_GBC_LINK_READY,
    PIR_GBC_LINK_RECEIVING,
    PIR_GBC_LINK_COMPLETE,
    PIR_GBC_LINK_FAILED
} pir_gbc_link_state_t;

typedef enum {
    PIR_GBC_LINK_OK = 0,
    PIR_GBC_LINK_INVALID_STATE,
    PIR_GBC_LINK_BAD_PACKET,
    PIR_GBC_LINK_TIMEOUT
} pir_gbc_link_status_t;

typedef enum {
    PIR_GBC_LINK_ACTION_NONE = 0,
    PIR_GBC_LINK_ACTION_SEND_CONNECT,
    PIR_GBC_LINK_ACTION_ACK_PACKET,
    PIR_GBC_LINK_ACTION_NAK_PACKET,
    PIR_GBC_LINK_ACTION_TRANSFER_COMPLETE,
    PIR_GBC_LINK_ACTION_ABORT
} pir_gbc_link_action_t;

typedef struct {
    pir_gbc_link_state_t state;
    uint32_t packets_accepted;
    uint32_t payload_bytes_accepted;
    uint32_t packets_rejected;
} pir_gbc_link_t;

/* Initialize/reset a session to IDLE and clear counters. */
void pir_gbc_link_reset(pir_gbc_link_t *link);
/* Begin connection. Caller must perform the physical pulse and report its echo. */
pir_gbc_link_status_t pir_gbc_link_connect(pir_gbc_link_t *link,
                                           pir_gbc_link_action_t *action);
/* Report that the expected connection echo was observed. */
pir_gbc_link_status_t pir_gbc_link_on_echo(pir_gbc_link_t *link,
                                           pir_gbc_link_action_t *action);
/* Arm receive after connection has been established. */
pir_gbc_link_status_t pir_gbc_link_begin_receive(pir_gbc_link_t *link,
                                                 pir_gbc_link_action_t *action);
/* Decode one raw packet and apply its result to the session. A valid decoded
 * packet is copied to packet_out when non-NULL. Malformed packets are NAKed
 * and counted as rejected; calls outside RECEIVING return INVALID_STATE. */
pir_gbc_link_status_t pir_gbc_link_on_packet_bytes(
    pir_gbc_link_t *link, const uint8_t *bytes, size_t byte_count,
    pir_gbc_packet_t *packet_out, pir_gbc_link_action_t *action);
/* Report one already-decoded packet. Invalid/checksum-failed packets are NAKed. */
pir_gbc_link_status_t pir_gbc_link_on_packet(pir_gbc_link_t *link,
                                             bool framing_valid,
                                             bool checksum_valid,
                                             uint8_t payload_length,
                                             pir_gbc_link_action_t *action);
/* A receive/connection deadline expired; the session becomes FAILED. */
pir_gbc_link_status_t pir_gbc_link_on_timeout(pir_gbc_link_t *link,
                                              pir_gbc_link_action_t *action);
const char *pir_gbc_link_state_name(pir_gbc_link_state_t state);
const char *pir_gbc_link_status_name(pir_gbc_link_status_t status);

#ifdef __cplusplus
}
#endif
#endif
