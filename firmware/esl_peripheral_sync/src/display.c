#include <zephyr/kernel.h>
#include <zephyr/smf.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h> 
#include <zephyr/pm/device.h>
#include <zephyr/drivers/display.h> 
#include <zephyr/input/input.h>
#include <lvgl.h>

#include "images.h"

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(display, LOG_LEVEL);

/* Event definitions */
#define EVENT_KEY_0     BIT(0)
#define EVENT_KEY_1     BIT(1)
#define EVENT_KEY_2     BIT(2)
#define EVENT_KEY_3     BIT(3)
#define EVENT_BOOT_DONE BIT(4)

#define BOOT_DISPLAY_DURATION_MS 5000  // 5 seconds boot display duration

const struct device *display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
#define X_RESOLUTION (int)DT_PROP(DT_CHOSEN(zephyr_display), width)
#define Y_RESOLUTION (int)DT_PROP(DT_CHOSEN(zephyr_display), height)

#define STATUS_HEIGHT 20
#define CONTENT_HEIGHT (Y_RESOLUTION - STATUS_HEIGHT)  // Remaining height
#define ICON_SPACING 20  // Space between icons
#define RIGHT_MARGIN 10  // Margin from right edge

static bool display_active = true;

#if defined(CONFIG_PM_DEVICE)
static int epd_suspend(const struct device *dev) {
    int err = pm_device_action_run(dev, PM_DEVICE_ACTION_SUSPEND);
    if (err) {
        LOG_ERR("Failed to suspend display: %d", err);
        return err;
    }
    display_active = false;
    LOG_INF("Display suspended");
    return 0;
}

static int epd_resume(const struct device *dev) {
    int err = pm_device_action_run(dev, PM_DEVICE_ACTION_RESUME);
    if (err) {
        LOG_ERR("Failed to resume display: %d", err);
        return err;
    }
    display_active = true;
    LOG_INF("Display resumed");
    return 0;
}
#endif

static void epd_partial_update(void) {
    lv_task_handler();
}

static void epd_full_update(void) {
    display_blanking_on(display_dev);
    lv_task_handler();
    display_blanking_off(display_dev);
}

static void epd_set_image(const lv_img_dsc_t *image) {
    LOG_INF("Setting image");
    lv_obj_t *display_image = lv_img_create(lv_scr_act());
    lv_img_set_src(display_image, image);
    return;    
}

enum display_states { BOOT, CONFIG, NAME_SELECTOR, NAME_TAG, MOSAIC };

// State machine context structure
struct display_sm_data {
    struct smf_ctx ctx;
    struct k_event smf_event;
    struct k_timer boot_timer;
    uint32_t events;
    uint8_t current_state;
};

// Global state machine context
static struct display_sm_data sm_data;

// Forward declarations for state handlers
static void boot_entry(void *o);
static void boot_run(void *o);
static void boot_exit(void *o);

static void config_entry(void *o);
static void config_run(void *o);
static void config_exit(void *o);

static void name_select_entry(void *o);
static void name_select_exit(void *o);
static void name_tag_entry(void *o);
static void name_tag_exit(void *o);

static void mosaic_entry(void *o);
static void mosaic_exit(void *o);

static void boot_timer_expired(struct k_timer *timer) {
    k_event_post(&sm_data.smf_event, EVENT_BOOT_DONE);
}

// State machine state definitions
static const struct smf_state display_states[] = {
    [BOOT] = SMF_CREATE_STATE(boot_entry, boot_run, boot_exit, NULL, NULL),
    [CONFIG] = SMF_CREATE_STATE(config_entry, config_run, config_exit, NULL, NULL),
    [NAME_SELECTOR] = SMF_CREATE_STATE(name_select_entry, NULL, name_select_exit, NULL, NULL),
    [NAME_TAG] = SMF_CREATE_STATE(name_tag_entry, NULL, name_tag_exit, NULL, NULL),
    [MOSAIC] = SMF_CREATE_STATE(mosaic_entry, NULL, mosaic_exit, NULL, NULL),
};

