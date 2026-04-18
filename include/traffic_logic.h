#ifndef TRAFFIC_LOGIC_H
#define TRAFFIC_LOGIC_H

#include <stdint.h>
#include <stdbool.h>

/* Tunable parameters — tweak live during the hackathon without rebuilds. */
typedef struct {
    uint32_t min_green_ms;       /* e.g. 5000  */
    uint32_t max_green_ms;       /* e.g. 30000 */
    uint32_t bonus_per_vehicle;  /* e.g. 2000 ms per waiting vehicle */
    uint8_t  high_traffic_ratio; /* e.g. 2 means 2x imbalance flips flag */
} traffic_config_t;

/* Result of one phase computation. */
typedef struct {
    uint32_t green_time_a_ms;
    uint32_t green_time_b_ms;
    bool     high_traffic;
    uint8_t  heavy_direction;    /* 0 = A, 1 = B (only valid if high_traffic) */
} traffic_phase_t;

/* Sensible defaults. */
traffic_config_t traffic_default_config(void);

/* Pure function: config + per-direction vehicle counts -> phase times. */
traffic_phase_t  traffic_compute_phase(const traffic_config_t *cfg,
                                       uint8_t count_a, uint8_t count_b);

#endif /* TRAFFIC_LOGIC_H */
