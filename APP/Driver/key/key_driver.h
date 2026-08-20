#ifndef KEY_DRIVER_H
#define KEY_DRIVER_H

#include "gd32f4xx.h"
#include "gd32f4xx_exti.h"
#include "gd32f4xx_gpio.h"
#include "gd32f4xx_syscfg.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define KEYE_PORT        GPIOE
#define KEYB_PORT        GPIOB
#define KEYA_PORT        GPIOA
#define KEYE_CLK_PORT    RCU_GPIOE
#define KEYB_CLK_PORT    RCU_GPIOB
#define KEYA_CLK_PORT    RCU_GPIOA

#define KEY1_PIN         GPIO_PIN_15
#define KEY2_PIN         GPIO_PIN_6
#define KEY3_PIN         GPIO_PIN_11
#define KEY4_PIN         GPIO_PIN_4
#define KEY5_PIN         GPIO_PIN_7
#define KEY6_PIN         GPIO_PIN_0
#define KEYW_PIN         GPIO_PIN_0

#define KEY1_READ        gpio_input_bit_get(KEYE_PORT, KEY1_PIN)
#define KEY2_READ        gpio_input_bit_get(KEYE_PORT, KEY2_PIN)
#define KEY3_READ        gpio_input_bit_get(KEYE_PORT, KEY3_PIN)
#define KEY4_READ        gpio_input_bit_get(KEYE_PORT, KEY4_PIN)
#define KEY5_READ        gpio_input_bit_get(KEYE_PORT, KEY5_PIN)
#define KEY6_READ        gpio_input_bit_get(KEYB_PORT, KEY6_PIN)
#define KEYW_READ        gpio_input_bit_get(KEYA_PORT, KEYW_PIN)

void bsp_btn_init(void);
void bsp_wkup_key_exti_init(void);

#ifdef __cplusplus
}
#endif

#endif
