#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/smf.h>
#include <zephyr/input/input.h>
#include <lvgl.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(state_manager, LOG_LEVEL_INF);

#include "display_manager.h"
#include "images.h"
#include "state_manager.h"
#include "ui_manager.h"
#include "nametag.h"
#include "routes.h"

static const struct device *const buttons_dev = DEVICE_DT_GET(DT_NODELABEL(buttons));

// Global state machine context
static struct epd_sm_data sm_data;

// Forward declarations for state handlers
static void boot_entry(void *o);
static void boot_run(void *o);
static void boot_exit(void *o);
static void config_entry(void *o);
static void config_run(void *o);
static void config_exit(void *o);
static void name_tag_entry(void *o);
static void name_tag_run(void *o);
static void name_tag_exit(void *o);
static void mosaic_entry(void *o);
static void mosaic_run(void *o);
static void mosaic_exit(void *o);
static void diagnostics_entry(void *o);
static void diagnostics_run(void *o);
static void diagnostics_exit(void *o);

enum epd_states {BOOT_STATE, CONFIG_STATE, NAME_TAG_STATE, MOSAIC_STATE, DIAGNOSTICS_STATE};

// State machine state definitions
static const struct smf_state display_states[] = {
    [BOOT_STATE] = SMF_CREATE_STATE(boot_entry, boot_run, boot_exit, NULL, NULL),
    [CONFIG_STATE] = SMF_CREATE_STATE(config_entry, config_run, config_exit, NULL, NULL), 
	[NAME_TAG_STATE] = SMF_CREATE_STATE(name_tag_entry, name_tag_run, name_tag_exit, NULL, NULL), 
	[MOSAIC_STATE] = SMF_CREATE_STATE(mosaic_entry, mosaic_run, mosaic_exit, NULL, NULL),
	[DIAGNOSTICS_STATE] = SMF_CREATE_STATE(diagnostics_entry, diagnostics_run, diagnostics_exit, NULL, NULL),
};

static void boot_timer_expired(struct k_timer *timer) {
    k_event_post(&sm_data.smf_event, EVENT_BOOT_DONE);
}

#define BOOT_DISPLAY_DURATION_MS 10000

// Boot state handlers
static void boot_entry(void *o) {
    struct epd_sm_data *sm = (struct epd_sm_data *)o;
    LOG_INF("Entering boot state");
    k_timer_start(&sm->boot_timer, K_MSEC(BOOT_DISPLAY_DURATION_MS), K_NO_WAIT);
	execute_route(BOOT_ROUTE);
}

static void boot_run(void *o) {
    struct epd_sm_data *sm = (struct epd_sm_data *)o;

    if (sm->events & EVENT_BOOT_DONE) {
        smf_set_state(SMF_CTX(&sm->ctx), &display_states[CONFIG_STATE]);
    }
}

static void boot_exit(void *o) {
    struct epd_sm_data *sm = (struct epd_sm_data *)o;
    LOG_INF("Exiting boot state");
	boot_cleanup();
}

// Config state handlers
static void config_entry(void *o) {
    struct epd_sm_data *sm = (struct epd_sm_data *)o;
    LOG_INF("Entering config state");
    sm->current_state = CONFIG_STATE;
	execute_route(CONFIG_ROUTE);
}

static void config_run(void *o) {
    struct epd_sm_data *sm = (struct epd_sm_data *)o;

	display_manager_partial_update();

	if (sm->events & EVENT_KEY_0) {
		smf_set_state(SMF_CTX(&sm->ctx), &display_states[MOSAIC_STATE]);
	} else if (sm->events & EVENT_KEY_3) {
        smf_set_state(SMF_CTX(&sm->ctx), &display_states[NAME_TAG_STATE]);
	}
}

static void config_exit(void *o) {
    LOG_INF("Exiting config state");
    display_manager_suspend();
}

// Name tag state handlers
static void name_tag_entry(void *o) {

    struct epd_sm_data *sm = (struct epd_sm_data *)o;
    sm->current_state = NAME_TAG_STATE;

    LOG_INF("Entering name tag state");

    display_manager_resume();

	ui_manager_show_bottom_bar(false);
	nametag_display_show(config_get_selected());

	display_manager_suspend();
}

static void name_tag_run(void *o) {
    struct epd_sm_data *sm = (struct epd_sm_data *)o;

	if (ui_manager_is_bottom_bar_visible()) {
		if (sm->events & EVENT_KEY_0) {
			smf_set_state(SMF_CTX(&sm->ctx), &display_states[MOSAIC_STATE]);
		} else if (sm->events & EVENT_KEY_1) {
			smf_set_state(SMF_CTX(&sm->ctx), &display_states[CONFIG_STATE]);
		} else if (sm->events & EVENT_KEY_2) {
			smf_set_state(SMF_CTX(&sm->ctx), &display_states[DIAGNOSTICS_STATE]);
		} else if (sm->events & EVENT_KEY_3) {
			LOG_INF("Canceled");
			display_manager_resume();
			ui_manager_show_bottom_bar(false);
			display_manager_full_update();
			display_manager_suspend();
		}
	} else if (sm->events != 0 ) {
		LOG_INF("Button press event raised");
		display_manager_resume();
		ui_manager_show_bottom_bar(true);
		display_manager_full_update();
	}

	if (!ui_manager_is_bottom_bar_visible()) {
		if (display_manager_is_active()) {
			LOG_INF("Bar no longer visible, suspending");
			display_manager_suspend();
		}
	}
}

static void name_tag_exit(void *o) {
    LOG_INF("Exiting name tag state");
}

// Mosaic state handlers
static void mosaic_entry(void *o) {
    struct epd_sm_data *sm = (struct epd_sm_data *)o;
    LOG_INF("Entering mosaic state");
    display_manager_resume();
    sm->current_state = MOSAIC_STATE;
    // Auto transition back to config
    smf_set_state(SMF_CTX(&sm->ctx), &display_states[CONFIG_STATE]);
}

static void mosaic_exit(void *o) {
    LOG_INF("Exiting mosaic state");
}

// Button callback
static void buttons_callback(struct input_event *evt, void *user_data) {
    ARG_UNUSED(user_data);
    if (evt->type == INPUT_EV_KEY && evt->value == 1) {
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
static void state_manager_thread(void *p1, void *p2, void *p3) {
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);
    
    int err = ui_manager_init();
    if (err) {
        LOG_ERR("Failed to initialize UI manager: %d", err);
    }

    k_event_init(&sm_data.smf_event);
    k_timer_init(&sm_data.boot_timer, boot_timer_expired, NULL);
    
    smf_set_initial(SMF_CTX(&sm_data.ctx), &display_states[BOOT_STATE]);
    sm_data.current_state = BOOT_STATE;

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

K_THREAD_DEFINE(state_manager_thread_id, 8192, state_manager_thread, NULL, NULL, NULL, 7, 0, 0);
