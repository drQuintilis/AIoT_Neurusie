#ifndef NRF54L15_INTERSECTION_POLICY_H
#define NRF54L15_INTERSECTION_POLICY_H

#include <stdint.h>

#define NRF54L15_INTERSECTION_POLICY_FEATURE_COUNT 9
#define NRF54L15_INTERSECTION_POLICY_ACTION_COUNT 6
#define NRF54L15_INTERSECTION_POLICY_WEIGHT_SCALE 56

static const int16_t nrf54l15_intersection_policy_action_scales[6] = { 65, 65, 65, 65, 65, 65 };
static const int8_t nrf54l15_intersection_policy_weights[10][6] = {
    { -14, -10, -15, -14, -9, -16 },
    { -14, -11, -14, -14, -11, -15 },
    { -14, -8, -14, -14, -8, -15 },
    { -14, -8, -14, -13, -9, -15 },
    { -55, -55, -55, -55, -55, -55 },
    { -55, -55, -55, -55, -55, -55 },
    { -55, -55, -55, -55, -55, -55 },
    { -55, -55, -55, -55, -55, -55 },
    { -55, -55, -55, -55, -55, -55 },
    { -55, -55, -55, -55, -55, -55 },
};

#endif
