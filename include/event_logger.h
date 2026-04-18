#ifndef EVENT_LOGGER_H
#define EVENT_LOGGER_H

#include <stdint.h>

#define LOG_BUFFER_SIZE 64
#define LOG_MSG_LEN     32

/* Log categories — enough to filter on dashboard. */
typedef enum {
    LOG_STATE_CHANGE,
    LOG_ALERT,
    LOG_TRAFFIC,
    LOG_SYSTEM
} log_category_t;

/* One log row. Fixed-size message keeps everything static-friendly. */
typedef struct {
    uint32_t       timestamp_ms;
    log_category_t category;
    uint8_t        event_code;
    uint8_t        old_state;
    uint8_t        new_state;
    char           msg[LOG_MSG_LEN];
} log_entry_t;

/* Ring buffer. O(1) write, overwrites oldest when full. */
typedef struct {
    log_entry_t entries[LOG_BUFFER_SIZE];
    uint16_t    head;   /* next write index */
    uint16_t    count;  /* number of valid entries (<= LOG_BUFFER_SIZE) */
} event_logger_t;

void     logger_init(event_logger_t *log);

void     logger_write(event_logger_t *log, log_category_t cat,
                      uint8_t evt, uint8_t old_st, uint8_t new_st,
                      uint32_t timestamp, const char *msg);

/* Copy up to n most-recent entries (newest first) into `out`.
 * Returns number of entries actually copied. */
uint16_t logger_read_last(const event_logger_t *log,
                          log_entry_t *out, uint16_t n);

void     logger_clear(event_logger_t *log);

#endif /* EVENT_LOGGER_H */
