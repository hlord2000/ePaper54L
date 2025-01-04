#ifndef STATE_MANAGER_H__
#define STATE_MANAGER_H__

#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/smf.h>

#define EVENT_KEY_0     BIT(0)
#define EVENT_KEY_1     BIT(1)
#define EVENT_KEY_2     BIT(2)
#define EVENT_KEY_3     BIT(3)
#define EVENT_BOOT_DONE BIT(4)

struct epd_sm_data {
    struct smf_ctx ctx;
    struct k_event smf_event;
    struct k_timer boot_timer;
    uint32_t events;
    uint8_t current_state;
};

#endif
