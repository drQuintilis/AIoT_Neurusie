#ifndef INTERSECTION_FSM_H
#define INTERSECTION_FSM_H

#include <stdint.h>
#include <stdbool.h>

/* Overall intersection state. */
typedef enum {
    STATE_NORMAL,        /* standard cycle, switches on timers */
    STATE_HIGH_TRAFFIC,  /* one direction has heavier load, longer green */
    STATE_EMERGENCY,     /* crash detected, all red, alarm on */
    STATE_COOLDOWN       /* recovering, flashing yellow before NORMAL */
} intersection_state_t;

/* Incoming events that may drive a transition. */
typedef enum {
    EVT_VEHICLE_DETECTED, /* sensor fired on direction A or B */
    EVT_CRASH,            /* shock / emergency button */
    EVT_TIMEOUT,          /* phase timer expired */
    EVT_CLEAR,            /* operator/timer clears emergency */
    EVT_HIGH_TRAFFIC      /* traffic_logic reports overload */
} intersection_event_t;

/* Light color for a single direction. */
typedef enum {
    LIGHT_RED,
    LIGHT_YELLOW,
    LIGHT_GREEN,
    LIGHT_FLASH_YELLOW
} light_color_t;

/* Current light output for both directions. */
typedef struct {
    light_color_t dir_a;
    light_color_t dir_b;
    bool          alarm_active;
} light_config_t;

/* Full system context — no globals, everything lives here. */
typedef struct {
    intersection_state_t state;
    light_config_t       lights;
    uint32_t             phase_elapsed_ms;
    uint8_t              traffic_count[2]; /* [0]=dir A, [1]=dir B */
} intersection_ctx_t;

/* Initialize context to default NORMAL state. */
void intersection_init(intersection_ctx_t *ctx);

/* Process one event; updates ctx->state and ctx->lights in place,
 * returns the new state for convenience. */
intersection_state_t intersection_update(intersection_ctx_t *ctx,
                                         intersection_event_t evt);

#endif /* INTERSECTION_FSM_H */
