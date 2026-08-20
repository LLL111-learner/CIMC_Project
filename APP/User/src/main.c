#include "bsp_system.h"


int main(void)
{
    SCB->VTOR = 0x08011000UL;

    systick_config();
    init_cycle_counter(false);

    bsp_led_init();
    bsp_btn_init();
    bsp_oled_init();
    cimc_param_init();
    bsp_usart_init(cimc_system_baud_code_to_value(cimc_system_get_baud_code()));
    cimc_protocol_init();
    
    (void)cimc_protocol_send_heartbeat();//发送心跳

    bsp_gd30ad3344_init();//外设初始化
    bsp_adc_init();
    bsp_dac_init();
    bsp_rtc_init();
    OLED_Init();

    cimc_data_init();//系统协议初始化
    cimc_alarm_init();
    cimc_system_init();
    scheduler_init();
	
    while(1) {
        scheduler_run();
    }
}
