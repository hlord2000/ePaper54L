#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/pm/device.h>
#include <lvgl.h>
#include <stdio.h>
#include <zephyr/kernel.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app);

const struct device *display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
static uint32_t count = 0;
static bool display_active = true;

// Function to suspend display
static int suspend_display(const struct device *dev) {
    int err = pm_device_action_run(dev, PM_DEVICE_ACTION_SUSPEND);
    if (err) {
        LOG_ERR("Failed to suspend display: %d", err);
        return err;
    }
    display_active = false;
    LOG_INF("Display suspended");
    return 0;
}

// Function to resume display
static int resume_display(const struct device *dev) {
    int err = pm_device_action_run(dev, PM_DEVICE_ACTION_RESUME);
    if (err) {
        LOG_ERR("Failed to resume display: %d", err);
        return err;
    }
    display_active = true;
    LOG_INF("Display resumed");
    return 0;
}

static void epd_partial_update(void) {
	lv_task_handler();
}

static void epd_full_update(void) {
	display_blanking_on(display_dev);
	lv_task_handler();
	display_blanking_off(display_dev);
}

int main(void) {
    char count_str[24] = {0};
    lv_obj_t *count_label;

    if (!device_is_ready(display_dev)) {
        LOG_ERR("Display device not ready");
        return 0;
    }

	#if 1
    // Create a simple LVGL label
    count_label = lv_label_create(lv_scr_act());
    lv_obj_align(count_label, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(count_label, "Starting...");

	display_blanking_off(display_dev);
	epd_full_update();

    while (1) {
        // Toggle display state every 5 seconds
        if ((count % 50) == 0) {
            if (display_active) {
				epd_full_update();
                suspend_display(display_dev);
            } else {
                resume_display(display_dev);
            }
        }

        // Update counter display when active
        if (display_active && (count % 10) == 0) {
            sprintf(count_str, "Count: %d", count/10);
			LOG_INF("Updating to: %s", count_str);
            lv_label_set_text(count_label, count_str);
			// Full update every 10 iters
			if ((count % 100) == 0) {
				epd_full_update();
			} else {
				epd_partial_update();
			}
        }

        count++;
        k_sleep(K_MSEC(100));
    }
#endif
	suspend_display(display_dev);
    return 0;
}
