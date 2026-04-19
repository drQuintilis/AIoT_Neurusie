/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <dk_buttons_and_leds.h>
#include <zephyr/logging/log.h>
#include <ram_pwrdn.h>
#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/vl53l0x.h>

#include "coap_client_utils.h"

#if CONFIG_BT_NUS
#include "ble_utils.h"
#endif

LOG_MODULE_REGISTER(coap_client, CONFIG_COAP_CLIENT_LOG_LEVEL);

#define OT_CONNECTION_LED DK_LED1
#define BLE_CONNECTION_LED DK_LED2
#define MTD_SED_LED DK_LED3

#if CONFIG_BT_NUS

#define COMMAND_REQUEST_UNICAST 'u'
#define COMMAND_REQUEST_MULTICAST 'm'
#define COMMAND_REQUEST_PROVISIONING 'p'

static void on_nus_received(struct bt_conn *conn, const uint8_t *const data, uint16_t len)
{
	LOG_INF("Received data: %c", data[0]);

	switch (*data) {
	case COMMAND_REQUEST_UNICAST:
		coap_client_toggle_one_light();
		break;

	case COMMAND_REQUEST_MULTICAST:
		coap_client_toggle_mesh_lights();
		break;

	case COMMAND_REQUEST_PROVISIONING:
		coap_client_send_provisioning_request();
		break;

	default:
		LOG_WRN("Received invalid data from NUS");
	}
}

static void on_ble_connect(struct k_work *item)
{
	ARG_UNUSED(item);

	dk_set_led_on(BLE_CONNECTION_LED);
}

static void on_ble_disconnect(struct k_work *item)
{
	ARG_UNUSED(item);

	dk_set_led_off(BLE_CONNECTION_LED);
}

#endif /* CONFIG_BT_NUS */

static void on_ot_connect(struct k_work *item)
{
	ARG_UNUSED(item);

	dk_set_led_on(OT_CONNECTION_LED);
}

static void on_ot_disconnect(struct k_work *item)
{
	ARG_UNUSED(item);

	dk_set_led_off(OT_CONNECTION_LED);
}

static void on_mtd_mode_toggle(uint32_t med)
{
#if IS_ENABLED(CONFIG_PM_DEVICE)
	const struct device *cons = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

	if (!device_is_ready(cons)) {
		return;
	}

	if (med) {
		pm_device_action_run(cons, PM_DEVICE_ACTION_RESUME);
	} else {
		pm_device_action_run(cons, PM_DEVICE_ACTION_SUSPEND);
	}
#endif
	dk_set_led(MTD_SED_LED, med);
}

static void on_button_changed(uint32_t button_state, uint32_t has_changed)
{
	uint32_t buttons = button_state & has_changed;

	if (buttons & DK_BTN1_MSK) {
		coap_client_toggle_one_light();
	}

	if (buttons & DK_BTN2_MSK) {
		coap_client_toggle_mesh_lights();
	}

	if (buttons & DK_BTN3_MSK) {
		coap_client_toggle_minimal_sleepy_end_device();
	}

	if (buttons & DK_BTN4_MSK) {
		coap_client_send_provisioning_request();
	}
}

int main(void)
{
	int ret;

	LOG_INF("Start CoAP-client sample");

	if (IS_ENABLED(CONFIG_RAM_POWER_DOWN_LIBRARY)) {
		power_down_unused_ram();
	}

	ret = dk_buttons_init(on_button_changed);
	if (ret) {
		LOG_ERR("Cannot init buttons (error: %d)", ret);
		return 0;
	}

	ret = dk_leds_init();
	if (ret) {
		LOG_ERR("Cannot init leds, (error: %d)", ret);
		return 0;
	}

#if CONFIG_BT_NUS
	struct bt_nus_cb nus_clbs = {
		.received = on_nus_received,
		.sent = NULL,
	};

	ret = ble_utils_init(&nus_clbs, on_ble_connect, on_ble_disconnect);
	if (ret) {
		LOG_ERR("Cannot init BLE utilities");
		return 0;
	}

#endif /* CONFIG_BT_NUS */

	coap_client_utils_init(on_ot_connect, on_ot_disconnect, on_mtd_mode_toggle);

	const struct device *const dev = DEVICE_DT_GET_ONE(st_vl53l0x);
	const struct device *const hc_sr04 = DEVICE_DT_GET_ONE(hc_sr04);
	struct sensor_value value;

	if (!device_is_ready(dev)) {
		LOG_ERR("sensor: VL53L0X device not ready.");
	}

	if (!device_is_ready(hc_sr04)) {
		LOG_ERR("sensor: HC-SR04 device not ready.");
	}

	while (1) {
		int dist_vl53l0x = 0;
		int dist_hc_sr04 = 0;

		if (device_is_ready(dev)) {
			sensor_sample_fetch(dev);
			sensor_channel_get(dev, SENSOR_CHAN_DISTANCE, &value);
			dist_vl53l0x = sensor_value_to_milli(&value);
		}

		if (device_is_ready(hc_sr04)) {
			sensor_sample_fetch(hc_sr04);
			sensor_channel_get(hc_sr04, SENSOR_CHAN_DISTANCE, &value);
			dist_hc_sr04 = sensor_value_to_milli(&value);
		}

		LOG_INF("VL53L0X: %d mm, HC-SR04: %d mm", dist_vl53l0x, dist_hc_sr04);

		coap_client_send_distances(dist_vl53l0x, dist_hc_sr04);

		k_sleep(K_MSEC(1000));
	}

	return 0;
}
