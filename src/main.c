#include <stdio.h>

#include "soft_timer.h"
#include "intersection_fsm.h"
#include "traffic_logic.h"
#include "event_logger.h"
#include "protocol.h"

/* Small pretty-printers for demo output. */
static const char *state_name(intersection_state_t s) {
    switch (s) {
        case STATE_NORMAL:       return "NORMAL MODE";
        case STATE_HIGH_TRAFFIC: return "HIGH TRAFFIC DETECTED";
        case STATE_EMERGENCY:    return "EMERGENCY MODE";
        case STATE_COOLDOWN:     return "COOLDOWN";
    }
    return "?";
}

static const char *light_name(light_color_t c) {
    switch (c) {
        case LIGHT_RED:          return "RED";
        case LIGHT_YELLOW:       return "YELLOW";
        case LIGHT_GREEN:        return "GREEN";
        case LIGHT_FLASH_YELLOW: return "FLASH_YELLOW";
    }
    return "?";
}

static void print_status(const intersection_ctx_t *ctx) {
    printf("[%s] A=%s  B=%s  alarm=%d  counts=(%u,%u)\n",
           state_name(ctx->state),
           light_name(ctx->lights.dir_a),
           light_name(ctx->lights.dir_b),
           ctx->lights.alarm_active,
           ctx->traffic_count[0], ctx->traffic_count[1]);
}

int main(void) {
    /* 1. Bring up timer manager (not used deeply in this demo, but wired). */
    timer_mgr_t tmgr;
    timer_mgr_init(&tmgr);

    /* 2. Init FSM + logger. */
    intersection_ctx_t ctx;
    intersection_init(&ctx);

    event_logger_t log;
    logger_init(&log);

    traffic_config_t cfg = traffic_default_config();
    uint32_t now = 0;

    puts("=== Smart Intersection Guardian - MVP demo ===");
    print_status(&ctx);

    /* 3. Normal phase rotation. */
    for (int i = 0; i < 2; i++) {
        now += 5000;
        intersection_state_t old = ctx.state;
        intersection_update(&ctx, EVT_TIMEOUT);
        logger_write(&log, LOG_STATE_CHANGE, EVT_TIMEOUT,
                     (uint8_t)old, (uint8_t)ctx.state, now, "phase rotate");
        print_status(&ctx);
    }

    /* 4. Simulate traffic buildup on direction A. */
    ctx.traffic_count[0] = 6;
    ctx.traffic_count[1] = 1;
    traffic_phase_t ph = traffic_compute_phase(&cfg,
                                               ctx.traffic_count[0],
                                               ctx.traffic_count[1]);
    printf("traffic_logic: greenA=%u ms greenB=%u ms highTraffic=%d heavyDir=%u\n",
           ph.green_time_a_ms, ph.green_time_b_ms,
           ph.high_traffic, ph.heavy_direction);

    if (ph.high_traffic) {
        intersection_update(&ctx, EVT_HIGH_TRAFFIC);
        logger_write(&log, LOG_TRAFFIC, EVT_HIGH_TRAFFIC,
                     STATE_NORMAL, ctx.state, now, "high traffic A");
        print_status(&ctx);
    }

    /* 5. Simulate a crash. */
    now += 3000;
    intersection_update(&ctx, EVT_CRASH);
    logger_write(&log, LOG_ALERT, EVT_CRASH,
                 STATE_HIGH_TRAFFIC, ctx.state, now, "ACCIDENT ALERT SENT");
    print_status(&ctx);

    /* 6. Serialize an alert packet (what nRF54L15 would send). */
    alert_payload_t alert = {
        .event_type   = EVT_CRASH,
        .severity     = 3,
        .timestamp_ms = now
    };
    uint8_t buf[32];
    int n = protocol_serialize_alert(&alert, buf, sizeof(buf));
    printf("serialized alert: %d bytes, checksum=0x%02X\n", n, buf[n - 1]);

    /* 7. Deserialize on the "other" board to verify. */
    packet_type_t type;
    alert_payload_t rx = {0};
    int rc = protocol_deserialize(buf, (size_t)n, &type, &rx);
    printf("deserialize rc=%d type=0x%02X event=%u severity=%u\n",
           rc, type, rx.event_type, rx.severity);

    /* 8. Clear emergency, drop through cooldown to normal. */
    now += 5000;
    intersection_update(&ctx, EVT_CLEAR);
    print_status(&ctx);
    now += 2000;
    intersection_update(&ctx, EVT_TIMEOUT);
    print_status(&ctx);

    /* 9. Dump last log entries. */
    log_entry_t last[5];
    uint16_t got = logger_read_last(&log, last, 5);
    puts("--- recent events ---");
    for (uint16_t i = 0; i < got; i++) {
        printf("  t=%u cat=%d evt=%u %u->%u  %s\n",
               last[i].timestamp_ms, last[i].category, last[i].event_code,
               last[i].old_state, last[i].new_state, last[i].msg);
    }
    return 0;
}
