#ifndef UART_DRIVER_H
#define UART_DRIVER_H

#include "gd32f4xx.h"
#include "gd32f4xx_dma.h"
#include "gd32f4xx_gpio.h"
#include "gd32f4xx_usart.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define USART1_RX_PORT             GPIOD
#define USART1_RX_PIN              GPIO_PIN_6
#define USART1_TX_PORT             GPIOD
#define USART1_TX_PIN              GPIO_PIN_5
#define AF_USART1                  GPIO_AF_7

#define USART1_RDATA_ADDRESS       ((uint32_t)&USART_DATA(USART1))

#define USART_RS_PORT              USART1_TX_PORT
#define USART_RS_PORT_RCU          RCU_GPIOD
#define USART_RS_RX_PORT           USART1_RX_PORT
#define USART_RS_RX_PIN            USART1_RX_PIN
#define USART_RS_TX_PORT           USART1_TX_PORT
#define USART_RS_TX_PIN            USART1_TX_PIN
#define USART_RS_RX                USART_RS_RX_PIN
#define USART_RS_TX                USART_RS_TX_PIN

#define RS485_CS_PORT              GPIOE
#define RS485_CS_PORT_RCU          RCU_GPIOE
#define RS485_CS_PIN               GPIO_PIN_8
#define RS485_CS_SET(x)            do { if(x) GPIO_BOP(RS485_CS_PORT) = RS485_CS_PIN; else GPIO_BC(RS485_CS_PORT) = RS485_CS_PIN; } while(0)
#define RS485_UART_RXBUF_SIZE      256U

#define USART1_RX_DMA_PERIPH       DMA0
#define USART1_RX_DMA_CHANNEL      DMA_CH5
#define USART1_RX_DMA_SUBPERI      DMA_SUBPERI4

#define DMA_RS_USART               USART1_RX_DMA_PERIPH
#define DMA_RS_USART_CHANNEL_RX    USART1_RX_DMA_CHANNEL
#define DMA_RS_USART_SUBPERI_RX    USART1_RX_DMA_SUBPERI
#define RS232_RS485_USART          (USART1)
#define USART_RS_RDATA_ADDRESS     USART1_RDATA_ADDRESS
#define USART_RS_RCU               RCU_USART1
#define USART_RS_BAUDRATE_DEFAULT  19200U

extern uint8_t usart1_rxbuffer[RS485_UART_RXBUF_SIZE];

void bsp_usart_init(uint32_t baudrate);
void bsp_usart1_init(uint32_t baudrate);
void bsp_usart_disable_for_deepsleep(void);
void bsp_usart_reinit_after_wakeup(void);

#ifdef __cplusplus
}
#endif

#endif
