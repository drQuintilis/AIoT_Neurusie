#include "test_common.h"
#include "soft_timer.h"

static int g_fired_count = 0;
static uint8_t g_last_id  = 0xFF;

static void on_fire(uint8_t id) {
    g_fired_count++;
    g_last_id = id;
}

static void test_one_shot_fires_once(void) {
    timer_mgr_t m;
    timer_mgr_init(&m);
    g_fired_count = 0;

    timer_start(&m, 0, 1000, on_fire, false);
    timer_tick(&m, 500);
    ASSERT_EQ(g_fired_count, 0);
    timer_tick(&m, 600);       /* crosses 1000 ms */
    ASSERT_EQ(g_fired_count, 1);
    ASSERT_EQ(g_last_id, 0);

    timer_tick(&m, 5000);      /* one-shot must not fire again */
    ASSERT_EQ(g_fired_count, 1);
}

static void test_repeat_fires_multiple_times(void) {
    timer_mgr_t m;
    timer_mgr_init(&m);
    g_fired_count = 0;

    timer_start(&m, 1, 100, on_fire, true);
    timer_tick(&m, 350);       /* should fire 3 times */
    ASSERT_EQ(g_fired_count, 3);
}

static void test_remaining_counts_down(void) {
    timer_mgr_t m;
    timer_mgr_init(&m);
    timer_start(&m, 2, 1000, on_fire, false);
    ASSERT_EQ(timer_remaining(&m, 2), 1000);
    timer_tick(&m, 300);
    ASSERT_EQ(timer_remaining(&m, 2), 700);
}

static void test_stop_disables(void) {
    timer_mgr_t m;
    timer_mgr_init(&m);
    g_fired_count = 0;
    timer_start(&m, 3, 100, on_fire, true);
    timer_stop(&m, 3);
    timer_tick(&m, 1000);
    ASSERT_EQ(g_fired_count, 0);
}

int main(void) {
    RUN_TEST(test_one_shot_fires_once);
    RUN_TEST(test_repeat_fires_multiple_times);
    RUN_TEST(test_remaining_counts_down);
    RUN_TEST(test_stop_disables);
    TEST_RETURN();
}
