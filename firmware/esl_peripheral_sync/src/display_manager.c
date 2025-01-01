#include <zephyr/kernel.h>
#include <zephyr/smf.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/pm/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/input/input.h>
#include <lvgl.h>

#include "display_manager.h"
#include "ui_manager.h"
#include "images.h"
#include "nametag.h"

#include <zephyr/logging/log.h>
#define LOG_LEVEL 4
LOG_MODULE_REGISTER(display_manager);

#define BOOT_DISPLAY_DURATION_MS 5000

// Global device pointer
static const struct device *display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
static const struct device *const buttons_dev = DEVICE_DT_GET(DT_NODELABEL(buttons));

// Global state machine context
static struct display_sm_data sm_data;

// Forward declarations for state handlers
static void boot_entry(void *o);
static void boot_run(void *o);
static void boot_exit(void *o);
static void config_entry(void *o);
static void config_run(void *o);
static void config_exit(void *o);
static void name_tag_entry(void *o);
static void name_tag_exit(void *o);
static void mosaic_entry(void *o);
static void mosaic_exit(void *o);

// State machine state definitions
static const struct smf_state display_states[] = {
    [BOOT] = SMF_CREATE_STATE(boot_entry, boot_run, boot_exit, NULL, NULL),
    [CONFIG] = SMF_CREATE_STATE(config_entry, config_run, config_exit, NULL, NULL),
    [NAME_TAG] = SMF_CREATE_STATE(name_tag_entry, NULL, name_tag_exit, NULL, NULL),
    [MOSAIC] = SMF_CREATE_STATE(mosaic_entry, NULL, mosaic_exit, NULL, NULL),
};

static void boot_timer_expired(struct k_timer *timer) {
    k_event_post(&sm_data.smf_event, EVENT_BOOT_DONE);
}

// Boot state handlers
static void boot_entry(void *o) {
    struct display_sm_data *sm = (struct display_sm_data *)o;
    LOG_INF("Entering boot state");
    k_timer_start(&sm->boot_timer, K_MSEC(BOOT_DISPLAY_DURATION_MS), K_NO_WAIT);
    ui_manager_resume();
}

static void boot_run(void *o) {
    struct display_sm_data *sm = (struct display_sm_data *)o;
    LOG_INF("Running boot state");

    static bool image_set = false;
    if (!image_set) {
        lv_obj_t *display_image = lv_img_create(lv_scr_act());
        lv_img_set_src(display_image, &nordic);
        ui_manager_full_update();
        image_set = true;
    }
    
    if (sm->events & EVENT_BOOT_DONE) {
        smf_set_state(SMF_CTX(&sm->ctx), &display_states[CONFIG]);
    }
}

static void boot_exit(void *o) {
    struct display_sm_data *sm = (struct display_sm_data *)o;
    LOG_INF("Exiting boot state");
    k_timer_stop(&sm->boot_timer);
    ui_manager_suspend();
}

static lv_obj_t *config_roller;

static void config_roller_cb(lv_event_t * e) {
	LOG_INF("Event: %d", e->code);
	int index = *((int *)e->user_data);
	LOG_INF("Button index: %d", index);

	lv_key_t key = 0;
	if (index == 1) {
		key = LV_KEY_UP;
	} else if (index == 2) {
		key = LV_KEY_DOWN;
	}
	lv_event_send(config_roller, LV_EVENT_KEY, &key);
}

static button_config_t buttons[] = {
	{
		.text = "Mosaic",
		.callback = NULL,
		.visible = true,
		.index = 0,
	},
	{
		.text = "Up",
		.callback = config_roller_cb,
		.visible = true,
		.index = 1,
	},
	{
		.text = "Down",
		.callback = config_roller_cb,
		.visible = true,
		.index = 2,
	},
	{
		.text = "Ok",
		.callback = NULL,
		.visible = true,
		.index = 3,
	},
};

// Config state handlers
static void config_entry(void *o) {
    struct display_sm_data *sm = (struct display_sm_data *)o;
    LOG_INF("Entering config state");
    sm->current_state = CONFIG;
    
    ui_manager_resume();
    ui_manager_clear_main();
    
    // Create roller in main content
    lv_obj_t *main_content = ui_manager_get_main();
    config_roller = lv_roller_create(main_content);
	
	char name_string[128] = {0};
	nametag_get_string(name_string, 128);

    lv_roller_set_options(config_roller,
                         name_string,
                         LV_ROLLER_MODE_INFINITE);

    lv_roller_set_visible_row_count(config_roller, 4);

    // Style for selected text
    lv_obj_set_style_bg_color(config_roller, lv_color_black(), LV_PART_SELECTED);
    lv_obj_set_style_bg_opa(config_roller, LV_OPA_COVER, LV_PART_SELECTED);
    lv_obj_set_style_text_color(config_roller, lv_color_white(), LV_PART_SELECTED);

    lv_obj_center(config_roller);
	
	ui_manager_set_buttons(buttons, sizeof(buttons)/sizeof(button_config_t));

	ui_manager_show_bottom_bar(true);
    
    ui_manager_full_update();
}

