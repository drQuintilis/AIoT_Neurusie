#include "test_common.h"
#include "traffic_logic.h"

static void test_empty_direction_gets_min(void) {
    traffic_config_t cfg = traffic_default_config();
    traffic_phase_t p = traffic_compute_phase(&cfg, 0, 0);
    ASSERT_EQ(p.green_time_a_ms, cfg.min_green_ms);
    ASSERT_EQ(p.green_time_b_ms, cfg.min_green_ms);
    ASSERT_EQ(p.high_traffic, 0);
}

static void test_bonus_scales_with_count(void) {
    traffic_config_t cfg = traffic_default_config();
    traffic_phase_t p = traffic_compute_phase(&cfg, 3, 0);
    /* 5000 + 3*2000 = 11000 */
    ASSERT_EQ(p.green_time_a_ms, 11000);
}

static void test_green_time_clamped_to_max(void) {
    traffic_config_t cfg = traffic_default_config();
    traffic_phase_t p = traffic_compute_phase(&cfg, 200, 0);
    ASSERT_EQ(p.green_time_a_ms, cfg.max_green_ms);
}

static void test_high_traffic_flag_on_imbalance(void) {
    traffic_config_t cfg = traffic_default_config();   /* ratio = 2 */
    traffic_phase_t p = traffic_compute_phase(&cfg, 6, 1);
    ASSERT_EQ(p.high_traffic, 1);
    ASSERT_EQ(p.heavy_direction, 0);

    p = traffic_compute_phase(&cfg, 1, 10);
    ASSERT_EQ(p.high_traffic, 1);
    ASSERT_EQ(p.heavy_direction, 1);

    p = traffic_compute_phase(&cfg, 3, 2);  /* not 2x imbalance */
    ASSERT_EQ(p.high_traffic, 0);
}

int main(void) {
    RUN_TEST(test_empty_direction_gets_min);
    RUN_TEST(test_bonus_scales_with_count);
    RUN_TEST(test_green_time_clamped_to_max);
    RUN_TEST(test_high_traffic_flag_on_imbalance);
    TEST_RETURN();
}
