#include "gd30ad3344.h"

extern uint8_t spi3_send_array[ARRAYSIZE];    
extern uint8_t spi3_receive_array[ARRAYSIZE]; 


uint8_t spi_gd30ad3344_send_byte_dma(uint8_t byte)
{
    spi3_send_array[0] = byte;
    
    dma_single_data_parameter_struct dma_init_struct;
    
    dma_deinit(GD30_DMA, GD30_DMA_CHANNEL_TX);
    dma_init_struct.periph_addr         = (uint32_t)&SPI_DATA(GD30_SPI);
    dma_init_struct.memory0_addr        = (uint32_t)spi3_send_array;
    dma_init_struct.direction           = DMA_MEMORY_TO_PERIPH;
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    dma_init_struct.priority            = DMA_PRIORITY_HIGH;
    dma_init_struct.number              = 1; 
    dma_init_struct.periph_inc          = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.memory_inc          = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.circular_mode       = DMA_CIRCULAR_MODE_DISABLE;
    dma_single_data_mode_init(GD30_DMA, GD30_DMA_CHANNEL_TX, &dma_init_struct);
    dma_channel_subperipheral_select(GD30_DMA, GD30_DMA_CHANNEL_TX, GD30_DMA_SUBPERI);
    
    dma_deinit(GD30_DMA, GD30_DMA_CHANNEL_RX);
    dma_init_struct.periph_addr         = (uint32_t)&SPI_DATA(GD30_SPI);
    dma_init_struct.memory0_addr        = (uint32_t)spi3_receive_array;
    dma_init_struct.direction           = DMA_PERIPH_TO_MEMORY;
    dma_init_struct.priority            = DMA_PRIORITY_HIGH;
    dma_single_data_mode_init(GD30_DMA, GD30_DMA_CHANNEL_RX, &dma_init_struct);
    dma_channel_subperipheral_select(GD30_DMA, GD30_DMA_CHANNEL_RX, GD30_DMA_SUBPERI);
    
    dma_channel_enable(GD30_DMA, GD30_DMA_CHANNEL_RX);
    dma_channel_enable(GD30_DMA, GD30_DMA_CHANNEL_TX);
    
    spi_dma_enable(GD30_SPI, SPI_DMA_RECEIVE);
    spi_dma_enable(GD30_SPI, SPI_DMA_TRANSMIT);
    
    while(RESET == dma_flag_get(GD30_DMA, GD30_DMA_CHANNEL_RX, DMA_FLAG_FTF));
    
    spi_dma_disable(GD30_SPI, SPI_DMA_RECEIVE);
    spi_dma_disable(GD30_SPI, SPI_DMA_TRANSMIT);
    dma_channel_disable(GD30_DMA, GD30_DMA_CHANNEL_RX);
    dma_channel_disable(GD30_DMA, GD30_DMA_CHANNEL_TX);
    
    dma_flag_clear(GD30_DMA, GD30_DMA_CHANNEL_RX, DMA_FLAG_FTF);
    dma_flag_clear(GD30_DMA, GD30_DMA_CHANNEL_TX, DMA_FLAG_FTF);
    
    return spi3_receive_array[0];
}

