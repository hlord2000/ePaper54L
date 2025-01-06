#include <zephyr/kernel.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(environmental_sensor, LOG_LEVEL_INF);

#include "esl_packets.h"

const struct device *const sht = DEVICE_DT_GET_ANY(sensirion_sht4x);

ZBUS_CHAN_DEFINE(sensor_chan, 
				 struct esl_sensor_reading,
				 NULL,
				 NULL,
				 ZBUS_OBSERVERS_EMPTY,
				 ZBUS_MSG_INIT(0)
);

struct esl_sensor_reading sensor_reading = {0};
struct sensor_value temp = {0};
struct sensor_value hum = {0};

#if defined(CONFIG_ESL_SENSOR_MOCK)
static void sensor_get_work_handler(struct k_work *work) {
	LOG_INF("Getting sensor, mock");
}
#else

static void sensor_get_work_handler(struct k_work *work) {
	if (sensor_sample_fetch(sht)) {
		LOG_INF("Failed to fetch sample from SHT4X device");
		return;
	}
	sensor_channel_get(sht, SENSOR_CHAN_AMBIENT_TEMP, &temp);
	sensor_channel_get(sht, SENSOR_CHAN_HUMIDITY, &hum);

	sensor_reading.temperature = sensor_value_to_float(&temp);
	sensor_reading.humidity = sensor_value_to_float(&hum);

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
