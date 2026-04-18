#include "test_common.h"
#include "hal.h"
#include "hal_stub.h"

static void test_stub_inputs_are_controllable(void) {
    hal_init();

    hal_stub_set_button(true);
    ASSERT_EQ(hal_button_pressed(), 1);
    hal_stub_set_button(false);
    ASSERT_EQ(hal_button_pressed(), 0);

    hal_stub_set_distance(42);
    ASSERT_EQ(hal_ultrasonic_distance_cm(), 42);

    hal_stub_set_light_sensor(800);
    ASSERT_EQ(hal_light_sensor(), 800);
}

static void test_millis_advances(void) {
    uint32_t t0 = hal_millis();
    hal_delay_ms(20);
    uint32_t t1 = hal_millis();
    ASSERT_TRUE(t1 >= t0 + 15);   /* allow a little jitter */
}

int main(void) {
    RUN_TEST(test_stub_inputs_are_controllable);
    RUN_TEST(test_millis_advances);
    TEST_RETURN();
}
