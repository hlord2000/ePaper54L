#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>

#if defined(CONFIG_PM_DEVICE)
#include <zephyr/pm/device.h>
#endif

LOG_MODULE_REGISTER(input_manager, LOG_LEVEL_INF);

static const struct device *const buttons_dev = DEVICE_DT_GET(DT_NODELABEL(buttons));

int buttons_init(void) {
    int err = pm_device_action_run(buttons_dev, PM_DEVICE_ACTION_RESUME);
	return err;
}

SYS_INIT(buttons_init, APPLICATION, 90);
