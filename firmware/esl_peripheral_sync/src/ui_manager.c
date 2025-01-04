#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/pm/device.h>
#include <zephyr/drivers/display.h>
#include <lvgl.h>

#include "ui_manager.h"
#include "display_manager.h"

#include <zephyr/logging/log.h>
#define LOG_LEVEL 4
LOG_MODULE_REGISTER(ui_manager);

// Display resolution from devicetree
#define X_RESOLUTION (int)DT_PROP(DT_CHOSEN(zephyr_display), width)
#define Y_RESOLUTION (int)DT_PROP(DT_CHOSEN(zephyr_display), height)
#define CONTENT_HEIGHT (Y_RESOLUTION - STATUS_HEIGHT)

static ui_components_t ui_components;
static bool display_active = true;

static void create_base_layout(void) {
    lv_obj_t *scr = lv_scr_act();
    
    // Set up the grid with fixed height values
    static lv_coord_t col_dsc[] = {X_RESOLUTION, LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {STATUS_HEIGHT, CONTENT_HEIGHT, LV_GRID_TEMPLATE_LAST};
    
    lv_obj_set_grid_dsc_array(scr, col_dsc, row_dsc);
    lv_obj_set_layout(scr, LV_LAYOUT_GRID);
    
    ui_components.screen = scr;
}

static void create_top_bar(void) {
    ui_components.top_bar = lv_obj_create(ui_components.screen);
    lv_obj_set_grid_cell(ui_components.top_bar, LV_GRID_ALIGN_STRETCH, 0, 1,
                         LV_GRID_ALIGN_STRETCH, 0, 1);

    // Company label
    ui_components.company_label = lv_label_create(ui_components.top_bar);
    lv_label_set_text(ui_components.company_label, "Nordic Semiconductor");
    lv_obj_set_style_text_font(ui_components.company_label, &lv_font_montserrat_14, 0);
    lv_obj_align(ui_components.company_label, LV_ALIGN_LEFT_MID, 10, 0);

    // Battery label
    ui_components.battery_label = lv_label_create(ui_components.top_bar);
    lv_label_set_text(ui_components.battery_label, LV_SYMBOL_BATTERY_FULL);
    lv_obj_set_style_text_font(ui_components.battery_label, &lv_font_montserrat_16, 0);
    lv_obj_align(ui_components.battery_label, LV_ALIGN_RIGHT_MID, -RIGHT_MARGIN, 0);
}

static void create_main_content(void) {
    ui_components.main_content = lv_obj_create(ui_components.screen);
    lv_obj_set_grid_cell(ui_components.main_content, LV_GRID_ALIGN_STRETCH, 0, 1,
                         LV_GRID_ALIGN_STRETCH, 1, 1);
}

int ui_manager_init() {
    // Initialize UI components
	LOG_INF("Creating base layout");
    create_base_layout();
	LOG_INF("Creating top bar");
    create_top_bar();
	LOG_INF("Creating main content");
    create_main_content();

    return 0;
}

void ui_manager_clear_main(void) {
	LOG_INF("Clearing main content");
    if (!display_active) {
        return;
    }
    lv_obj_clean(ui_components.main_content);
}

lv_obj_t *ui_manager_get_main(void) {
    return ui_components.main_content;
}

void ui_manager_update_battery(const char *battery_symbol) {
    if (!display_active) {
        return;
    }
    lv_label_set_text(ui_components.battery_label, battery_symbol);
}

void ui_manager_update_company(const char *name) {
    if (!display_active) {
        return;
    }
    lv_label_set_text(ui_components.company_label, name);
}

void ui_manager_set_buttons(button_config_t *buttons, uint8_t button_count) {
    if (!display_active || !buttons) {
        return;
    }

    // Remove existing bottom bar if it exists
    if (ui_components.bottom_bar) {
        lv_obj_del(ui_components.bottom_bar);
        ui_components.bottom_bar = NULL;
    }

    // Create new bottom bar if we have buttons to show
    if (button_count > 0) {
        // Create on screen but force it to the bottom
        ui_components.bottom_bar = lv_obj_create(lv_scr_act());
        
        // Clear all styles and layout settings
        lv_obj_remove_style_all(ui_components.bottom_bar);
        lv_obj_clear_flag(ui_components.bottom_bar, LV_OBJ_FLAG_SCROLLABLE);
        
        // Set exact size and align to bottom
        lv_obj_set_size(ui_components.bottom_bar, X_RESOLUTION, 20);
		lv_obj_add_flag(ui_components.bottom_bar, LV_OBJ_FLAG_FLOATING);
        lv_obj_set_align(ui_components.bottom_bar, LV_ALIGN_BOTTOM_MID);
        
        // Calculate button width including gaps
        lv_coord_t button_width = (X_RESOLUTION / button_count) - 1;

        for (uint8_t i = 0; i < button_count; i++) {
            if (buttons[i].visible) {
                lv_obj_t *btn = lv_btn_create(ui_components.bottom_bar);
                lv_obj_remove_style_all(btn);  // Clear default styles
                
                // Set size and position
                lv_obj_set_size(btn, button_width, 20);
                
                // Style the button
                lv_obj_set_style_bg_color(btn, lv_color_black(), 0);
                lv_obj_set_style_bg_opa(btn, 255, 0);
                lv_obj_set_style_border_width(btn, 0, 0);
                lv_obj_set_style_pad_all(btn, 0, 0);
                
                // Position from left
                lv_obj_align(btn, LV_ALIGN_LEFT_MID, i * (button_width + 1), 0);

                // Create and style label
                lv_obj_t *label = lv_label_create(btn);
                lv_label_set_text(label, buttons[i].text);
                lv_obj_set_style_text_color(label, lv_color_white(), 0);
                lv_obj_center(label);

                if (buttons[i].callback) {
                    lv_obj_add_event_cb(btn, buttons[i].callback, LV_EVENT_CLICKED, &buttons[i].index);
                }
            }
        }
    }
	lv_obj_update_layout(ui_components.bottom_bar);
	LOG_INF("Screen height: %d", Y_RESOLUTION);
	LOG_INF("Bottom bar y-pos: %d", lv_obj_get_y(ui_components.bottom_bar));
}

void ui_manager_show_bottom_bar(bool show) {
    if (ui_components.bottom_bar) {
        if (show) {
            lv_obj_clear_flag(ui_components.bottom_bar, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(ui_components.bottom_bar, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

bool ui_manager_is_bottom_bar_visible(void) {
	return !lv_obj_has_flag(ui_components.bottom_bar, LV_OBJ_FLAG_HIDDEN);
}

/*
int ui_manager_set_route(enum routes route) {
	
}
*/
