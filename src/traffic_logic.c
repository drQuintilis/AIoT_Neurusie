#include "traffic_logic.h"

static uint32_t clamp_u32(uint32_t v, uint32_t lo, uint32_t hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

/* Green time = min + (count * bonus), clamped to [min, max].
 * Empty direction only gets the minimum. */
static uint32_t compute_green(const traffic_config_t *cfg, uint8_t count) {
    if (count == 0) return cfg->min_green_ms;
    uint32_t raw = cfg->min_green_ms + (uint32_t)count * cfg->bonus_per_vehicle;
    return clamp_u32(raw, cfg->min_green_ms, cfg->max_green_ms);
}

traffic_config_t traffic_default_config(void) {
    traffic_config_t cfg;
    cfg.min_green_ms        = 5000;
    cfg.max_green_ms        = 30000;
    cfg.bonus_per_vehicle   = 2000;
    cfg.high_traffic_ratio  = 2;
    return cfg;
}

traffic_phase_t traffic_compute_phase(const traffic_config_t *cfg,
                                      uint8_t count_a, uint8_t count_b) {
    traffic_phase_t out = {0};
    if (!cfg) return out;

    out.green_time_a_ms = compute_green(cfg, count_a);
    out.green_time_b_ms = compute_green(cfg, count_b);

    /* High-traffic flag: one side has >= ratio * other side (and >0). */
    uint8_t ratio = cfg->high_traffic_ratio ? cfg->high_traffic_ratio : 2;
    if (count_a > 0 && count_a >= (uint32_t)count_b * ratio) {
        out.high_traffic = true;
        out.heavy_direction = 0;
    } else if (count_b > 0 && count_b >= (uint32_t)count_a * ratio) {
        out.high_traffic = true;
        out.heavy_direction = 1;
    }
    return out;
}
