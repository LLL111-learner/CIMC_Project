#ifndef DAC_DRIVER_H
#define DAC_DRIVER_H

#include "gd32f4xx.h"
#include "gd32f4xx_dac.h"
#include "gd32f4xx_gpio.h"
#include "gd32f4xx_dma.h"
#include "gd32f4xx_timer.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DAC0_OUT_PORT        GPIOA
#define DAC0_OUT_PIN         GPIO_PIN_4
#define DAC1_OUT_PORT        GPIOA
#define DAC1_OUT_PIN         GPIO_PIN_5

#define CONVERT_NUM          (1)
#define DAC0_R12DH_ADDRESS   (0x40007408)

#define DAC1_PORT            DAC0_OUT_PORT
#define DAC1_CLK_PORT        RCU_GPIOA
#define DAC1_PIN             DAC0_OUT_PIN

extern uint16_t convertarr[CONVERT_NUM];

void bsp_dac_init(void);
void bsp_dac_disable_for_deepsleep(void);

#ifdef __cplusplus
}
#endif

#endif
