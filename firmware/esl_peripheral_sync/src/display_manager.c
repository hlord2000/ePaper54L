#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/pm/device.h>
#include <zephyr/drivers/display.h>
#include <lvgl.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(display_manager, LOG_LEVEL_INF);

#include "display_manager.h"

static const struct device *display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

static bool display_active = true;

void display_manager_full_update(void) {
    if (!display_active) {
        return;
    }
    display_blanking_on(display_dev);
    lv_task_handler();
    display_blanking_off(display_dev);
}

void display_manager_partial_update(void) {
    if (!display_active) {
        return;
    }
    lv_task_handler();
}

int display_manager_resume(void) {
#if defined(CONFIG_PM_DEVICE)
    int err = pm_device_action_run(display_dev, PM_DEVICE_ACTION_RESUME);
    if (err) {
        LOG_ERR("Failed to resume display: %d", err);
        return err;
    }
    display_active = true;
    LOG_INF("Display resumed");
#endif
    return 0;
}

int display_manager_suspend(void) {
#if defined(CONFIG_PM_DEVICE)
    int err = pm_device_action_run(display_dev, PM_DEVICE_ACTION_SUSPEND);
    if (err) {
        LOG_ERR("Failed to suspend display: %d", err);
        return err;
    }
    display_active = false;
    LOG_INF("Display suspended");
#endif
    return 0;
}

bool display_manager_is_active(void) {
	return display_active;
}
