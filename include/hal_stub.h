#ifndef HAL_STUB_H
#define HAL_STUB_H

#include <stdint.h>
#include <stdbool.h>

/* Test/demo-only setters for the stub HAL backend. Not available on Arduino. */
void hal_stub_set_button(bool pressed);
void hal_stub_set_distance(uint16_t cm);
void hal_stub_set_light_sensor(uint16_t raw);

#endif /* HAL_STUB_H */
