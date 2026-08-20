#ifndef ALARM_APP_H
#define ALARM_APP_H

#include "bsp_system.h"

#define CIMC_ALARM_QUERY_BUFFER_SIZE 512U

void cimc_alarm_init(void);
bool cimc_alarm_set_active_mode(uint8_t mode);
void cimc_alarm_task(void);
bool cimc_alarm_clear_records(void);
void cimc_alarm_build_query_response(char *out, uint16_t out_size);

#endif 
