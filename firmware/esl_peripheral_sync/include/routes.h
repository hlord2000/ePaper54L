#ifndef ROUTES_H__
#define ROUTES_H__

#include "ui_manager.h"

enum routes {
	BOOT_ROUTE,
	CONFIG_ROUTE,
	NAMETAG_ROUTE,
	MOSAIC_ROUTE,
	DIAGNOSTICS_ROUTE,
};

struct route_ctx {
	enum routes route;
	button_config_t *bottom_bar_buttons;
};

typedef void (*route_handler_t)(void);

void handle_boot(void);
void handle_config(void);

static route_handler_t route_handlers[] = {
	handle_boot,
    handle_config,
#if 0
    handle_nametag,
    handle_mosaic,
    handle_diagnostics,
#endif
};

static inline void execute_route(enum routes route) {
	ui_manager_clear_main();
	route_handlers[route]();
}

/* BOOT FUNCTIONS */
void boot_cleanup(void);

/* CONFIG FUNCTIONS */
int config_get_selected(void);

#endif
