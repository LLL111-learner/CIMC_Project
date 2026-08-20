#include "power_driver.h"


void bsp_gpio_enter_deepsleep_state(void)
{
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_GPIOC);
    rcu_periph_clock_enable(RCU_GPIOD);
    rcu_periph_clock_enable(RCU_GPIOE);

    LED1_OFF;
    LED2_OFF;
    LED3_OFF;
    LED4_OFF;
    LED5_OFF;
    LED6_OFF;

    gpio_mode_set(KEYE_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE,
                  KEY1_PIN | KEY2_PIN | KEY3_PIN | KEY4_PIN | KEY5_PIN);
    gpio_mode_set(KEYB_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, KEY6_PIN);

    gpio_mode_set(USART1_TX_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, USART1_TX_PIN);
    gpio_mode_set(USART1_RX_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, USART1_RX_PIN);
    gpio_mode_set(OLED_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, OLED_DAT_PIN | OLED_CLK_PIN);

    gpio_mode_set(GD30_SPI_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE,
                  GD30_SPI_SCK | GD30_SPI_MISO | GD30_SPI_MOSI);
    gpio_mode_set(GD30_CS_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GD30_CS_PIN);
    gpio_output_options_set(GD30_CS_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, GD30_CS_PIN);
    GD30_CS_HIGH();

    gpio_mode_set(ADC1_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, ADC1_PIN);
    gpio_mode_set(ADC_IN11_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, ADC_DAC_FB_PIN);
    gpio_mode_set(DAC1_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, DAC1_PIN);

    gpio_mode_set(KEYA_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, KEYW_PIN);
}

void bsp_deepsleep_reinit_after_wakeup(void)
{
    SystemInit();
    SCB->VTOR = 0x08011000UL;
    SystemCoreClockUpdate();
    systick_config();
    update_perf_counter();
    bsp_led_init();
    bsp_btn_init();
    bsp_usart_reinit_after_wakeup();
    bsp_oled_init();
    OLED_Init();
    bsp_adc_init();
    bsp_dac_init();
    bsp_gd30ad3344_init();
}

void bsp_enter_deepsleep_for_seconds(uint16_t seconds)
{
    rcu_periph_clock_enable(RCU_PMU);

    __disable_irq();

    bsp_usart_disable_for_deepsleep();
    bsp_oled_disable_for_deepsleep();
    bsp_gd30ad3344_disable_for_deepsleep();
    bsp_adc_disable_for_deepsleep();
    bsp_dac_disable_for_deepsleep();

    bsp_gpio_enter_deepsleep_state();
    bsp_wkup_key_exti_init();
    bsp_rtc_wakeup_config(seconds);

    pmu_flag_clear(PMU_FLAG_RESET_WAKEUP);
    pmu_flag_clear(PMU_FLAG_RESET_STANDBY);

    before_cycle_counter_reconfiguration();
    SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;

    __enable_irq();

    pmu_to_deepsleepmode(PMU_LDO_LOWPOWER, PMU_LOWDRIVER_ENABLE, WFI_CMD);

    bsp_rtc_wakeup_deconfig();
    bsp_deepsleep_reinit_after_wakeup();
}
