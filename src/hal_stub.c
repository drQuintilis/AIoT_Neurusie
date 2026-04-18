#include "hal.h"

#include <stdio.h>
#include <time.h>

/* PC stub backend for the HAL.
 *
 * - Prints every action so you can "see" the intersection without LEDs.
 * - hal_millis() uses CLOCK_MONOTONIC.
 * - Inputs return safe defaults; tests override via the `hal_stub_*`
 *   setters below so behaviour is deterministic. */

static uint32_t g_start_ms = 0;

static bool     g_stub_button     = false;
static uint16_t g_stub_distance   = 0;
static uint16_t g_stub_light_lvl  = 512;

static const char *light_name(hal_light_t c) {
    switch (c) {
        case HAL_LIGHT_OFF:    return "OFF";
        case HAL_LIGHT_RED:    return "RED";
        case HAL_LIGHT_YELLOW: return "YELLOW";
        case HAL_LIGHT_GREEN:  return "GREEN";
    }
    return "?";
}

static uint32_t now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000UL + ts.tv_nsec / 1000000UL);
}

void hal_init(void) {
    g_start_ms = now_ms();
    printf("[HAL] init (stub backend)\n");
}

uint32_t hal_millis(void) {
    return now_ms() - g_start_ms;
}

void hal_delay_ms(uint32_t ms) {
    /* Busy loop would waste cycles on PC; use clock_nanosleep via timespec. */
    struct timespec ts = { .tv_sec = ms / 1000,
                           .tv_nsec = (ms % 1000) * 1000000L };
    nanosleep(&ts, 0);
}

void hal_set_light(hal_direction_t dir, hal_light_t color) {
    printf("[HAL] light %c -> %s\n",
           dir == HAL_DIR_A ? 'A' : 'B', light_name(color));
}

void hal_set_alarm(bool on) {
    printf("[HAL] alarm %s\n", on ? "ON" : "OFF");
}

void hal_display_number(uint16_t value) {
    printf("[HAL] 7-seg: %04u\n", value);
}

bool     hal_button_pressed(void)         { return g_stub_button; }
uint16_t hal_ultrasonic_distance_cm(void) { return g_stub_distance; }
uint16_t hal_light_sensor(void)           { return g_stub_light_lvl; }

/* --- Stub-only setters: only visible when building with the stub backend.
 * Tests/demos include hal_stub.h to drive the fake inputs. */
void hal_stub_set_button(bool pressed)        { g_stub_button    = pressed; }
void hal_stub_set_distance(uint16_t cm)       { g_stub_distance  = cm; }
void hal_stub_set_light_sensor(uint16_t raw)  { g_stub_light_lvl = raw; }
