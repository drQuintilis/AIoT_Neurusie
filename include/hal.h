#ifndef HAL_H
#define HAL_H

#include <stdint.h>
#include <stdbool.h>

/* Hardware Abstraction Layer.
 *
 * Core modules (FSM, traffic_logic, etc.) never touch hardware directly.
 * They call these functions. A "stub" backend is used on PC (for tests /
 * demos) and a real backend (e.g. Arduino) implements the same prototypes
 * on the actual board. Only the .c file changes — headers stay identical. */

/* --- Light directions & colors (mirror intersection_fsm.h) --- */
typedef enum {
    HAL_DIR_A = 0,
    HAL_DIR_B = 1
} hal_direction_t;

typedef enum {
    HAL_LIGHT_OFF,
    HAL_LIGHT_RED,
    HAL_LIGHT_YELLOW,
    HAL_LIGHT_GREEN
} hal_light_t;

/* --- Lifecycle --- */
void     hal_init(void);

/* --- Time --- */
uint32_t hal_millis(void);           /* monotonic ms since boot */
void     hal_delay_ms(uint32_t ms);

/* --- Outputs --- */
void     hal_set_light(hal_direction_t dir, hal_light_t color);
void     hal_set_alarm(bool on);     /* buzzer / alarm LED */
void     hal_display_number(uint16_t value); /* 4-digit 7-seg via HC595 */

/* --- Inputs --- */
bool     hal_button_pressed(void);           /* emergency button / joystick */
uint16_t hal_ultrasonic_distance_cm(void);   /* HC-SR04, 0 = no reading */
uint16_t hal_light_sensor(void);             /* TEMT6000 raw */

#endif /* HAL_H */
