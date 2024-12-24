#include <zephyr/kernel.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#define LOG_LEVEL 4 

LOG_MODULE_REGISTER(temp_humidity_sensor);

#include "esl_packets.h"

ZBUS_CHAN_DEFINE(sensor_chan, 
				 struct esl_sensor_reading,
				 NULL,
				 NULL,
				 ZBUS_OBSERVERS_EMPTY,
				 ZBUS_MSG_INIT(0)
);

struct esl_sensor_reading sensor_reading = {0};

#if defined(CONFIG_ESL_SENSOR_MOCK)
static void sensor_get_work_handler(struct k_work *work) {
	LOG_INF("Getting sensor, mock");
}
#else
static void sensor_get_work_handler(struct k_work *work) {
	LOG_INF("Getting sensor");
	sensor_reading.temperature++;
	sensor_reading.humidity++;
	int err = zbus_chan_pub(&sensor_chan, &sensor_reading, K_SECONDS(1));
	if (err < 0) {
		LOG_ERR("Failed to publish sensor value");
	}
}
#endif

static K_WORK_DEFINE(sensor_get_work, sensor_get_work_handler);

static void sensor_timer_expiry_function(struct k_timer *timer_id) {
	int err = k_work_submit(&sensor_get_work);
	if (err < 0) {
		LOG_ERR("Failed to submit sensor get work item");
	}
}

K_TIMER_DEFINE(sensor_timer, sensor_timer_expiry_function, NULL);

static int sensor_timer_init(void) {
	LOG_INF("Initialized sensor timer");
	k_timer_start(&sensor_timer, K_SECONDS(5), K_SECONDS(5));
	return 0;
}

SYS_INIT(sensor_timer_init, APPLICATION, 90);
