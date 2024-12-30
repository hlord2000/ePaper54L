#define STATUS_BAR_PERCENT 11  // Status bar takes 10% of height
#define CONTENT_PERCENT (100 - STATUS_BAR_PERCENT)
#define STATUS_HEIGHT ((Y_RESOLUTION * STATUS_BAR_PERCENT) / 100)
#define CONTENT_HEIGHT (Y_RESOLUTION - STATUS_HEIGHT)  // Remaining height
#define ICON_SPACING 20  // Space between icons
#define RIGHT_MARGIN 10  // Margin from right edge

void epd_create_layout() {
    // Get the active screen
    lv_obj_t *scr = lv_scr_act();
    
    // Set up the grid with fixed height values
    static lv_coord_t col_dsc[] = {X_RESOLUTION, LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {STATUS_HEIGHT, CONTENT_HEIGHT, LV_GRID_TEMPLATE_LAST};
    
    lv_obj_set_grid_dsc_array(scr, col_dsc, row_dsc);
    lv_obj_set_layout(scr, LV_LAYOUT_GRID);
    
    // Create top status bar container
    LOG_INF("Creating status bar");
    lv_obj_t *top_bar = lv_obj_create(scr); lv_obj_set_grid_cell(top_bar, LV_GRID_ALIGN_STRETCH, 0, 1,
                        LV_GRID_ALIGN_STRETCH, 0, 1);
    
    // Add name text on the left
    lv_obj_t *company_label = lv_label_create(top_bar);
    lv_label_set_text(company_label, "Nordic Semiconductor");
    lv_obj_align(company_label, LV_ALIGN_LEFT_MID, 10, 0);  // 10px margin from left
    
    // Battery icon (rightmost)
    lv_obj_t *battery_label = lv_label_create(top_bar);
    lv_label_set_text(battery_label, LV_SYMBOL_BATTERY_FULL);
    lv_obj_align(battery_label, LV_ALIGN_RIGHT_MID, -RIGHT_MARGIN, 0);

    // BLE icon (to the left of battery)
    lv_obj_t *ble_label = lv_label_create(top_bar);
    lv_label_set_text(ble_label, LV_SYMBOL_BLUETOOTH);
    lv_obj_align(ble_label, LV_ALIGN_RIGHT_MID, -(RIGHT_MARGIN + ICON_SPACING), 0);

    // Create main content area
    lv_obj_t *main_content = lv_obj_create(scr);
    lv_obj_set_grid_cell(main_content, LV_GRID_ALIGN_STRETCH, 0, 1,
                        LV_GRID_ALIGN_STRETCH, 1, 1);
    
    // Create and configure image
    lv_obj_t *image = lv_img_create(main_content);
    lv_obj_set_size(image, X_RESOLUTION, CONTENT_HEIGHT);
    lv_obj_center(image);
    lv_img_set_src(image, &boston);

    // Create floating container with black background
    static lv_style_t style_container;
    lv_style_init(&style_container);
    lv_style_set_bg_opa(&style_container, LV_OPA_COVER);
    lv_style_set_bg_color(&style_container, lv_color_black());
    lv_style_set_pad_top(&style_container, 10);
    lv_style_set_pad_bottom(&style_container, 10);
    lv_style_set_pad_left(&style_container, 15);
    lv_style_set_pad_right(&style_container, 15);
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
    lv_label_set_text(name_label, "Mariano");
    lv_obj_align(name_label, LV_ALIGN_TOP_LEFT, 0, 0);

    // Create location label
    static lv_style_t style_location;
    lv_style_init(&style_location);
    lv_style_set_text_font(&style_location, &lv_font_montserrat_16);
    lv_style_set_text_color(&style_location, lv_color_white());
    
    lv_obj_t *location_label = lv_label_create(float_container);
    lv_obj_add_style(location_label, &style_location, 0);
    lv_label_set_text(location_label, "Boston");
    lv_obj_align_to(location_label, name_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    
    lv_task_handler();
}


