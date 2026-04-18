#include "event_logger.h"

/* Minimal local string copy — avoids pulling <string.h> dependencies
 * into constrained embedded builds. */
static void safe_copy(char *dst, const char *src, uint16_t max_len) {
    uint16_t i = 0;
    if (src) {
        while (i < max_len - 1 && src[i] != '\0') {
            dst[i] = src[i];
            i++;
        }
    }
    dst[i] = '\0';
}

void logger_init(event_logger_t *log) {
    if (!log) return;
    log->head  = 0;
    log->count = 0;
}

void logger_write(event_logger_t *log, log_category_t cat,
                  uint8_t evt, uint8_t old_st, uint8_t new_st,
                  uint32_t timestamp, const char *msg) {
    if (!log) return;

    log_entry_t *e = &log->entries[log->head];
    e->timestamp_ms = timestamp;
    e->category     = cat;
    e->event_code   = evt;
    e->old_state    = old_st;
    e->new_state    = new_st;
    safe_copy(e->msg, msg, LOG_MSG_LEN);

    log->head = (log->head + 1) % LOG_BUFFER_SIZE;
    if (log->count < LOG_BUFFER_SIZE) log->count++;
}

uint16_t logger_read_last(const event_logger_t *log,
                          log_entry_t *out, uint16_t n) {
    if (!log || !out || n == 0) return 0;
    uint16_t want = (n < log->count) ? n : log->count;

    /* Walk backward from the most recent entry. */
    for (uint16_t i = 0; i < want; i++) {
        uint16_t idx = (log->head + LOG_BUFFER_SIZE - 1 - i) % LOG_BUFFER_SIZE;
        out[i] = log->entries[idx];
    }
    return want;
}

void logger_clear(event_logger_t *log) {
    if (!log) return;
    log->head = 0;
    log->count = 0;
}
