#ifndef BSP_SYSTEM_H
#define BSP_SYSTEM_H

#include "gd32f4xx.h"
#include "systick.h"
#include "usart_app.h"

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>



#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************************************************/
/* Wakeup key */
#define WK_UP_PORT                GPIOA
#define WK_UP_PIN                 GPIO_PIN_0
#define WK_UP_CLK_PORT            RCU_GPIOA

/***************************************************************************************************************/
/* RS485 uses USART1: TX=PD5, RX=PD6 */
#define USART1_RX_PORT            GPIOD
#define USART1_RX_PIN             GPIO_PIN_6
#define USART1_TX_PORT            GPIOD
#define USART1_TX_PIN             GPIO_PIN_5

#define AF_USART1                 GPIO_AF_7

/***************************************************************************************************************/
/* USART1 RS485 channel */
#define DEBUG_USART               (USART1)
#define USART1_RDATA_ADDRESS      ((uint32_t)&USART_DATA(USART1))

#define USART_RS_PORT             USART1_TX_PORT
#define USART_RS_PORT_RCU         RCU_GPIOD
#define USART_RS_RX_PORT          USART1_RX_PORT
#define USART_RS_RX_PIN           USART1_RX_PIN
#define USART_RS_TX_PORT          USART1_TX_PORT
#define USART_RS_TX_PIN           USART1_TX_PIN
#define USART_RS_RX               USART_RS_RX_PIN
#define USART_RS_TX               USART_RS_TX_PIN

#define RS485_CS_PORT             GPIOE
#define RS485_CS_PORT_RCU         RCU_GPIOE
#define RS485_CS_PIN              GPIO_PIN_8
#define RS485_CS_SET(x)           do { if(x) GPIO_BOP(RS485_CS_PORT) = RS485_CS_PIN; else GPIO_BC(RS485_CS_PORT) = RS485_CS_PIN; } while(0)

#define RS232_RS485_USART         (USART1)
#define USART_RS_RDATA_ADDRESS    USART1_RDATA_ADDRESS
#define USART_RS_RCU              RCU_USART1

void bsp_usart1_init_with_baudrate(uint32_t baudrate);
void bsp_usart_init(void);
void bsp_enter_deepsleep(void);

/***************************************************************************************************************/

#ifdef __cplusplus
  }
#endif

#endif /* BSP_SYSTEM_H */
