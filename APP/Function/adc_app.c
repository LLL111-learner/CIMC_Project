#include "adc_app.h"


void adc_task(void)
{
    (void)adc_value;
    cimc_data_update_ch2_value();
    cimc_alarm_task();
}

