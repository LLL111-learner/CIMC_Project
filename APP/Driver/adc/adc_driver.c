#include "adc_driver.h"


uint16_t adc_value[2];

void bsp_adc_init(void)
{
    dma_single_data_parameter_struct dma_single_data_parameter;

    rcu_periph_clock_enable(ADC1_CLK_PORT);
    rcu_periph_clock_enable(RCU_ADC0);
    rcu_periph_clock_enable(ADC_DMA_RCU);

    adc_clock_config(ADC_ADCCK_PCLK2_DIV8);

    gpio_mode_set(ADC1_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, ADC1_PIN);
    gpio_mode_set(ADC_IN11_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, ADC_DAC_FB_PIN);

    dma_deinit(ADC_DMA, ADC_DMA_CHANNEL);

    dma_single_data_parameter.periph_addr = (uint32_t)(&ADC_RDATA(ADC0));
    dma_single_data_parameter.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_single_data_parameter.memory0_addr = (uint32_t)(adc_value);
    dma_single_data_parameter.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_single_data_parameter.periph_memory_width = DMA_PERIPH_WIDTH_16BIT;
    dma_single_data_parameter.direction = DMA_PERIPH_TO_MEMORY;
    dma_single_data_parameter.number = 2;
    dma_single_data_parameter.priority = DMA_PRIORITY_HIGH;
    dma_single_data_mode_init(ADC_DMA, ADC_DMA_CHANNEL, &dma_single_data_parameter);
    dma_channel_subperipheral_select(ADC_DMA, ADC_DMA_CHANNEL, ADC_DMA_SUBPERI);

    dma_circulation_enable(ADC_DMA, ADC_DMA_CHANNEL);
    dma_channel_enable(ADC_DMA, ADC_DMA_CHANNEL);

    adc_sync_mode_config(ADC_SYNC_MODE_INDEPENDENT);
    adc_special_function_config(ADC0, ADC_CONTINUOUS_MODE, ENABLE);
    adc_special_function_config(ADC0, ADC_SCAN_MODE, ENABLE);
    adc_data_alignment_config(ADC0, ADC_DATAALIGN_RIGHT);

    adc_channel_length_config(ADC0, ADC_ROUTINE_CHANNEL, 2);
    adc_routine_channel_config(ADC0, 0, ADC_CHANNEL_10, ADC_SAMPLETIME_15);
    adc_routine_channel_config(ADC0, 1, ADC_DAC_FB_CHANNEL, ADC_SAMPLETIME_15);
    adc_external_trigger_source_config(ADC0, ADC_ROUTINE_CHANNEL, ADC_EXTTRIG_ROUTINE_T0_CH0);
    adc_external_trigger_config(ADC0, ADC_ROUTINE_CHANNEL, EXTERNAL_TRIGGER_DISABLE);

    adc_dma_request_after_last_enable(ADC0);
    adc_dma_mode_enable(ADC0);

    adc_enable(ADC0);
    delay_1ms(1);
    adc_calibration_enable(ADC0);

    adc_software_trigger_enable(ADC0, ADC_ROUTINE_CHANNEL);
}

void bsp_adc_disable_for_deepsleep(void)
{
    adc_disable(ADC0);
    adc_dma_mode_disable(ADC0);
    dma_channel_disable(ADC_DMA, ADC_DMA_CHANNEL);
}
