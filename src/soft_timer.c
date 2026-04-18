#include "soft_timer.h"

void timer_mgr_init(timer_mgr_t *mgr) {
    if (!mgr) return;
    for (uint8_t i = 0; i < MAX_TIMERS; i++) {
        mgr->timers[i].timeout_ms = 0;
        mgr->timers[i].elapsed_ms = 0;
        mgr->timers[i].callback   = 0;
        mgr->timers[i].active     = false;
        mgr->timers[i].repeat     = false;
    }
}

int timer_start(timer_mgr_t *mgr, uint8_t id,
                uint32_t timeout_ms,
                timer_callback_t cb, bool repeat) {
    if (!mgr || id >= MAX_TIMERS) return -1;
    soft_timer_t *t = &mgr->timers[id];
    t->timeout_ms = timeout_ms;
    t->elapsed_ms = 0;
    t->callback   = cb;
    t->repeat     = repeat;
    t->active     = true;
    return 0;
}

void timer_stop(timer_mgr_t *mgr, uint8_t id) {
    if (!mgr || id >= MAX_TIMERS) return;
    mgr->timers[id].active = false;
}

void timer_tick(timer_mgr_t *mgr, uint32_t delta_ms) {
    if (!mgr) return;
    for (uint8_t i = 0; i < MAX_TIMERS; i++) {
        soft_timer_t *t = &mgr->timers[i];
        if (!t->active) continue;

        t->elapsed_ms += delta_ms;

        /* Guard against zero-timeout timers creating an infinite loop. */
        if (t->timeout_ms == 0) {
            if (t->callback) t->callback(i);
            t->active = false;
            continue;
        }

        /* Fire repeatedly if delta spans multiple periods. */
        while (t->active && t->elapsed_ms >= t->timeout_ms) {
            if (t->callback) t->callback(i);
            if (t->repeat) {
                t->elapsed_ms -= t->timeout_ms;
            } else {
                t->active = false;
            }
        }
    }
}

uint32_t timer_remaining(const timer_mgr_t *mgr, uint8_t id) {
    if (!mgr || id >= MAX_TIMERS) return 0;
    const soft_timer_t *t = &mgr->timers[id];
    if (!t->active) return 0;
    if (t->elapsed_ms >= t->timeout_ms) return 0;
    return t->timeout_ms - t->elapsed_ms;
}