// Boot state handlers
static void boot_entry(void *o) {
    struct display_sm_data *sm = (struct display_sm_data *)o;

    LOG_INF("Entering boot state");
    
    // Start boot display timer
    k_timer_start(&sm->boot_timer, K_MSEC(BOOT_DISPLAY_DURATION_MS), K_NO_WAIT);

	// Resume display
	epd_resume(display_dev);
}

static void boot_run(void *o) {
    struct display_sm_data *sm = (struct display_sm_data *)o;

    LOG_INF("Running boot state");

	static bool image_set = false;
	if (!image_set) {
		epd_set_image(&nordic);
		epd_full_update();
		image_set = true;
	}
    
    if (sm->events & EVENT_BOOT_DONE) {
        smf_set_state(SMF_CTX(&sm->ctx), &display_states[CONFIG]);
    }
	return;
}

static void boot_exit(void *o) {
    struct display_sm_data *sm = (struct display_sm_data *)o;

    LOG_INF("Exiting boot state");

    k_timer_stop(&sm->boot_timer);
	epd_suspend(display_dev);
}

// Config state handlers
static void config_entry(void *o) {
    struct display_sm_data *sm = (struct display_sm_data *)o;

    LOG_INF("Entering config state");

    sm->current_state = CONFIG;

	epd_resume(display_dev);

	lv_obj_clean(lv_scr_act());

    // Get the active screen
    lv_obj_t *scr = lv_scr_act();
    
    // Set up the grid with fixed height values
    static lv_coord_t col_dsc[] = {X_RESOLUTION, LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {STATUS_HEIGHT, CONTENT_HEIGHT, LV_GRID_TEMPLATE_LAST};
    
    lv_obj_set_grid_dsc_array(scr, col_dsc, row_dsc);
    lv_obj_set_layout(scr, LV_LAYOUT_GRID);
   	
    // Create top status bar container
    LOG_INF("Creating status bar");
    lv_obj_t *top_bar = lv_obj_create(scr); 
	lv_obj_remove_style_all(top_bar);

	lv_obj_set_grid_cell(top_bar, LV_GRID_ALIGN_STRETCH, 0, 1,
                        LV_GRID_ALIGN_STRETCH, 0, 1);
    
    // Add name text on the left
    lv_obj_t *company_label = lv_label_create(top_bar);
    lv_label_set_text(company_label, "Nordic Semiconductor");
	lv_obj_set_style_text_font(company_label, &lv_font_montserrat_14, 0);
	lv_obj_set_style_text_color(company_label, lv_color_make(0, 0, 0), 0);
    lv_obj_align(company_label, LV_ALIGN_LEFT_MID, 10, 0);  // 10px margin from left
    
    // Battery icon 
    lv_obj_t *battery_label = lv_label_create(top_bar);
    lv_label_set_text(battery_label, LV_SYMBOL_BATTERY_FULL);
	lv_obj_set_style_text_font(battery_label, &lv_font_montserrat_16, 0);
	lv_obj_set_style_text_color(battery_label, lv_color_make(0, 0, 0), 0);
    lv_obj_align(battery_label, LV_ALIGN_RIGHT_MID, -RIGHT_MARGIN, 0);

    // Create main content area
    lv_obj_t *main_content = lv_obj_create(scr);
    lv_obj_set_grid_cell(main_content, LV_GRID_ALIGN_STRETCH, 0, 1,
                        LV_GRID_ALIGN_STRETCH, 1, 1);
 
	epd_full_update();
	
	epd_suspend(display_dev);
}

static void config_run(void *o) {
    struct display_sm_data *sm = (struct display_sm_data *)o;

    LOG_INF("Running config state");

    // Handle transitions based on any existing events
    if (sm->events & EVENT_KEY_0) {
        smf_set_state(SMF_CTX(&sm->ctx), &display_states[NAME_SELECTOR]);
    } else if (sm->events & EVENT_KEY_1) {
        smf_set_state(SMF_CTX(&sm->ctx), &display_states[NAME_TAG]);
    } else if (sm->events & EVENT_KEY_2) {
        smf_set_state(SMF_CTX(&sm->ctx), &display_states[MOSAIC]);
    }
	return;
}

static void config_exit(void *o) {
    LOG_INF("Exiting config state");
	epd_suspend(display_dev);
}

// Name selector state handlers
static void name_select_entry(void *o) {
    struct display_sm_data *sm = (struct display_sm_data *)o;
    LOG_INF("Entering name selector state");
    epd_resume(display_dev);
    sm->current_state = NAME_SELECTOR;
    
    // Handle transition if event is present
    if (sm->events & EVENT_KEY_3) {
        smf_set_state(SMF_CTX(&sm->ctx), &display_states[NAME_TAG]);
    }
}

static void name_select_exit(void *o) {
    LOG_INF("Exiting name selector state");
}

// Name tag state handlers
static void name_tag_entry(void *o) {
    struct display_sm_data *sm = (struct display_sm_data *)o;
    LOG_INF("Entering name tag state");
    epd_resume(display_dev);
    sm->current_state = NAME_TAG;
    // Auto transition back to config
    smf_set_state(SMF_CTX(&sm->ctx), &display_states[CONFIG]);
}

static void name_tag_exit(void *o) {
    LOG_INF("Exiting name tag state");
}

// Mosaic state handlers
static void mosaic_entry(void *o) {
    struct display_sm_data *sm = (struct display_sm_data *)o;
    LOG_INF("Entering mosaic state");
    epd_resume(display_dev);
    sm->current_state = MOSAIC;
    // Auto transition back to config
    smf_set_state(SMF_CTX(&sm->ctx), &display_states[CONFIG]);
}

static void mosaic_exit(void *o) {
    LOG_INF("Exiting mosaic state");
}

static const struct device *const buttons_dev = DEVICE_DT_GET(DT_NODELABEL(buttons));

// Button callback that posts events
static void buttons_callback(struct input_event *evt, void *user_data) {
    ARG_UNUSED(user_data);
    if (evt->type == INPUT_EV_KEY) {
        switch(evt->code) {
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

// Display initialization
static int epd_init(void) {
    if (!device_is_ready(display_dev)) {
        LOG_ERR("Display device not ready");
        return -1;
    }
    display_blanking_off(display_dev);
    
#if defined(CONFIG_PM_DEVICE)
    epd_suspend(display_dev);
#endif
    return 0;
}

// State machine initialization
static int display_sm_init(void) {
    LOG_INF("Initializing display state machine");
    
    // Initialize event system
    k_event_init(&sm_data.smf_event);
    LOG_DBG("Event system initialized");
    
    // Initialize boot timer
    k_timer_init(&sm_data.boot_timer, boot_timer_expired, NULL);
    LOG_DBG("Boot timer initialized");
    
    // Initialize state machine
#if 0
    smf_set_initial(SMF_CTX(&sm_data.ctx), &display_states[BOOT]);
    sm_data.current_state = BOOT;
#else
    smf_set_initial(SMF_CTX(&sm_data.ctx), &display_states[CONFIG]);
    sm_data.current_state = CONFIG;
#endif
    sm_data.events = 0;
    LOG_INF("State machine initialized to BOOT state");
    
    return 0;
}

// State machine thread
static void epd_thread(void *p1, void *p2, void *p3) {
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);
    
    LOG_INF("EPD thread starting, running initial state machine");
    
    // Run state machine immediately to handle boot state
    if (smf_run_state(SMF_CTX(&sm_data.ctx))) {
        LOG_ERR("Initial state machine run failed");
    }

    while (1) {
        // Wait for any event (button or timer)
        LOG_DBG("Waiting for events...");
        sm_data.events = k_event_wait(&sm_data.smf_event,
                                    EVENT_KEY_0 | EVENT_KEY_1 | EVENT_KEY_2 | 
                                    EVENT_KEY_3 | EVENT_BOOT_DONE,
                                    true, K_MSEC(100));
		if (sm_data.events != 0) {
			LOG_INF("Got events: 0x%x", sm_data.events);
		}
        
        // Run state machine
        if (smf_run_state(SMF_CTX(&sm_data.ctx))) {
            LOG_ERR("State machine run failed");
        }
        
        LOG_DBG("Clearing events");
        sm_data.events = 0;
    }
}

// Thread definition with 8192 stack size
K_THREAD_DEFINE(epd_thread_id, 8192, epd_thread, NULL, NULL, NULL, 7, 0, 0);

SYS_INIT(epd_init, APPLICATION, 90);
SYS_INIT(display_sm_init, APPLICATION, 99);
