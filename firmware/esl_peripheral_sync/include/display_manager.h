#ifndef DISPLAY_MANAGER_H__
#define DISPLAY_MANAGER_H__

#include <zephyr/device.h>
#include <lvgl.h>

void display_manager_full_update(void);

void display_manager_partial_update(void);

int display_manager_resume(void);

int display_manager_suspend(void);

bool display_manager_is_active(void);

#endif /* DISPLAY_MANAGER_H__ */
