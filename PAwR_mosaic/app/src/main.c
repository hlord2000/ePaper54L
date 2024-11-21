/*
 * Copyright (c) 2018 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <lvgl.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <lvgl_input_device.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app);

const struct device *display_spi = DEVICE_DT_GET(DT_PROP(DT_PARENT(DT_CHOSEN(zephyr_display)), spi_dev));

int main(void) {
	const struct device *display_dev;

	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		LOG_ERR("Device not ready, aborting test");
	}

	uint8_t my_str[] = "Hey!";
	uint8_t other_str[] = "What's good?";
	lv_obj_t *label = lv_label_create(lv_scr_act());
	lv_label_set_text(label, my_str);
	lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
	lv_task_handler();

	int ret;
	while (true) {
		LOG_INF("Suspending display");
		ret = pm_device_action_run(display_dev, PM_DEVICE_ACTION_SUSPEND);
		if (ret < 0) {
			LOG_ERR("Could not suspend the display (err %d)", ret);
		}

		LOG_INF("Suspending display bus");
		ret = pm_device_action_run(display_spi, PM_DEVICE_ACTION_SUSPEND);
		if (ret < 0) {
			LOG_ERR("Could not suspend the display bus (err %d)", ret);
			return ret;
		}
		k_msleep(5000);
		LOG_INF("Resuming display bus");
		ret = pm_device_action_run(display_spi, PM_DEVICE_ACTION_RESUME);
		if (ret < 0) {
			LOG_ERR("Could not resume the display (err %d)", ret);
			return ret;
		}
		LOG_INF("Resuming display");
		ret = pm_device_action_run(display_dev, PM_DEVICE_ACTION_RESUME);
		if (ret < 0) {
			LOG_ERR("Could not resume the display (err %d)", ret);
		}
		lv_label_set_text(label, other_str);
		k_msleep(100);
		lv_task_handler();
		k_msleep(1000);
		lv_label_set_text(label, my_str);
		k_msleep(100);
		lv_task_handler();
	}
}
