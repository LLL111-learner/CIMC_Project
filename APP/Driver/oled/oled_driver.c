#include "oled_driver.h"


__IO uint8_t oled_cmd_buf[2] = {0x00, 0x00};
__IO uint8_t oled_data_buf[2] = {0x40, 0x00};

void bsp_oled_init(void)
{
    dma_single_data_parameter_struct dma_init_struct;

    rcu_periph_clock_enable(OLED_CLK_PORT);
    rcu_periph_clock_enable(RCU_I2C0);
    rcu_periph_clock_enable(RCU_DMA0);

    gpio_af_set(OLED_PORT, AF_OLED_I2C0, OLED_DAT_PIN);
    gpio_af_set(OLED_PORT, AF_OLED_I2C0, OLED_CLK_PIN);

    gpio_mode_set(OLED_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, OLED_DAT_PIN);
    gpio_output_options_set(OLED_PORT, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, OLED_DAT_PIN);
    gpio_mode_set(OLED_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, OLED_CLK_PIN);
    gpio_output_options_set(OLED_PORT, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, OLED_CLK_PIN);

    i2c_clock_config(I2C0, 400000, I2C_DTCY_2);
    i2c_mode_addr_config(I2C0, I2C_I2CMODE_ENABLE, I2C_ADDFORMAT_7BITS, I2C0_OWN_ADDRESS7);
    i2c_enable(I2C0);
    i2c_ack_config(I2C0, I2C_ACK_ENABLE);

    dma_deinit(DMA0, DMA_CH6);

    dma_single_data_para_struct_init(&dma_init_struct);
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPH;
    dma_init_struct.memory0_addr = (uint32_t)oled_data_buf;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    dma_init_struct.number = 2;
    dma_init_struct.periph_addr = I2C0_DATA_ADDRESS;
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.priority = DMA_PRIORITY_ULTRA_HIGH;
    dma_single_data_mode_init(DMA0, DMA_CH6, &dma_init_struct);

    dma_circulation_disable(DMA0, DMA_CH6);
    dma_channel_subperipheral_select(DMA0, DMA_CH6, DMA_SUBPERI1);
}

void bsp_oled_disable_for_deepsleep(void)
{
    OLED_Display_Off();

    i2c_dma_config(I2C0, I2C_DMA_OFF);
    dma_channel_disable(DMA0, DMA_CH6);
    i2c_disable(I2C0);

    gpio_mode_set(OLED_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, OLED_DAT_PIN | OLED_CLK_PIN);
}
