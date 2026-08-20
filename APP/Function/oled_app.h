#ifndef __OLED_APP_H__
#define __OLED_APP_H__

#include "bsp_system.h"

#ifdef __cplusplus
extern "C" {
#endif

int oled_printf(uint8_t x, uint8_t y, const char *format, ...);
void oled_show_bootloader_status(void);
void oled_task(void);

#ifdef __cplusplus
}
#endif

#endif


