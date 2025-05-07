#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/poweroff.h>

const struct device *display_bus_dev = DEVICE_DT_GET(DT_PARENT(DT_CHOSEN(zephyr_display)));
const struct device *display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

int main(void) {
	int err;

	err = pm_device_action_run(display_dev, PM_DEVICE_ACTION_SUSPEND);
	if (err < 0) {
		printk("Failed to suspend display %d\r\n", err);
	}

	err = pm_device_action_run(display_bus_dev, PM_DEVICE_ACTION_SUSPEND);
	if (err < 0) {
		printk("Failed to suspend display bus %d\r\n", err);
	}

	printk("Goodbye\r\n");

	sys_poweroff();
}
