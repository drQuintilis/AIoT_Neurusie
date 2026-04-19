/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <dk_buttons_and_leds.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/openthread.h>
#include <openthread/thread.h>
#include <zephyr/device.h>
#include <zephyr/drivers/led_strip.h>
#include <stdlib.h>
#include <string.h>

#include "ot_coap_utils.h"

LOG_MODULE_REGISTER(coap_server, CONFIG_COAP_SERVER_LOG_LEVEL);

#define STRIP_NODE DT_ALIAS(led_strip)
#if DT_NODE_HAS_STATUS_OKAY(STRIP_NODE)
#define STRIP_NUM_PIXELS DT_PROP(STRIP_NODE, chain_length)
#define HAS_LED_STRIP 1
static const struct device *const strip = DEVICE_DT_GET(STRIP_NODE);
static struct led_rgb pixels[STRIP_NUM_PIXELS];
#else
#define HAS_LED_STRIP 0
#endif

#if defined(CONFIG_BOARD_NRF7002DK_NRF5340_CPUAPP) || defined(CONFIG_BOARD_NRF7002DK_NRF5340_CPUAPP_NS)
#define OT_CONNECTION_LED DK_LED1
#define PROVISIONING_LED DK_LED2
#define LIGHT_LED DK_LED2
#define PAIRING_BUTTON DK_BTN2_MSK
#else
#define OT_CONNECTION_LED DK_LED1
#define PROVISIONING_LED DK_LED3
#define LIGHT_LED DK_LED4
#define PAIRING_BUTTON DK_BTN4_MSK
#endif

#define COAP_SERVER_WORKQ_STACK_SIZE 2048
#define COAP_SERVER_WORKQ_PRIORITY 5

K_THREAD_STACK_DEFINE(coap_server_workq_stack_area, COAP_SERVER_WORKQ_STACK_SIZE);
static struct k_work_q coap_server_workq;

static struct k_work provisioning_work;

static struct k_timer led_timer;
static struct k_timer provisioning_timer;

static void on_light_request(uint8_t command)
{
	static uint8_t val;

	switch (command) {
	case THREAD_COAP_UTILS_LIGHT_CMD_ON:
		dk_set_led_on(LIGHT_LED);
		val = 1;
		break;

	case THREAD_COAP_UTILS_LIGHT_CMD_OFF:
		dk_set_led_off(LIGHT_LED);
		val = 0;
		break;

	case THREAD_COAP_UTILS_LIGHT_CMD_TOGGLE:
		val = !val;
		dk_set_led(LIGHT_LED, val);
		break;

	default:
		break;
	}
}

static void activate_provisioning(struct k_work *item)
{
	ARG_UNUSED(item);

	ot_coap_activate_provisioning();

	k_timer_start(&led_timer, K_MSEC(100), K_MSEC(100));
	k_timer_start(&provisioning_timer, K_SECONDS(5), K_NO_WAIT);

	LOG_INF("Provisioning activated");
}

static void deactivate_provisionig(void)
{
	k_timer_stop(&led_timer);
	k_timer_stop(&provisioning_timer);

	if (ot_coap_is_provisioning_active()) {
		ot_coap_deactivate_provisioning();
		LOG_INF("Provisioning deactivated");
	}
}

static void on_provisioning_timer_expiry(struct k_timer *timer_id)
{
	ARG_UNUSED(timer_id);

	deactivate_provisionig();
}

static void on_led_timer_expiry(struct k_timer *timer_id)
{
	static uint8_t val = 1;

	ARG_UNUSED(timer_id);

	dk_set_led(PROVISIONING_LED, val);
	val = !val;
}

static void on_led_timer_stop(struct k_timer *timer_id)
{
	ARG_UNUSED(timer_id);

	dk_set_led_off(PROVISIONING_LED);
}

static void on_button_changed(uint32_t button_state, uint32_t has_changed)
{
	uint32_t buttons = button_state & has_changed;

	if (buttons & PAIRING_BUTTON) {
		k_work_submit_to_queue(&coap_server_workq, &provisioning_work);
	}
}

static void set_all_pixels(uint8_t r, uint8_t g, uint8_t b)
{
#if HAS_LED_STRIP
        for (int i = 0; i < STRIP_NUM_PIXELS; i++) {
                pixels[i].r = r;
                pixels[i].g = g;
                pixels[i].b = b;
        }
        led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);
#endif
}

static void update_traffic_light(int distance_mm)
{
        if (distance_mm < 100) {
                /* Red */
                set_all_pixels(100, 0, 0);
        } else if (distance_mm < 500) {
                /* Yellow */
                set_all_pixels(100, 100, 0);
        } else {
                /* Green */
                set_all_pixels(0, 100, 0);
        }
}

static void on_distance_request(const char *payload, uint16_t len)
{
        LOG_INF("Distance updated: %.*s", len, payload);
        
        char buf[16];
        uint16_t copy_len = len < sizeof(buf) - 1 ? len : sizeof(buf) - 1;
        memcpy(buf, payload, copy_len);
        buf[copy_len] = '\0';
        
        int distance = atoi(buf);
        update_traffic_light(distance);
}
static void on_thread_state_changed(otChangedFlags flags, void *user_data)
{
	if (flags & OT_CHANGED_THREAD_ROLE) {
		switch (otThreadGetDeviceRole(openthread_get_default_instance())) {
		case OT_DEVICE_ROLE_CHILD:
		case OT_DEVICE_ROLE_ROUTER:
		case OT_DEVICE_ROLE_LEADER:
			dk_set_led_on(OT_CONNECTION_LED);
			break;

		case OT_DEVICE_ROLE_DISABLED:
		case OT_DEVICE_ROLE_DETACHED:
		default:
			dk_set_led_off(OT_CONNECTION_LED);
			deactivate_provisionig();
			break;
		}
	}
}

static struct openthread_state_changed_callback ot_state_chaged_cb = {
	.otCallback = on_thread_state_changed};

int main(void)
{
	int ret;

	LOG_INF("Start CoAP-server sample");

	k_timer_init(&led_timer, on_led_timer_expiry, on_led_timer_stop);
	k_timer_init(&provisioning_timer, on_provisioning_timer_expiry, NULL);

	k_work_queue_init(&coap_server_workq);
	k_work_queue_start(&coap_server_workq, coap_server_workq_stack_area,
					K_THREAD_STACK_SIZEOF(coap_server_workq_stack_area),
					COAP_SERVER_WORKQ_PRIORITY, NULL);
	k_work_init(&provisioning_work, activate_provisioning);

	ret = ot_coap_init(&deactivate_provisionig, &on_light_request, &on_distance_request);
	if (ret) {
		LOG_ERR("Could not initialize OpenThread CoAP");
		goto end;
	}

	        ret = dk_leds_init();
	        if (ret) {
	                LOG_ERR("Could not initialize leds, err code: %d", ret);
	                goto end;
	        }
	
	#if HAS_LED_STRIP
	        if (device_is_ready(strip)) {
	                LOG_INF("Found LED strip device %s", strip->name);
	                set_all_pixels(0, 0, 0);
	        } else {
	                LOG_ERR("LED strip device %s is not ready", strip->name);
	        }
	#endif
	ret = dk_buttons_init(on_button_changed);
	if (ret) {
		LOG_ERR("Cannot init buttons (error: %d)", ret);
		goto end;
	}

	openthread_state_changed_callback_register(&ot_state_chaged_cb);
	openthread_run();

end:
	return 0;
}
