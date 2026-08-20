#ifndef GD30AD3344_DRIVER_H
#define GD30AD3344_DRIVER_H

#include "gd32f4xx.h"
#include "gd32f4xx_dma.h"
#include "gd32f4xx_gpio.h"
#include "gd32f4xx_spi.h"
#include "gd30ad3344.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define S_SPI_CS_PORT          GPIOE
#define S_SPI_CS_PIN           GPIO_PIN_10

#define SPI3_SCK_PORT          GPIOE
#define SPI3_SCK_PIN           GPIO_PIN_12
#define SPI3_MISO_PORT         GPIOE
#define SPI3_MISO_PIN          GPIO_PIN_13
#define SPI3_MOSI_PORT         GPIOE
#define SPI3_MOSI_PIN          GPIO_PIN_14

#define AF_SPI3                GPIO_AF_5

#define SPI3_PORT              SPI3_SCK_PORT
#define SPI3_CLK_PORT          RCU_GPIOE

#define SPI3_NSS               S_SPI_CS_PIN
#define SPI3_SCK               SPI3_SCK_PIN
#define SPI3_MISO              SPI3_MISO_PIN
#define SPI3_MOSI              SPI3_MOSI_PIN

#define SPI_MODE_0             SPI_CK_PL_LOW_PH_1EDGE
#define SPI_MODE_1             SPI_CK_PL_LOW_PH_2EDGE
#define SPI_MODE_2             SPI_CK_PL_HIGH_PH_1EDGE
#define SPI_MODE_3             SPI_CK_PL_HIGH_PH_2EDGE

#define GD30_SPIMODE           SPI_MODE_1
#define ARRAYSIZE              (12U)

#define GD30_SPI               SPI3
#define GD30_DMA               DMA1
#define GD30_DMA_CHANNEL_TX    DMA_CH1
#define GD30_DMA_CHANNEL_RX    DMA_CH0
#define GD30_DMA_SUBPERI       DMA_SUBPERI4

#define GD30_DMA_RCU           RCU_DMA1
#define GD30_SPI_RCU           RCU_SPI3

#define GD30_SPI_PORT          SPI3_PORT
#define GD30_SPI_PORT_RCU      SPI3_CLK_PORT
#define GD30_SPI_SCK           SPI3_SCK
#define GD30_SPI_MISO          SPI3_MISO
#define GD30_SPI_MOSI          SPI3_MOSI

#define GD30_CS_PORT           S_SPI_CS_PORT
#define GD30_CS_PORT_RCU       RCU_GPIOE
#define GD30_CS_PIN            S_SPI_CS_PIN

#define GD30_CS_LOW()          gpio_bit_reset(GD30_CS_PORT, GD30_CS_PIN)
#define GD30_CS_HIGH()         gpio_bit_set(GD30_CS_PORT, GD30_CS_PIN)

extern uint8_t spi3_send_array[ARRAYSIZE];
extern uint8_t spi3_receive_array[ARRAYSIZE];

void bsp_gd30ad3344_init(void);
void bsp_gd30ad3344_disable_for_deepsleep(void);

#ifdef __cplusplus
}
#endif

#endif
