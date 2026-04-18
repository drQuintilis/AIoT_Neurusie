#include "protocol.h"

/* Local memcpy to avoid pulling string.h on every target. */
static void raw_copy(void *dst, const void *src, size_t n) {
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    for (size_t i = 0; i < n; i++) d[i] = s[i];
}

uint8_t protocol_checksum(const uint8_t *data, size_t len) {
    uint8_t xor_sum = 0;
    for (size_t i = 0; i < len; i++) xor_sum ^= data[i];
    return xor_sum;
}

/* Shared builder: [header][payload][checksum]. */
static int serialize(uint8_t type, const void *payload, uint8_t payload_len,
                     uint8_t *buf, size_t len) {
    size_t total = (size_t)PROTOCOL_OVERHEAD + payload_len;
    if (!buf || len < total) return -1;

    buf[0] = type;
    buf[1] = payload_len;
    raw_copy(&buf[2], payload, payload_len);
    buf[2 + payload_len] = protocol_checksum(buf, 2 + payload_len);
    return (int)total;
}

int protocol_serialize_status(const status_payload_t *s,
                              uint8_t *buf, size_t len) {
    if (!s) return -1;
    return serialize(PKT_STATUS, s, (uint8_t)sizeof(*s), buf, len);
}

int protocol_serialize_alert(const alert_payload_t *a,
                             uint8_t *buf, size_t len) {
    if (!a) return -1;
    return serialize(PKT_ALERT, a, (uint8_t)sizeof(*a), buf, len);
}

int protocol_deserialize(const uint8_t *buf, size_t len,
                         packet_type_t *type, void *payload) {
    if (!buf || !type || !payload || len < PROTOCOL_OVERHEAD) return -1;

    uint8_t pkt_type    = buf[0];
    uint8_t payload_len = buf[1];
    size_t  total       = (size_t)PROTOCOL_OVERHEAD + payload_len;
    if (len < total) return -1;

    /* Validate checksum over header + payload. */
    uint8_t expected = protocol_checksum(buf, 2 + payload_len);
    if (expected != buf[2 + payload_len]) return -2;

    *type = (packet_type_t)pkt_type;
    raw_copy(payload, &buf[2], payload_len);
    return 0;
}
