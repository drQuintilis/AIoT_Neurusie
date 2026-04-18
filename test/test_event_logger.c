#include "test_common.h"
#include "event_logger.h"

static void test_write_and_read_order(void) {
    event_logger_t log;
    logger_init(&log);
    logger_write(&log, LOG_STATE_CHANGE, 1, 0, 0, 100, "first");
    logger_write(&log, LOG_STATE_CHANGE, 2, 0, 0, 200, "second");
    logger_write(&log, LOG_STATE_CHANGE, 3, 0, 0, 300, "third");

    log_entry_t out[3];
    uint16_t n = logger_read_last(&log, out, 3);
    ASSERT_EQ(n, 3);
    ASSERT_EQ(out[0].event_code, 3);   /* newest first */
    ASSERT_EQ(out[1].event_code, 2);
    ASSERT_EQ(out[2].event_code, 1);
}

static void test_ring_overwrites_oldest(void) {
    event_logger_t log;
    logger_init(&log);
    for (int i = 0; i < LOG_BUFFER_SIZE + 5; i++) {
        logger_write(&log, LOG_SYSTEM, (uint8_t)i, 0, 0, (uint32_t)i, "x");
    }
    ASSERT_EQ(log.count, LOG_BUFFER_SIZE);

    log_entry_t out[1];
    logger_read_last(&log, out, 1);
    /* newest should be the last one written (index 68 on 64-slot buffer) */
    ASSERT_EQ(out[0].event_code, (uint8_t)(LOG_BUFFER_SIZE + 4));
}

static void test_clear_resets(void) {
    event_logger_t log;
    logger_init(&log);
    logger_write(&log, LOG_SYSTEM, 1, 0, 0, 0, "a");
    logger_clear(&log);
    ASSERT_EQ(log.count, 0);
    log_entry_t out[1];
    ASSERT_EQ(logger_read_last(&log, out, 1), 0);
}

int main(void) {
    RUN_TEST(test_write_and_read_order);
    RUN_TEST(test_ring_overwrites_oldest);
    RUN_TEST(test_clear_resets);
    TEST_RETURN();
}
