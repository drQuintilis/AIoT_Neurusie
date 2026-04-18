#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stddef.h>

/* Packet types on the wire between nRF54L15 (controller)
 * and nRF7002 (gateway/dashboard). */
typedef enum {
    PKT_STATUS = 0x01,
    PKT_ALERT  = 0x02,
    PKT_ACK    = 0x03
} packet_type_t;

/* Every packet starts with a 2-byte header: type + payload length. */
typedef struct __attribute__((packed)) {
    uint8_t type;
    uint8_t length;
} packet_header_t;

/* Periodic status snapshot — sent ~every second. */
typedef struct __attribute__((packed)) {
    uint8_t  state;        /* intersection_state_t */
    uint8_t  light_a;      /* light_color_t */
    uint8_t  light_b;
    uint8_t  count_a;
    uint8_t  count_b;
    uint32_t timestamp_ms;
} status_payload_t;

/* Alert fired immediately on event (e.g. crash). */
typedef struct __attribute__((packed)) {
    uint8_t  event_type;
    uint8_t  severity;     /* 0-3 */
    uint32_t timestamp_ms;
} alert_payload_t;

/* Overhead = header (2) + checksum (1). */
#define PROTOCOL_OVERHEAD 3

/* Serialize functions: write header + payload + checksum into `buf`.
 * Return number of bytes written, or -1 if `buf` is too small. */
int protocol_serialize_status(const status_payload_t *s,
                              uint8_t *buf, size_t len);
int protocol_serialize_alert (const alert_payload_t  *a,
                              uint8_t *buf, size_t len);

/* Deserialize: validate checksum, extract type, copy payload into `*payload`
 * (caller provides a pointer to the correct struct).
 * Returns 0 on success, -1 on bad length, -2 on bad checksum. */
int protocol_deserialize(const uint8_t *buf, size_t len,
                         packet_type_t *type, void *payload);

/* XOR-based checksum over `len` bytes. Cheap but catches most corruption. */
uint8_t protocol_checksum(const uint8_t *data, size_t len);

#endif /* PROTOCOL_H */
