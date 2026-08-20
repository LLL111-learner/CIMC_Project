#include "uart_driver.h"


uint8_t usart1_rxbuffer[RS485_UART_RXBUF_SIZE];

static uint32_t s_usart1_baudrate = USART_RS_BAUDRATE_DEFAULT;

void bsp_usart_init(uint32_t baudrate)
{
    bsp_usart1_init(baudrate);
}

void bsp_usart1_init(uint32_t baudrate)
{
    dma_single_data_parameter_struct dma_init_struct;

    if(baudrate == 0U) {
        baudrate = USART_RS_BAUDRATE_DEFAULT;
    }
    s_usart1_baudrate = baudrate;

    rcu_periph_clock_enable(RCU_DMA0);
    rcu_periph_clock_enable(RCU_GPIOD);
    rcu_periph_clock_enable(RS485_CS_PORT_RCU);
    rcu_periph_clock_enable(RCU_USART1);

    dma_deinit(USART1_RX_DMA_PERIPH, USART1_RX_DMA_CHANNEL);
    dma_init_struct.direction = DMA_PERIPH_TO_MEMORY;
    dma_init_struct.memory0_addr = (uint32_t)usart1_rxbuffer;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.number = sizeof(usart1_rxbuffer);
    dma_init_struct.periph_addr = USART1_RDATA_ADDRESS;
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    dma_init_struct.priority = DMA_PRIORITY_ULTRA_HIGH;
    dma_single_data_mode_init(USART1_RX_DMA_PERIPH, USART1_RX_DMA_CHANNEL, &dma_init_struct);

    dma_circulation_disable(USART1_RX_DMA_PERIPH, USART1_RX_DMA_CHANNEL);
    dma_channel_subperipheral_select(USART1_RX_DMA_PERIPH, USART1_RX_DMA_CHANNEL, USART1_RX_DMA_SUBPERI);
    dma_channel_enable(USART1_RX_DMA_PERIPH, USART1_RX_DMA_CHANNEL);

    gpio_af_set(USART1_TX_PORT, AF_USART1, USART1_TX_PIN | USART1_RX_PIN);
    gpio_mode_set(USART1_TX_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, USART1_TX_PIN | USART1_RX_PIN);
    gpio_output_options_set(USART1_TX_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, USART1_TX_PIN | USART1_RX_PIN);

    gpio_mode_set(RS485_CS_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, RS485_CS_PIN);
    gpio_output_options_set(RS485_CS_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, RS485_CS_PIN);
    RS485_CS_SET(0);

    usart_deinit(USART1);
    usart_baudrate_set(USART1, s_usart1_baudrate);
    usart_receive_config(USART1, USART_RECEIVE_ENABLE);
    usart_transmit_config(USART1, USART_TRANSMIT_ENABLE);
    usart_dma_receive_config(USART1, USART_RECEIVE_DMA_ENABLE);
    usart_enable(USART1);

    nvic_irq_enable(USART1_IRQn, 0, 0);
    usart_interrupt_enable(USART1, USART_INT_IDLE);
}

void bsp_usart_disable_for_deepsleep(void)
{
    usart_interrupt_disable(USART1, USART_INT_IDLE);
    nvic_irq_disable(USART1_IRQn);
    usart_dma_receive_config(USART1, USART_RECEIVE_DMA_DISABLE);
    dma_channel_disable(USART1_RX_DMA_PERIPH, USART1_RX_DMA_CHANNEL);
    usart_disable(USART1);
}

void bsp_usart_reinit_after_wakeup(void)
{
    bsp_usart1_init(s_usart1_baudrate);
}
