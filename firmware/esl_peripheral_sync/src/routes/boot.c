
#include <zephyr/kernel.h>
#include <lvgl.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(boot, LOG_LEVEL_INF);

#include "routes.h"
#include "display_manager.h"
#include "ui_manager.h"
#include "images.h"

static lv_obj_t *display_image;

void handle_boot(void) {
	display_image = lv_img_create(lv_scr_act());
	lv_img_set_src(display_image, &nordic);
	display_manager_full_update();
}

void boot_cleanup(void) {
	lv_obj_del(display_image);
	display_image = NULL;
}
