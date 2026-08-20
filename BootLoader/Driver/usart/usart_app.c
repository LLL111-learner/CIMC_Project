#include "usart_app.h"

__IO uint16_t tx_count = 0;

int my_printf(uint32_t usart_periph, const char *format, ...)
{
    char buffer[512];
    va_list arg;
    int len;

    va_start(arg, format);
    len = vsnprintf(buffer, sizeof(buffer), format, arg);
    va_end(arg);
    
    for(tx_count = 0; tx_count < len; tx_count++){
        usart_data_transmit(usart_periph, buffer[tx_count]);
        while(RESET == usart_flag_get(usart_periph, USART_FLAG_TBE));
    }
    return len;
}

void rs485_send_bytes(const uint8_t *data, uint16_t len)
{
    uint16_t i;

    if((data == NULL) || (len == 0U)) {
        return;
    }

    RS485_CS_SET(1);
    for(i = 0U; i < len; i++) {
        usart_data_transmit(RS232_RS485_USART, data[i]);
        while(RESET == usart_flag_get(RS232_RS485_USART, USART_FLAG_TBE)) {
        }
    }
    while(RESET == usart_flag_get(RS232_RS485_USART, USART_FLAG_TC)) {
    }
    RS485_CS_SET(0);
}
