#include "bsp_system.h"
#include "bl_core.h"

int main(void)
{
    systick_config();
    bsp_usart_init();

    bootloader_run();

    while(1) {
        bsp_enter_deepsleep();     //低功耗兜底
        bootloader_run();
    }
}
