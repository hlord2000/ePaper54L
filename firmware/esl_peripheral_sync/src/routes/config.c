#include <zephyr/kernel.h>
#include <lvgl.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(config, LOG_LEVEL_INF);

#include "routes.h"
#include "display_manager.h"
#include "ui_manager.h"
#include "nametag.h"

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

void handle_config(void) {
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
    
    display_manager_full_update();
}

int config_get_selected(void) {
	return lv_roller_get_selected(config_roller);
}
