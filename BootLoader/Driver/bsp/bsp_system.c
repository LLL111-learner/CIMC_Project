#include "bsp_system.h"

static void bsp_wkup_key_exti_init(void)
{
    rcu_periph_clock_enable(WK_UP_CLK_PORT);
    rcu_periph_clock_enable(RCU_SYSCFG);

    gpio_mode_set(WK_UP_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, WK_UP_PIN);

    syscfg_exti_line_config(EXTI_SOURCE_GPIOA, EXTI_SOURCE_PIN0);
    exti_init(EXTI_0, EXTI_INTERRUPT, EXTI_TRIG_BOTH);
    exti_interrupt_flag_clear(EXTI_0);
    nvic_irq_enable(EXTI0_IRQn, 1U, 0U);
}

static void bsp_usart_disable_for_deepsleep(void)
{
    usart_interrupt_disable(RS232_RS485_USART, USART_INT_IDLE);
    nvic_irq_disable(USART1_IRQn);
    usart_dma_receive_config(RS232_RS485_USART, USART_RECEIVE_DMA_DISABLE);
    usart_disable(RS232_RS485_USART);
}

static void bsp_gpio_enter_deepsleep_state(void)
{
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOD);
    rcu_periph_clock_enable(RS485_CS_PORT_RCU);

    gpio_mode_set(USART_RS_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, USART_RS_TX_PIN | USART_RS_RX_PIN);
    gpio_mode_set(RS485_CS_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, RS485_CS_PIN);
    gpio_mode_set(WK_UP_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, WK_UP_PIN);
}

static void bsp_deepsleep_reinit_after_wakeup(void)
{
    SystemInit();
    SystemCoreClockUpdate();
    systick_config();
    bsp_usart_init();
}

void bsp_enter_deepsleep(void)
{
    rcu_periph_clock_enable(RCU_PMU);

    __disable_irq();

    bsp_usart_disable_for_deepsleep();
    bsp_gpio_enter_deepsleep_state();
    bsp_wkup_key_exti_init();

    pmu_flag_clear(PMU_FLAG_RESET_WAKEUP);
    pmu_flag_clear(PMU_FLAG_RESET_STANDBY);

    SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;

    __enable_irq();

    pmu_to_deepsleepmode(PMU_LDO_LOWPOWER, PMU_LOWDRIVER_ENABLE, WFI_CMD);

    bsp_deepsleep_reinit_after_wakeup();
}


void bsp_usart_init(void)
{
    bsp_usart1_init_with_baudrate(19200U);
}

void bsp_usart1_init_with_baudrate(uint32_t baudrate)
{
    if(baudrate == 0UL) {
        baudrate = 19200U;
    }

    rcu_periph_clock_enable(USART_RS_PORT_RCU);
    rcu_periph_clock_enable(RS485_CS_PORT_RCU);
    rcu_periph_clock_enable(USART_RS_RCU);

    gpio_af_set(USART_RS_PORT, AF_USART1, USART_RS_TX_PIN | USART_RS_RX_PIN);
    gpio_mode_set(USART_RS_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, USART_RS_TX_PIN | USART_RS_RX_PIN);
    gpio_output_options_set(USART_RS_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, USART_RS_TX_PIN | USART_RS_RX_PIN);

    gpio_mode_set(RS485_CS_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, RS485_CS_PIN);
    gpio_output_options_set(RS485_CS_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, RS485_CS_PIN);
    RS485_CS_SET(0);

    usart_deinit(RS232_RS485_USART);
    usart_baudrate_set(RS232_RS485_USART, baudrate);
    usart_receive_config(RS232_RS485_USART, USART_RECEIVE_ENABLE);
    usart_transmit_config(RS232_RS485_USART, USART_TRANSMIT_ENABLE);
    usart_dma_receive_config(RS232_RS485_USART, USART_RECEIVE_DMA_DISABLE);
    usart_interrupt_disable(RS232_RS485_USART, USART_INT_IDLE);
    usart_enable(RS232_RS485_USART);
}