uint16_t spi_gd30ad3344_send_halfword_dma(uint16_t half_word)
{
    GD30_CS_LOW();
    uint16_t rx_data;
    
    spi3_send_array[0] = (uint8_t)(half_word >> 8);
    spi3_send_array[1] = (uint8_t)half_word;
    
    dma_single_data_parameter_struct dma_init_struct;
    
    dma_deinit(GD30_DMA, GD30_DMA_CHANNEL_TX);
    dma_init_struct.periph_addr         = (uint32_t)&SPI_DATA(GD30_SPI);
    dma_init_struct.memory0_addr        = (uint32_t)spi3_send_array;
    dma_init_struct.direction           = DMA_MEMORY_TO_PERIPH;
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    dma_init_struct.priority            = DMA_PRIORITY_HIGH;
    dma_init_struct.number              = 2; 
    dma_init_struct.periph_inc          = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.memory_inc          = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.circular_mode       = DMA_CIRCULAR_MODE_DISABLE;
    dma_single_data_mode_init(GD30_DMA, GD30_DMA_CHANNEL_TX, &dma_init_struct);
    dma_channel_subperipheral_select(GD30_DMA, GD30_DMA_CHANNEL_TX, GD30_DMA_SUBPERI);
    
    dma_deinit(GD30_DMA, GD30_DMA_CHANNEL_RX);
    dma_init_struct.periph_addr         = (uint32_t)&SPI_DATA(GD30_SPI);
    dma_init_struct.memory0_addr        = (uint32_t)spi3_receive_array;
    dma_init_struct.direction           = DMA_PERIPH_TO_MEMORY;
    dma_init_struct.priority            = DMA_PRIORITY_HIGH;
    dma_single_data_mode_init(GD30_DMA, GD30_DMA_CHANNEL_RX, &dma_init_struct);
    dma_channel_subperipheral_select(GD30_DMA, GD30_DMA_CHANNEL_RX, GD30_DMA_SUBPERI);
    
    dma_channel_enable(GD30_DMA, GD30_DMA_CHANNEL_RX);
    dma_channel_enable(GD30_DMA, GD30_DMA_CHANNEL_TX);
    
    spi_dma_enable(GD30_SPI, SPI_DMA_RECEIVE);
    spi_dma_enable(GD30_SPI, SPI_DMA_TRANSMIT);
    
    while(RESET == dma_flag_get(GD30_DMA, GD30_DMA_CHANNEL_RX, DMA_FLAG_FTF));
    
    spi_dma_disable(GD30_SPI, SPI_DMA_RECEIVE);
    spi_dma_disable(GD30_SPI, SPI_DMA_TRANSMIT);
    dma_channel_disable(GD30_DMA, GD30_DMA_CHANNEL_RX);
    dma_channel_disable(GD30_DMA, GD30_DMA_CHANNEL_TX);
    
    dma_flag_clear(GD30_DMA, GD30_DMA_CHANNEL_RX, DMA_FLAG_FTF);
    dma_flag_clear(GD30_DMA, GD30_DMA_CHANNEL_TX, DMA_FLAG_FTF);
    
    rx_data = (uint16_t)(spi3_receive_array[0] << 8);
    rx_data |= spi3_receive_array[1];
    GD30_CS_HIGH();
    return rx_data;
}


void spi_gd30ad3344_transmit_receive_dma(uint8_t *tx_buffer, uint8_t *rx_buffer, uint16_t size)
{
    
    if (size > ARRAYSIZE) {
        size = ARRAYSIZE;
    }
    
    for (uint16_t i = 0; i < size; i++) {
        spi3_send_array[i] = tx_buffer[i];
    }
    
    dma_single_data_parameter_struct dma_init_struct;
    
    dma_deinit(GD30_DMA, GD30_DMA_CHANNEL_TX);
    dma_init_struct.periph_addr         = (uint32_t)&SPI_DATA(GD30_SPI);
    dma_init_struct.memory0_addr        = (uint32_t)spi3_send_array;
    dma_init_struct.direction           = DMA_MEMORY_TO_PERIPH;
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    dma_init_struct.priority            = DMA_PRIORITY_HIGH;
    dma_init_struct.number              = size;
    dma_init_struct.periph_inc          = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.memory_inc          = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.circular_mode       = DMA_CIRCULAR_MODE_DISABLE;
    dma_single_data_mode_init(GD30_DMA, GD30_DMA_CHANNEL_TX, &dma_init_struct);
    dma_channel_subperipheral_select(GD30_DMA, GD30_DMA_CHANNEL_TX, GD30_DMA_SUBPERI);
    
    dma_deinit(GD30_DMA, GD30_DMA_CHANNEL_RX);
    dma_init_struct.periph_addr         = (uint32_t)&SPI_DATA(GD30_SPI);
    dma_init_struct.memory0_addr        = (uint32_t)spi3_receive_array;
    dma_init_struct.direction           = DMA_PERIPH_TO_MEMORY;
    dma_init_struct.priority            = DMA_PRIORITY_HIGH;
    dma_single_data_mode_init(GD30_DMA, GD30_DMA_CHANNEL_RX, &dma_init_struct);
    dma_channel_subperipheral_select(GD30_DMA, GD30_DMA_CHANNEL_RX, GD30_DMA_SUBPERI);
    
    dma_channel_enable(GD30_DMA, GD30_DMA_CHANNEL_RX);
    dma_channel_enable(GD30_DMA, GD30_DMA_CHANNEL_TX);
    
    spi_dma_enable(GD30_SPI, SPI_DMA_RECEIVE);
    spi_dma_enable(GD30_SPI, SPI_DMA_TRANSMIT);
    
    while(RESET == dma_flag_get(GD30_DMA, GD30_DMA_CHANNEL_RX, DMA_FLAG_FTF));
    
    spi_dma_disable(GD30_SPI, SPI_DMA_RECEIVE);
    spi_dma_disable(GD30_SPI, SPI_DMA_TRANSMIT);
    dma_channel_disable(GD30_DMA, GD30_DMA_CHANNEL_RX);
    dma_channel_disable(GD30_DMA, GD30_DMA_CHANNEL_TX);
    
    dma_flag_clear(GD30_DMA, GD30_DMA_CHANNEL_RX, DMA_FLAG_FTF);
    dma_flag_clear(GD30_DMA, GD30_DMA_CHANNEL_TX, DMA_FLAG_FTF);
    
    for (uint16_t i = 0; i < size; i++) {
        rx_buffer[i] = spi3_receive_array[i];
    }
}


