#ifndef __USART_APP_H__
#define __USART_APP_H__


#include "bsp_system.h"


#ifdef __cplusplus
extern "C" {
#endif

void rs485_send_bytes(const uint8_t *data, uint16_t len);
void uart_task(void);
void rs485_frame_callback(const uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif
