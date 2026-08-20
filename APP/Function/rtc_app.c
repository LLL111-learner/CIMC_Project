#include "rtc_app.h"

void rtc_task(void)
{
    rtc_current_time_get(&rtc_initpara);
}
