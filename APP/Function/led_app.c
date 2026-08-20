#include "led_app.h"


uint8_t ucLed[6] = {0, 0, 0, 0, 0, 0};

#define LED_STATUS_TOGGLE_MS 1000U

void led_disp(uint8_t *ucLed)
{
    uint8_t temp = 0;
    static uint8_t temp_old = 0xff;

    if (ucLed == NULL)
    {
        return;
    }

    for (int i = 0; i < 6; i++)
    {
        if (ucLed[i])
        {
            temp |= (1 << i);
        }
    }

    if (temp_old != temp)
    {
		
        LED1_SET(temp & 0x01);
        LED2_SET(temp & 0x02);
        LED3_SET(temp & 0x04);
        LED4_SET(temp & 0x08);
        LED5_SET(temp & 0x10);
        LED6_SET(temp & 0x20);

        temp_old = temp;
    }
}


void led_task(void)
{
	
    static uint8_t initialized = 0U;
    static uint32_t last_toggle_ms = 0U;
    uint32_t now_ms = get_system_ms();

	
    if (initialized == 0U)
    {
        LED1_OFF;
        LED2_OFF;
        LED3_OFF;
        LED4_OFF;
        LED5_OFF;
        LED6_OFF;
        last_toggle_ms = now_ms;
        initialized = 1U;
    }

    if ((now_ms - last_toggle_ms) >= LED_STATUS_TOGGLE_MS)
    {
        LED1_TOGGLE;
        last_toggle_ms = now_ms;
    }

    if (cimc_data_auto_report_is_running())
    {
        LED2_ON;
    }
    else
    {
        LED2_OFF;
    }
}
