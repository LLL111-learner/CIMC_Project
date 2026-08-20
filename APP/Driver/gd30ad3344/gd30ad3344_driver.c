#include "gd30ad3344_driver.h"
#include "gd30ad3344.h"
#include "gd32f4xx_spi.h"

uint8_t spi3_send_array[ARRAYSIZE] = {0};
uint8_t spi3_receive_array[ARRAYSIZE] = {0};

void bsp_gd30ad3344_init(void)
{
    spi_parameter_struct spi_init_struct;

    rcu_periph_clock_enable(GD30_SPI_PORT_RCU);
    rcu_periph_clock_enable(GD30_CS_PORT_RCU);
    rcu_periph_clock_enable(GD30_SPI_RCU);
    rcu_periph_clock_enable(GD30_DMA_RCU);

    gpio_af_set(GD30_SPI_PORT, AF_SPI3, GD30_SPI_SCK | GD30_SPI_MISO | GD30_SPI_MOSI);
    gpio_mode_set(GD30_SPI_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE,
                  GD30_SPI_SCK | GD30_SPI_MISO | GD30_SPI_MOSI);
    gpio_output_options_set(GD30_SPI_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                            GD30_SPI_SCK | GD30_SPI_MISO | GD30_SPI_MOSI);

    gpio_mode_set(GD30_CS_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GD30_CS_PIN);
    gpio_output_options_set(GD30_CS_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GD30_CS_PIN);
    GD30_CS_HIGH();

    spi_init_struct.trans_mode = SPI_TRANSMODE_FULLDUPLEX;
    spi_init_struct.device_mode = SPI_MASTER;
    spi_init_struct.frame_size = SPI_FRAMESIZE_8BIT;
    spi_init_struct.clock_polarity_phase = GD30_SPIMODE;
    spi_init_struct.nss = SPI_NSS_SOFT;
    spi_init_struct.prescale = SPI_PSC_8;
    spi_init_struct.endian = SPI_ENDIAN_MSB;
    spi_init(GD30_SPI, &spi_init_struct);

    GD30AD3344_Init();
}

void bsp_gd30ad3344_disable_for_deepsleep(void)
{
    GD30_CS_HIGH();

    spi_dma_disable(GD30_SPI, SPI_DMA_RECEIVE);
    spi_dma_disable(GD30_SPI, SPI_DMA_TRANSMIT);

    dma_channel_disable(GD30_DMA, GD30_DMA_CHANNEL_RX);
    dma_channel_disable(GD30_DMA, GD30_DMA_CHANNEL_TX);

    spi_disable(GD30_SPI);
}
