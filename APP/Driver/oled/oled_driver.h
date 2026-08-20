#ifndef OLED_DRIVER_H
#define OLED_DRIVER_H

#include "gd32f4xx.h"
#include "gd32f4xx_dma.h"
#include "gd32f4xx_gpio.h"
#include "gd32f4xx_i2c.h"
#include "oled.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OLED_DAT_PORT          GPIOB
#define OLED_DAT_PIN_V2        GPIO_PIN_9
#define OLED_CLK_PORT_V2       GPIOB
#define OLED_CLK_PIN_V2        GPIO_PIN_8
#define AF_OLED_I2C0           GPIO_AF_4

#define I2C0_OWN_ADDRESS7      0x72
#define I2C0_SLAVE_ADDRESS7    0x82
#define I2C0_DATA_ADDRESS      (uint32_t)&I2C_DATA(I2C0)

#define OLED_PORT              OLED_DAT_PORT
#define OLED_CLK_PORT          RCU_GPIOB
#define OLED_DAT_PIN           OLED_DAT_PIN_V2
#define OLED_CLK_PIN           OLED_CLK_PIN_V2

extern __IO uint8_t oled_cmd_buf[2];
extern __IO uint8_t oled_data_buf[2];

void bsp_oled_init(void);
void bsp_oled_disable_for_deepsleep(void);

#ifdef __cplusplus
}
#endif

#endif
