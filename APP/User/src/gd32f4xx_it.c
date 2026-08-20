/*!
    \file    gd32f4xx_it.c
    \brief   interrupt service routines

    \version 2024-12-20, V3.3.1, firmware for GD32F4xx
*/

/*
    Copyright (c) 2024, GigaDevice Semiconductor Inc.

    Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its contributors
       may be used to endorse or promote products derived from this software without
       specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.
*/

#include "gd32f4xx_it.h"
#include "systick.h"
#include "string.h"
#include "uart_driver.h"
#include "usart_app.h"

extern uint8_t rs485_uart_dma_buffer[RS485_UART_RXBUF_SIZE];
extern __IO uint8_t rs485_rx_flag;
extern __IO uint16_t rs485_uart_dma_len;

/*!
    \brief      this function handles NMI exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void NMI_Handler(void)
{
    /* if NMI exception occurs, go to infinite loop */
    while(1) {
    }
}

/*!
    \brief      this function handles HardFault exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void HardFault_Handler(void)
{
    /* if Hard Fault exception occurs, go to infinite loop */
    while(1) {
    }
}

/*!
    \brief      this function handles MemManage exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void MemManage_Handler(void)
{
    /* if Memory Manage exception occurs, go to infinite loop */
    while(1) {
    }
}

/*!
    \brief      this function handles BusFault exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void BusFault_Handler(void)
{
    /* if Bus Fault exception occurs, go to infinite loop */
    while(1) {
    }
}

/*!
    \brief      this function handles UsageFault exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void UsageFault_Handler(void)
{
    /* if Usage Fault exception occurs, go to infinite loop */
    while(1) {
    }
}

/*!
    \brief      this function handles SVC exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void SVC_Handler(void)
{
    /* if SVC exception occurs, go to infinite loop */
    while(1) {
    }
}

/*!
    \brief      this function handles DebugMon exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void DebugMon_Handler(void)
{
    /* if DebugMon exception occurs, go to infinite loop */
    while(1) {
    }
}

/*!
    \brief      this function handles PendSV exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void PendSV_Handler(void)
{
    /* if PendSV exception occurs, go to infinite loop */
    while(1) {
    }
}

void USART1_IRQHandler(void)
{
    uint32_t rx_len;

    if(RESET != usart_interrupt_flag_get(USART1, USART_INT_FLAG_IDLE)) {
        usart_data_receive(USART1);

        rx_len = RS485_UART_RXBUF_SIZE - dma_transfer_number_get(USART1_RX_DMA_PERIPH, USART1_RX_DMA_CHANNEL);
        if((rx_len > 0U) && (rx_len <= RS485_UART_RXBUF_SIZE) && (rs485_rx_flag == 0U)) {
            memcpy(rs485_uart_dma_buffer, usart1_rxbuffer, rx_len);
            rs485_uart_dma_len = (uint16_t)rx_len;
            rs485_rx_flag = 1U;
        }

        memset(usart1_rxbuffer, 0, RS485_UART_RXBUF_SIZE);
        dma_channel_disable(USART1_RX_DMA_PERIPH, USART1_RX_DMA_CHANNEL);
        dma_flag_clear(USART1_RX_DMA_PERIPH, USART1_RX_DMA_CHANNEL, DMA_FLAG_FTF);
        dma_transfer_number_config(USART1_RX_DMA_PERIPH, USART1_RX_DMA_CHANNEL, RS485_UART_RXBUF_SIZE);
        dma_channel_enable(USART1_RX_DMA_PERIPH, USART1_RX_DMA_CHANNEL);
    }
}
void RTC_WKUP_IRQHandler(void)
{
    if(RESET != rtc_flag_get(RTC_FLAG_WT)) {
        rtc_flag_clear(RTC_FLAG_WT);
    }
    exti_interrupt_flag_clear(EXTI_22);
}

void EXTI0_IRQHandler(void)
{
    if(RESET != exti_interrupt_flag_get(EXTI_0)) {
        exti_interrupt_flag_clear(EXTI_0);
    }
}

/*! 
    \brief    this function handles SysTick exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void SysTick_Handler(void)
{
    delay_decrement();
}