static void config_run(void *o) {
    struct display_sm_data *sm = (struct display_sm_data *)o;

	ui_manager_partial_update();

	if (sm->events & EVENT_KEY_0) {
		smf_set_state(SMF_CTX(&sm->ctx), &display_states[MOSAIC]);
	} else if (sm->events & EVENT_KEY_3) {
        smf_set_state(SMF_CTX(&sm->ctx), &display_states[NAME_TAG]);
	}
}

int nametag_index = -1;

static void config_exit(void *o) {
    LOG_INF("Exiting config state");
	LOG_INF("Name tag index is: %d", nametag_index);
    ui_manager_suspend();
	
	nametag_index = lv_roller_get_selected(config_roller);
}

// Name tag state handlers
static void name_tag_entry(void *o) {

    struct display_sm_data *sm = (struct display_sm_data *)o;
    sm->current_state = NAME_TAG;

    LOG_INF("Entering name tag state");

    ui_manager_resume();
	ui_manager_show_bottom_bar(false);

	nametag_display_show(nametag_index);

	ui_manager_suspend();
}

static void name_tag_run(void *o) {
    struct display_sm_data *sm = (struct display_sm_data *)o;

	/*
	if (ui_manager_is_active()) {
		ui_manager_partial_update();
	}
	*/

	if (ui_manager_is_bottom_bar_visible()) {
		if (sm->events & EVENT_KEY_0) {
			smf_set_state(SMF_CTX(&sm->ctx), &display_states[MOSAIC]);
		} else if (sm->events & EVENT_KEY_3) {
			smf_set_state(SMF_CTX(&sm->ctx), &display_states[NAME_TAG]);
		}
	}
}

static void name_tag_exit(void *o) {
    LOG_INF("Exiting name tag state");
}

// Mosaic state handlers
static void mosaic_entry(void *o) {
    struct display_sm_data *sm = (struct display_sm_data *)o;
    LOG_INF("Entering mosaic state");
    ui_manager_resume();
    sm->current_state = MOSAIC;
    // Auto transition back to config
    smf_set_state(SMF_CTX(&sm->ctx), &display_states[CONFIG]);
}

static void mosaic_exit(void *o) {
    LOG_INF("Exiting mosaic state");
}

// Button callback
static void buttons_callback(struct input_event *evt, void *user_data) {
    ARG_UNUSED(user_data);
    if (evt->type == INPUT_EV_KEY) {
        switch (evt->code) {
            case INPUT_KEY_0:
                LOG_INF("Button 0 pressed, posting EVENT_KEY_0");
                k_event_post(&sm_data.smf_event, EVENT_KEY_0);
                break;
            case INPUT_KEY_1:
                LOG_INF("Button 1 pressed, posting EVENT_KEY_1");
                k_event_post(&sm_data.smf_event, EVENT_KEY_1);
                break;
            case INPUT_KEY_2:
                LOG_INF("Button 2 pressed, posting EVENT_KEY_2");
                k_event_post(&sm_data.smf_event, EVENT_KEY_2);
                break;
            case INPUT_KEY_3:
                LOG_INF("Button 3 pressed, posting EVENT_KEY_3");
                k_event_post(&sm_data.smf_event, EVENT_KEY_3);
                break;
            default:
                LOG_WRN("Unknown button code: %d", evt->code);
                break;
        }
    }
}

INPUT_CALLBACK_DEFINE(buttons_dev, buttons_callback, NULL);

// State machine thread
static void display_thread(void *p1, void *p2, void *p3) {
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);
    
    if (!device_is_ready(display_dev)) {
        LOG_ERR("Display device not ready");
        return;
    }

    int err = ui_manager_init();
    if (err) {
        LOG_ERR("Failed to initialize UI manager: %d", err);
    }

    k_event_init(&sm_data.smf_event);
    k_timer_init(&sm_data.boot_timer, boot_timer_expired, NULL);
    
#if 0
    smf_set_initial(SMF_CTX(&sm_data.ctx), &display_states[BOOT]);
    sm_data.current_state = BOOT;
#else
    smf_set_initial(SMF_CTX(&sm_data.ctx), &display_states[CONFIG]);
    sm_data.current_state = CONFIG;
#endif
    sm_data.events = 0;


    if (smf_run_state(SMF_CTX(&sm_data.ctx))) {
        LOG_ERR("Initial state machine run failed");
    }

    while (1) {
        sm_data.events = k_event_wait(&sm_data.smf_event,
                                    EVENT_KEY_0 | EVENT_KEY_1 | EVENT_KEY_2 | 
                                    EVENT_KEY_3 | EVENT_BOOT_DONE,
                                    true, K_MSEC(100));
        
        if (sm_data.events != 0) {
            LOG_INF("Got events: 0x%x", sm_data.events);
        }
        
        if (smf_run_state(SMF_CTX(&sm_data.ctx))) {
            LOG_ERR("State machine run failed");
        }
        
        sm_data.events = 0;
    }
}

K_THREAD_DEFINE(display_thread_id, 8192, display_thread, NULL, NULL, NULL, 7, 0, 0);
