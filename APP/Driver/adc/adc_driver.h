#ifndef ADC_DRIVER_H
#define ADC_DRIVER_H

#include "gd32f4xx.h"
#include "gd32f4xx_adc.h"
#include "gd32f4xx_dma.h"
#include "gd32f4xx_gpio.h"
#include "systick.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ADC_IN10_PORT        GPIOC
#define ADC_IN10_PIN         GPIO_PIN_0
#define ADC_IN11_PORT        GPIOC
#define ADC_IN11_PIN         GPIO_PIN_1

#define ADC1_PORT            ADC_IN10_PORT
#define ADC1_CLK_PORT        RCU_GPIOC
#define ADC1_PIN             ADC_IN10_PIN

#define ADC_DAC_FB_PIN       ADC_IN11_PIN
#define ADC_DAC_FB_CHANNEL   ADC_CHANNEL_11

#define ADC_DMA              DMA1
#define ADC_DMA_RCU          RCU_DMA1
#define ADC_DMA_CHANNEL      DMA_CH4
#define ADC_DMA_SUBPERI      DMA_SUBPERI0

extern uint16_t adc_value[2];

void bsp_adc_init(void);
void bsp_adc_disable_for_deepsleep(void);

#ifdef __cplusplus
}
#endif

#endif
