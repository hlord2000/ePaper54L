#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <lvgl.h>

// Display configuration
#define STATUS_HEIGHT 20
#define CONTENT_HEIGHT (Y_RESOLUTION - STATUS_HEIGHT)
#define RIGHT_MARGIN 10

// UI Component handles
typedef struct {
    lv_obj_t *screen;
    lv_obj_t *top_bar;
    lv_obj_t *main_content;
    lv_obj_t *bottom_bar;
    lv_obj_t *company_label;
    lv_obj_t *battery_label;
} ui_components_t;

// Bottom bar button configuration
typedef struct {
    const char *text;
    bool visible;
    lv_event_cb_t callback;
	int index;
} button_config_t;

/**
 * @brief Initialize the UI manager
 * 
 * @param display Display device pointer
 * @return int 0 on success, negative errno on failure
 */
int ui_manager_init();

/**
 * @brief Clean the main content area
 */
void ui_manager_clear_main(void);

/**
 * @brief Get pointer to main content area
 * 
 * @return lv_obj_t* Pointer to main content object
 */
lv_obj_t *ui_manager_get_main(void);

/**
 * @brief Update battery status icon
 * 
 * @param battery_symbol LVGL symbol string for battery
 */
void ui_manager_update_battery(const char *battery_symbol);

/**
 * @brief Update company name in top bar
 * 
 * @param name New company name string
 */
void ui_manager_update_company(const char *name);

/**
 * @brief Configure bottom bar buttons
 * 
 * @param buttons Array of button configurations
 * @param button_count Number of buttons
 */
void ui_manager_set_buttons(button_config_t *buttons, uint8_t button_count);

/**
 * @brief Show or hide bottom bar
 * 
 * @param show true to show, false to hide
 */
void ui_manager_show_bottom_bar(bool show);

bool ui_manager_is_bottom_bar_visible(void);

/**
 * @brief Perform full display update
 */
void ui_manager_full_update(void);

/**
 * @brief Perform partial display update
 */
void ui_manager_partial_update(void);

extern struct k_work ui_manager_full_update_work;
extern struct k_work ui_manager_partial_update_work;

/**
 * @brief Resume display from suspended state
 * 
 * @return int 0 on success, negative errno on failure
 */
int ui_manager_resume(void);

/**
 * @brief Suspend display
 * 
 * @return int 0 on success, negative errno on failure
 */
int ui_manager_suspend(void);

bool ui_manager_is_active(void) {

#endif /* UI_MANAGER_H */
