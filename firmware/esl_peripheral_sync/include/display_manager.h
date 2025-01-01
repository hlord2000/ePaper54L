#ifndef DISPLAY_MANAGER_H__
#define DISPLAY_MANAGER_H__

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/smf.h>
#include <lvgl.h>

// Display configuration
#define STATUS_HEIGHT 20
#define RIGHT_MARGIN 10
#define X_RESOLUTION (int)DT_PROP(DT_CHOSEN(zephyr_display), width)
#define Y_RESOLUTION (int)DT_PROP(DT_CHOSEN(zephyr_display), height)
#define CONTENT_HEIGHT (Y_RESOLUTION - STATUS_HEIGHT)

// Event definitions
#define EVENT_KEY_0     BIT(0)
#define EVENT_KEY_1     BIT(1)
#define EVENT_KEY_2     BIT(2)
#define EVENT_KEY_3     BIT(3)
#define EVENT_BOOT_DONE BIT(4)

// State definitions
enum display_states {
    BOOT,
    CONFIG,
    NAME_SELECTOR,
    NAME_TAG,
    MOSAIC
};

// State machine context
struct display_sm_data {
    struct smf_ctx ctx;
    struct k_event smf_event;
    struct k_timer boot_timer;
    uint32_t events;
    uint8_t current_state;
};

// UI Component handles
/*
*/

#endif /* DISPLAY_MANAGER_H__ */
