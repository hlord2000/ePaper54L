#include <lvgl.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nametag, LOG_LEVEL_INF);

#include "images.h"
#include "ui_manager.h"
#include "display_manager.h"


#define X_RESOLUTION (int)DT_PROP(DT_CHOSEN(zephyr_display), width)
#define Y_RESOLUTION (int)DT_PROP(DT_CHOSEN(zephyr_display), height)
#define CONTENT_HEIGHT (Y_RESOLUTION - STATUS_HEIGHT)

typedef struct {
    const char *name;
    const char *location;
    const lv_img_dsc_t *image;
} nametag_data_t;

void nametag_display_show(uint8_t index);
void nametag_display_next(void);
void nametag_display_previous(void);

const nametag_data_t nametags[] = {
	{
		.name = "Geir",
		.location = "Trondheim",
		.image = &trondheim,
	},
	{
		.name = "Helmut",
		.location = "Spartanburg",
		.image = &spartanburg,
	},
	{
		.name = "Jennifer",
		.location = "San Jose",
		.image = &sanjose,
	},
	{
		.name = "Mariano",
		.location = "Boston",
		.image = &boston,
	},
	{
		.name = "Mike",
		.location = "San Jose",
		.image = &sanjose,
	},
	{
		.name = "Vegard",
		.location = "Trondheim",
		.image = &trondheim,
	},
	{
		.name = "Wes",
		.location = "Philadelphia",
		.image = &philadelphia,
	},
};


/*
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
*/

static button_config_t buttons[] = {
	{
		.text = "Mosaic",
		.callback = NULL,
		.visible = true,
		.index = 0,
	},
	{
		.text = "Config",
		.callback = NULL,
		.visible = true,
		.index = 1,
	},
	{
		.text = "Diag.",
		.callback = NULL,
		.visible = true,
		.index = 2,
	},
	{
		.text = "Cancel",
		.callback = NULL,
		.visible = true,
		.index = 3,
	},
};

static const size_t NUM_NAMETAGS = sizeof(nametags) / sizeof(nametags[0]);
static uint8_t current_nametag_index = 0;

static void update_main_content(const nametag_data_t *nametag) {
    lv_obj_t *main_content = ui_manager_get_main();
    ui_manager_clear_main();

    // Create and configure image
    lv_obj_t *image = lv_img_create(main_content);
    lv_obj_set_size(image, X_RESOLUTION, CONTENT_HEIGHT);
    lv_obj_center(image);
    lv_img_set_src(image, nametag->image);

    // Create floating container with black background
    static lv_style_t style_container;
    lv_style_init(&style_container);
    lv_style_set_bg_opa(&style_container, LV_OPA_COVER);
    lv_style_set_bg_color(&style_container, lv_color_black());
    lv_style_set_pad_all(&style_container, 5);
    lv_style_set_radius(&style_container, 0);

    lv_obj_t *float_container = lv_obj_create(main_content);
    lv_obj_clear_flag(float_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_style(float_container, &style_container, 0);
    lv_obj_set_size(float_container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(float_container, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    // Create name label
    static lv_style_t style_name;
    lv_style_init(&style_name);
    lv_style_set_text_font(&style_name, &lv_font_montserrat_24);
    lv_style_set_text_color(&style_name, lv_color_white());

    lv_obj_t *name_label = lv_label_create(float_container);
    lv_obj_add_style(name_label, &style_name, 0);
    lv_label_set_text(name_label, nametag->name);
    lv_obj_align(name_label, LV_ALIGN_TOP_LEFT, 0, 0);

    // Create location label
    static lv_style_t style_location;
    lv_style_init(&style_location);
    lv_style_set_text_font(&style_location, &lv_font_montserrat_16);
    lv_style_set_text_color(&style_location, lv_color_white());

    lv_obj_t *location_label = lv_label_create(float_container);
    lv_obj_add_style(location_label, &style_location, 0);
    lv_label_set_text(location_label, nametag->location);
    lv_obj_align_to(location_label, name_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);

    display_manager_full_update();

	ui_manager_set_buttons(buttons, sizeof(buttons)/sizeof(button_config_t));
	ui_manager_show_bottom_bar(false);
}

void nametag_display_show(uint8_t index) {
    if (index >= NUM_NAMETAGS) {
        return;
    }
    current_nametag_index = index;
    update_main_content(&nametags[current_nametag_index]);
}

void nametag_display_next(void) {
    current_nametag_index = (current_nametag_index + 1) % NUM_NAMETAGS;
    update_main_content(&nametags[current_nametag_index]);
}

void nametag_display_previous(void) {
    current_nametag_index = (current_nametag_index == 0) ? NUM_NAMETAGS - 1 : current_nametag_index - 1;
    update_main_content(&nametags[current_nametag_index]);
}

size_t nametag_get_string(char *str, size_t length) {
    size_t accumulated_length = 0;
    str[0] = '\0';  // Initialize string to empty

    for (int i = 0; i < NUM_NAMETAGS; i++) {
        size_t name_length = strlen(nametags[i].name);
        size_t delimiter_length = (i < NUM_NAMETAGS - 1) ? 1 : 0;
        
        if (name_length + delimiter_length + accumulated_length >= length) {
            return accumulated_length;
        }
        
        strcat(str, nametags[i].name);
        accumulated_length += name_length;
        
        if (i < NUM_NAMETAGS - 1) {
            strcat(str, "\n");
            accumulated_length += delimiter_length;
        }
    }
    
    return accumulated_length;
}