void spi_gd30ad3344_wait_for_dma_end(void)
{
    while(RESET == dma_flag_get(GD30_DMA, GD30_DMA_CHANNEL_RX, DMA_FLAG_FTF));
    
    dma_flag_clear(GD30_DMA, GD30_DMA_CHANNEL_RX, DMA_FLAG_FTF);
    dma_flag_clear(GD30_DMA, GD30_DMA_CHANNEL_TX, DMA_FLAG_FTF);
}


GD30AD3344 GD30AD3344_InitStruct;

void GD30AD3344_Init(void)
{
    GD30AD3344_InitStruct.SS         = GD30AD3344_OS_DISABLE;
    GD30AD3344_InitStruct.MUX        = GD30AD3344_MUX_AIN0_GND;
                                               
    GD30AD3344_InitStruct.PGA        = GD30AD3344_PGA_2V048;
                                               
    GD30AD3344_InitStruct.MODE       = GD30AD3344_MODE_CONTINUOUS;
    GD30AD3344_InitStruct.DR         = GD30AD3344_DR_25SPS;
                                               
    GD30AD3344_InitStruct.RESERVED_1 = GD30AD3344_RESERVED_0;
    GD30AD3344_InitStruct.PULL_UP_EN = GD30AD3344_PULL_UP_DISABLE;
    GD30AD3344_InitStruct.NOP        = GD30AD3344_NOP_VALID_UPDATE;
    GD30AD3344_InitStruct.RESERVED   = GD30AD3344_RESERVED_1;
    
    spi_enable(GD30_SPI);
    spi_gd30ad3344_send_halfword_dma(GD30AD3344_InitStruct_Value);
}

float ADS118_PGA_SET(GD30AD3344_PGA_TypeDef PGA)
{
    switch(PGA) {
    case GD30AD3344_PGA_6V144:
        return 6.144f;
    case GD30AD3344_PGA_4V096:
        return 4.096f;
    case GD30AD3344_PGA_2V048:
        return 2.048f;
    case GD30AD3344_PGA_1V024:
        return 1.024f;
    case GD30AD3344_PGA_0V512:
        return 0.512f;
    case GD30AD3344_PGA_0V256:
        return 0.256f;
    case GD30AD3344_PGA_0V064:
        return 0.064f;
    default:
        return 2.048f;
    }
}

float GD30AD3344_AD_Read(GD30AD3344_Channel_TypeDef CH, GD30AD3344_PGA_TypeDef Ref)
{
    uint16_t raw_data;
    float result = 0.0;

    GD30AD3344_InitStruct.MUX = CH;
    GD30AD3344_InitStruct.PGA = Ref;

    raw_data = spi_gd30ad3344_send_halfword_dma(GD30AD3344_InitStruct_Value);
    
    result = (float)((int16_t)raw_data) * ADS118_PGA_SET(Ref) / 32768.0f;
    return (float)result;
}
