#include <zephyr/kernel.h>
#include <zephyr/smf.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h> 
#include <zephyr/pm/device.h>
#include <zephyr/drivers/display.h> 
#include <zephyr/input/input.h>
#include <lvgl.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(display, LOG_LEVEL);

const struct device *display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

#define X_RESOLUTION (int)DT_PROP(DT_CHOSEN(zephyr_display), width)
#define Y_RESOLUTION (int)DT_PROP(DT_CHOSEN(zephyr_display), height)

static bool display_active = true;

#if defined(CONFIG_PM_DEVICE)
static int epd_suspend(const struct device *dev) {
    int err = pm_device_action_run(dev, PM_DEVICE_ACTION_SUSPEND);
    if (err) {
        LOG_ERR("Failed to suspend display: %d", err);
        return err;
    }
    display_active = false;
    LOG_INF("Display suspended");
    return 0;
}

static int epd_resume(const struct device *dev) {
    int err = pm_device_action_run(dev, PM_DEVICE_ACTION_RESUME);
    if (err) {
        LOG_ERR("Failed to resume display: %d", err);
        return err;
    }
    display_active = true;
    LOG_INF("Display resumed");
    return 0;
}
#endif

static void epd_partial_update(void) {
	lv_task_handler();
}

static void epd_full_update(void) {
	display_blanking_on(display_dev);
	lv_task_handler();
	display_blanking_off(display_dev);
}

enum display_states { BOOT, CONFIG, NAME_SELECTOR, NAME_TAG, MOSAIC };

/*
const struct smf_state display_states[] = {
	[BOOT] = SMF_CREATE_STATE(boot_entry, boot_run, boot_exit, NULL, NULL),
	[CONFIG] = SMF_CREATE_STATE(config_entry, config_run, config_exit, NULL, NULL),
	[NAME_SELECTOR] = SMF_CREATE_STATE(name_select_entry, name_select_run, name_select_exit, NULL, NULL),
	[NAME_TAG] = SMF_CREATE_STATE(name_tag_entry, name_tag_run, name_tag_exit, NULL, NULL),
	[MOSAIC] = SMF_CREATE_STATE(mosaic_entry, mosaic_run, mosaic_exit, NULL, NULL),
};
*/

static const struct device *const buttons_dev = DEVICE_DT_GET(DT_NODELABEL(buttons));

void buttons_callback(struct input_event *evt, void *user_data) {
	ARG_UNUSED(user_data);
	if (evt->type == INPUT_EV_KEY) {
		switch(evt->code) {
		case INPUT_KEY_0:
			break;
		case INPUT_KEY_1:
			break;
		case INPUT_KEY_2:
			break;
		case INPUT_KEY_3:
			break;
		default:
			break;
		}
	}
}

INPUT_CALLBACK_DEFINE(buttons_dev, buttons_callback, NULL);

int epd_init(void) {
    if (!device_is_ready(display_dev)) {
        LOG_ERR("Display device not ready");
        return -1;
    }
    display_blanking_off(display_dev);
#if defined(CONFIG_PM_DEVICE)
    epd_suspend(display_dev);
#endif
    return 0;
}

SYS_INIT(epd_init, APPLICATION, 90);
