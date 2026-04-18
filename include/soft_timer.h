#ifndef SOFT_TIMER_H
#define SOFT_TIMER_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_TIMERS 8

typedef void (*timer_callback_t)(uint8_t timer_id);

/* A single software timer slot. */
typedef struct {
    uint32_t         timeout_ms;   /* target duration in ms */
    uint32_t         elapsed_ms;   /* time accumulated since start */
    timer_callback_t callback;     /* fired when elapsed >= timeout */
    bool             active;       /* slot is in use */
    bool             repeat;       /* true = periodic, false = one-shot */
} soft_timer_t;

/* Manager holding a fixed pool of timer slots. */
typedef struct {
    soft_timer_t timers[MAX_TIMERS];
} timer_mgr_t;

void     timer_mgr_init(timer_mgr_t *mgr);

/* Register a timer into slot `id`. Returns 0 on success, -1 on invalid id. */
int      timer_start(timer_mgr_t *mgr, uint8_t id,
                     uint32_t timeout_ms,
                     timer_callback_t cb, bool repeat);

void     timer_stop(timer_mgr_t *mgr, uint8_t id);

/* Advance all active timers by delta_ms, fire callbacks as needed. */
void     timer_tick(timer_mgr_t *mgr, uint32_t delta_ms);

/* Remaining ms for slot id (0 if inactive or expired). */
uint32_t timer_remaining(const timer_mgr_t *mgr, uint8_t id);

#endif /* SOFT_TIMER_H */
