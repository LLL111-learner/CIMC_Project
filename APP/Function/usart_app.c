#include "usart_app.h"


__IO uint8_t rs485_rx_flag = 0;
__IO uint16_t rs485_uart_dma_len = 0;
uint8_t rs485_uart_dma_buffer[RS485_UART_RXBUF_SIZE] = {0};

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

void rs485_frame_callback(const uint8_t *data, uint16_t len)
{
    cimc_protocol_process_bytes(data, len);
}

void uart_task(void)
{
    if(rs485_rx_flag) {
        rs485_frame_callback(rs485_uart_dma_buffer, rs485_uart_dma_len);
        memset(rs485_uart_dma_buffer, 0, sizeof(rs485_uart_dma_buffer));
        rs485_uart_dma_len = 0;
        rs485_rx_flag = 0;
    }

    cimc_protocol_task();
}
