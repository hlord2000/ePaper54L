#ifndef ESL_PACKETS_H__
#define ESL_PACKETS_H__

#include <stdint.h>

#define ESL_CMD_COORDINATE 0x01
#define ESL_CMD_SENSOR 0x02

struct esl_coordinate {
    uint8_t x;
    uint8_t y;
} __packed;

struct esl_sensor_reading {
    float temperature;
    float humidity;
} __packed;

#endif
