#include "test_common.h"
#include "protocol.h"

static void test_roundtrip_status(void) {
    status_payload_t s = {
        .state = 1, .light_a = 2, .light_b = 0,
        .count_a = 5, .count_b = 3, .timestamp_ms = 123456
    };
    uint8_t buf[64];
    int n = protocol_serialize_status(&s, buf, sizeof(buf));
    ASSERT_TRUE(n > 0);

    packet_type_t type;
    status_payload_t rx = {0};
    int rc = protocol_deserialize(buf, (size_t)n, &type, &rx);
    ASSERT_EQ(rc, 0);
    ASSERT_EQ(type, PKT_STATUS);
    ASSERT_EQ(rx.state, 1);
    ASSERT_EQ(rx.count_a, 5);
    ASSERT_EQ(rx.timestamp_ms, 123456);
}

static void test_roundtrip_alert(void) {
    alert_payload_t a = { .event_type = 7, .severity = 3, .timestamp_ms = 999 };
    uint8_t buf[64];
    int n = protocol_serialize_alert(&a, buf, sizeof(buf));
    ASSERT_TRUE(n > 0);

    packet_type_t type;
    alert_payload_t rx = {0};
    ASSERT_EQ(protocol_deserialize(buf, (size_t)n, &type, &rx), 0);
    ASSERT_EQ(type, PKT_ALERT);
    ASSERT_EQ(rx.event_type, 7);
    ASSERT_EQ(rx.severity, 3);
}

static void test_bad_checksum_rejected(void) {
    alert_payload_t a = { .event_type = 1, .severity = 1, .timestamp_ms = 0 };
    uint8_t buf[64];
    int n = protocol_serialize_alert(&a, buf, sizeof(buf));
    buf[n - 1] ^= 0xFF;  /* corrupt checksum */

    packet_type_t type;
    alert_payload_t rx;
    int rc = protocol_deserialize(buf, (size_t)n, &type, &rx);
    ASSERT_EQ(rc, -2);
}

static void test_buffer_too_small(void) {
    alert_payload_t a = {0};
    uint8_t buf[2];
    int n = protocol_serialize_alert(&a, buf, sizeof(buf));
    ASSERT_EQ(n, -1);
}

int main(void) {
    RUN_TEST(test_roundtrip_status);
    RUN_TEST(test_roundtrip_alert);
    RUN_TEST(test_bad_checksum_rejected);
    RUN_TEST(test_buffer_too_small);
    TEST_RETURN();
}
