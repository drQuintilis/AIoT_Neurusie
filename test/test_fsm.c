#include "test_common.h"
#include "intersection_fsm.h"

static void test_initial_state(void) {
    intersection_ctx_t ctx;
    intersection_init(&ctx);
    ASSERT_EQ(ctx.state, STATE_NORMAL);
    ASSERT_EQ(ctx.lights.dir_a, LIGHT_GREEN);
    ASSERT_EQ(ctx.lights.dir_b, LIGHT_RED);
    ASSERT_EQ(ctx.lights.alarm_active, 0);
}

static void test_timeout_rotates_phases(void) {
    intersection_ctx_t ctx;
    intersection_init(&ctx);
    intersection_update(&ctx, EVT_TIMEOUT);
    ASSERT_EQ(ctx.lights.dir_a, LIGHT_RED);
    ASSERT_EQ(ctx.lights.dir_b, LIGHT_GREEN);
    intersection_update(&ctx, EVT_TIMEOUT);
    ASSERT_EQ(ctx.lights.dir_a, LIGHT_GREEN);
    ASSERT_EQ(ctx.lights.dir_b, LIGHT_RED);
}

static void test_crash_goes_to_emergency_from_any_state(void) {
    intersection_ctx_t ctx;

    intersection_init(&ctx);
    intersection_update(&ctx, EVT_CRASH);
    ASSERT_EQ(ctx.state, STATE_EMERGENCY);
    ASSERT_EQ(ctx.lights.alarm_active, 1);
    ASSERT_EQ(ctx.lights.dir_a, LIGHT_RED);
    ASSERT_EQ(ctx.lights.dir_b, LIGHT_RED);

    intersection_init(&ctx);
    intersection_update(&ctx, EVT_HIGH_TRAFFIC);
    ASSERT_EQ(ctx.state, STATE_HIGH_TRAFFIC);
    intersection_update(&ctx, EVT_CRASH);
    ASSERT_EQ(ctx.state, STATE_EMERGENCY);
}

static void test_clear_then_timeout_returns_to_normal(void) {
    intersection_ctx_t ctx;
    intersection_init(&ctx);
    intersection_update(&ctx, EVT_CRASH);
    intersection_update(&ctx, EVT_CLEAR);
    ASSERT_EQ(ctx.state, STATE_COOLDOWN);
    ASSERT_EQ(ctx.lights.dir_a, LIGHT_FLASH_YELLOW);
    intersection_update(&ctx, EVT_TIMEOUT);
    ASSERT_EQ(ctx.state, STATE_NORMAL);
}

static void test_high_traffic_uses_heavier_direction(void) {
    intersection_ctx_t ctx;
    intersection_init(&ctx);
    ctx.traffic_count[0] = 2;
    ctx.traffic_count[1] = 8;   /* B is heavier */
    intersection_update(&ctx, EVT_HIGH_TRAFFIC);
    ASSERT_EQ(ctx.state, STATE_HIGH_TRAFFIC);
    ASSERT_EQ(ctx.lights.dir_b, LIGHT_GREEN);
    ASSERT_EQ(ctx.lights.dir_a, LIGHT_RED);
}

int main(void) {
    RUN_TEST(test_initial_state);
    RUN_TEST(test_timeout_rotates_phases);
    RUN_TEST(test_crash_goes_to_emergency_from_any_state);
    RUN_TEST(test_clear_then_timeout_returns_to_normal);
    RUN_TEST(test_high_traffic_uses_heavier_direction);
    TEST_RETURN();
}
