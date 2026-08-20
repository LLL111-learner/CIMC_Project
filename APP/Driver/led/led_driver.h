#ifndef LED_DRIVER_H
#define LED_DRIVER_H

#include "gd32f4xx.h"
#include "gd32f4xx_gpio.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LED_PORT        GPIOD
#define LED_CLK_PORT    RCU_GPIOD

#define LED1_PIN        GPIO_PIN_10
#define LED2_PIN        GPIO_PIN_11
#define LED3_PIN        GPIO_PIN_12
#define LED4_PIN        GPIO_PIN_13
#define LED5_PIN        GPIO_PIN_14
#define LED6_PIN        GPIO_PIN_15

#define LED_ACTIVE_HIGH 1

#if LED_ACTIVE_HIGH
#define LED_WRITE(pin, on) do { if(on) GPIO_BOP(LED_PORT) = (pin); else GPIO_BC(LED_PORT) = (pin); } while(0)
#else
#define LED_WRITE(pin, on) do { if(on) GPIO_BC(LED_PORT) = (pin); else GPIO_BOP(LED_PORT) = (pin); } while(0)
#endif

#define LED1_SET(x)     do { LED_WRITE(LED1_PIN, (x)); } while(0)
#define LED2_SET(x)     do { LED_WRITE(LED2_PIN, (x)); } while(0)
#define LED3_SET(x)     do { LED_WRITE(LED3_PIN, (x)); } while(0)
#define LED4_SET(x)     do { LED_WRITE(LED4_PIN, (x)); } while(0)
#define LED5_SET(x)     do { LED_WRITE(LED5_PIN, (x)); } while(0)
#define LED6_SET(x)     do { LED_WRITE(LED6_PIN, (x)); } while(0)

#define LED1_TOGGLE     do { GPIO_TG(LED_PORT) = LED1_PIN; } while(0)
#define LED2_TOGGLE     do { GPIO_TG(LED_PORT) = LED2_PIN; } while(0)
#define LED3_TOGGLE     do { GPIO_TG(LED_PORT) = LED3_PIN; } while(0)
#define LED4_TOGGLE     do { GPIO_TG(LED_PORT) = LED4_PIN; } while(0)
#define LED5_TOGGLE     do { GPIO_TG(LED_PORT) = LED5_PIN; } while(0)
#define LED6_TOGGLE     do { GPIO_TG(LED_PORT) = LED6_PIN; } while(0)

#define LED1_ON         do { LED_WRITE(LED1_PIN, 1); } while(0)
#define LED2_ON         do { LED_WRITE(LED2_PIN, 1); } while(0)
#define LED3_ON         do { LED_WRITE(LED3_PIN, 1); } while(0)
#define LED4_ON         do { LED_WRITE(LED4_PIN, 1); } while(0)
#define LED5_ON         do { LED_WRITE(LED5_PIN, 1); } while(0)
#define LED6_ON         do { LED_WRITE(LED6_PIN, 1); } while(0)

#define LED1_OFF        do { LED_WRITE(LED1_PIN, 0); } while(0)
#define LED2_OFF        do { LED_WRITE(LED2_PIN, 0); } while(0)
#define LED3_OFF        do { LED_WRITE(LED3_PIN, 0); } while(0)
#define LED4_OFF        do { LED_WRITE(LED4_PIN, 0); } while(0)
#define LED5_OFF        do { LED_WRITE(LED5_PIN, 0); } while(0)
#define LED6_OFF        do { LED_WRITE(LED6_PIN, 0); } while(0)

void bsp_led_init(void);

#ifdef __cplusplus
}
#endif

#endif
