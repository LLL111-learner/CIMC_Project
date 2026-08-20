#ifndef __USART_APP_H__
#define __USART_APP_H__

#include "bsp_system.h"

#ifdef __cplusplus
extern "C" {
#endif

int my_printf(uint32_t usart_periph, const char *format, ...);
void rs485_send_bytes(const uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif
