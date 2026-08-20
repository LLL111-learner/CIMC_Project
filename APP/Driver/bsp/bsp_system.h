#ifndef BSP_SYSTEM_H
#define BSP_SYSTEM_H

#include "gd32f4xx.h"
#include "gd32f4xx_dma.h"
#include "systick.h"

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>

#include "led_driver.h"
#include "key_driver.h"
#include "uart_driver.h"
#include "oled_driver.h"
#include "gd30ad3344_driver.h"
#include "adc_driver.h"
#include "dac_driver.h"
#include "rtc_driver.h"
#include "power_driver.h"

#include "oled.h"
#include "gd30ad3344.h"

#include "bl_param.h"
#include "perf_counter.h"

#include "led_app.h"
#include "adc_app.h"
#include "oled_app.h"
#include "usart_app.h"
#include "rtc_app.h"
#include "scheduler.h"
#include "alarm_app.h"
#include "data_app.h"
#include "system_app.h"
#include "param_app.h"

#include "system_protocol.h"

#endif /* BSP_SYSTEM_H */
