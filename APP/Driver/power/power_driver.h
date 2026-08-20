#ifndef POWER_DRIVER_H
#define POWER_DRIVER_H

#include "gd32f4xx.h"
#include "gd32f4xx_pmu.h"
#include "bsp_system.h"

#ifdef __cplusplus
extern "C" {
#endif

void bsp_gpio_enter_deepsleep_state(void);
void bsp_deepsleep_reinit_after_wakeup(void);
void bsp_enter_deepsleep_for_seconds(uint16_t seconds);

#ifdef __cplusplus
}
#endif

#endif
