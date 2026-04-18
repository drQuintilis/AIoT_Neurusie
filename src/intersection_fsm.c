#include "intersection_fsm.h"

/* Apply light configuration for a given state.
 * Convention: in NORMAL/HIGH_TRAFFIC direction A starts green, B red.
 * The traffic_logic / timer layer will later rotate phases. */
static void apply_lights(intersection_ctx_t *ctx) {
    switch (ctx->state) {
        case STATE_NORMAL:
            ctx->lights.dir_a = LIGHT_GREEN;
            ctx->lights.dir_b = LIGHT_RED;
            ctx->lights.alarm_active = false;
            break;
        case STATE_HIGH_TRAFFIC:
            /* Heavier direction gets green; pick the larger counter. */
            if (ctx->traffic_count[0] >= ctx->traffic_count[1]) {
                ctx->lights.dir_a = LIGHT_GREEN;
                ctx->lights.dir_b = LIGHT_RED;
            } else {
                ctx->lights.dir_a = LIGHT_RED;
                ctx->lights.dir_b = LIGHT_GREEN;
            }
            ctx->lights.alarm_active = false;
            break;
        case STATE_EMERGENCY:
            ctx->lights.dir_a = LIGHT_RED;
            ctx->lights.dir_b = LIGHT_RED;
            ctx->lights.alarm_active = true;
            break;
        case STATE_COOLDOWN:
            ctx->lights.dir_a = LIGHT_FLASH_YELLOW;
            ctx->lights.dir_b = LIGHT_FLASH_YELLOW;
            ctx->lights.alarm_active = false;
            break;
    }
}

void intersection_init(intersection_ctx_t *ctx) {
    if (!ctx) return;
    ctx->state = STATE_NORMAL;
    ctx->phase_elapsed_ms = 0;
    ctx->traffic_count[0] = 0;
    ctx->traffic_count[1] = 0;
    apply_lights(ctx);
}

intersection_state_t intersection_update(intersection_ctx_t *ctx,
                                         intersection_event_t evt) {
    if (!ctx) return STATE_NORMAL;

    /* CRASH from any state wins — go to EMERGENCY immediately. */
    if (evt == EVT_CRASH) {
        ctx->state = STATE_EMERGENCY;
        ctx->phase_elapsed_ms = 0;
        apply_lights(ctx);
        return ctx->state;
    }

    switch (ctx->state) {
        case STATE_NORMAL:
            switch (evt) {
                case EVT_HIGH_TRAFFIC:
                    ctx->state = STATE_HIGH_TRAFFIC;
                    ctx->phase_elapsed_ms = 0;
                    break;
                case EVT_TIMEOUT:
                    /* Rotate which direction is green. */
                    if (ctx->lights.dir_a == LIGHT_GREEN) {
                        ctx->lights.dir_a = LIGHT_RED;
                        ctx->lights.dir_b = LIGHT_GREEN;
                    } else {
                        ctx->lights.dir_a = LIGHT_GREEN;
                        ctx->lights.dir_b = LIGHT_RED;
                    }
                    ctx->phase_elapsed_ms = 0;
                    return ctx->state;  /* skip full re-apply */
                case EVT_VEHICLE_DETECTED:
                default:
                    /* No state change; traffic_logic handles counts. */
                    break;
            }
            break;

        case STATE_HIGH_TRAFFIC:
            switch (evt) {
                case EVT_TIMEOUT:
                    /* Heavy phase ended — back to NORMAL cycle. */
                    ctx->state = STATE_NORMAL;
                    ctx->phase_elapsed_ms = 0;
                    break;
                case EVT_VEHICLE_DETECTED:
                case EVT_HIGH_TRAFFIC:
                default:
                    break;
            }
            break;

        case STATE_EMERGENCY:
            if (evt == EVT_CLEAR) {
                ctx->state = STATE_COOLDOWN;
                ctx->phase_elapsed_ms = 0;
            }
            break;

        case STATE_COOLDOWN:
            if (evt == EVT_TIMEOUT) {
                ctx->state = STATE_NORMAL;
                ctx->phase_elapsed_ms = 0;
            }
            break;
    }

    apply_lights(ctx);
    return ctx->state;
}
