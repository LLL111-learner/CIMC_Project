#ifndef RTC_DRIVER_H
#define RTC_DRIVER_H

#include "gd32f4xx.h"
#include "gd32f4xx_rtc.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RTC_CLOCK_SOURCE_LXTAL
#define BKP_VALUE    0x32F0

extern rtc_parameter_struct rtc_initpara;
extern __IO uint32_t prescaler_a;
extern __IO uint32_t prescaler_s;
extern uint32_t RTCSRC_FLAG;

int bsp_rtc_init(void);
void bsp_rtc_wakeup_config(uint16_t seconds);
void bsp_rtc_wakeup_deconfig(void);

#ifdef __cplusplus
}
#endif

#endif
