#ifndef NAMETAG_H__
#define NAMETAG_H__

#include <stdint.h>
#include <string.h>

void nametag_display_show(uint8_t index);
void nametag_display_next(void);
void nametag_display_previous(void);
size_t nametag_get_string(char *str, size_t length);

#endif
