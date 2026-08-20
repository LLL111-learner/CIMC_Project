#ifndef SYSTEM_APP_H
#define SYSTEM_APP_H

#include "bsp_system.h"

#define CIMC_FW_VER_MAJOR       2U
#define CIMC_FW_VER_MINOR       0U
#define CIMC_FW_VER_PATCH       1U
#define CIMC_FW_VER_BUILD       0U

#define CIMC_BAUD_CODE_4800     0x11U
#define CIMC_BAUD_CODE_9600     0x12U
#define CIMC_BAUD_CODE_19200    0x13U
#define CIMC_BAUD_CODE_115200   0x14U

void cimc_system_init(void);
void cimc_system_set_time(uint32_t utc_seconds);
uint32_t cimc_system_get_time(void);
uint8_t cimc_system_get_baud_code(void);
uint32_t cimc_system_baud_code_to_value(uint8_t baud_code);
void cimc_system_apply_baud_rate(void);
bool cimc_system_request_bootloader_upgrade(void);
void cimc_system_reboot(void);
void cimc_system_enter_sleep_and_report_wakeup(void);

#endif
