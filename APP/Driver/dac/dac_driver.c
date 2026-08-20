#include "dac_driver.h"
#include "gd32f4xx_dac.h"
#include "gd32f4xx_dma.h"
#include "gd32f4xx_timer.h"

uint16_t convertarr[CONVERT_NUM] = {0};

void bsp_dac_init(void)
{
    rcu_periph_clock_enable(DAC1_CLK_PORT);
    rcu_periph_clock_enable(RCU_DAC);

    gpio_mode_set(DAC1_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, DAC1_PIN);

    dac_deinit(DAC0);
    dac_trigger_disable(DAC0, DAC_OUT0);
    dac_wave_mode_config(DAC0, DAC_OUT0, DAC_WAVE_DISABLE);

    dac_enable(DAC0, DAC_OUT0);
    dac_data_set(DAC0, DAC_OUT0, DAC_ALIGN_12B_R, convertarr[0]);
}

void bsp_dac_disable_for_deepsleep(void)
{
    dac_disable(DAC0, DAC_OUT0);
    dac_dma_disable(DAC0, DAC_OUT0);
    dma_channel_disable(DMA0, DMA_CH5);
    timer_disable(TIMER5);
}
