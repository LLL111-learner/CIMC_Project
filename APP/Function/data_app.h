#ifndef DATA_APP_H
#define DATA_APP_H

#include "bsp_system.h"

void cimc_data_init(void);
float cimc_data_get_ch0_value(void);
float cimc_data_get_ch1_value(void);
float cimc_data_get_ch2_value(void);
float cimc_data_get_ch0_ratio(void);
float cimc_data_get_ch1_ratio(void);
float cimc_data_get_ch0_threshold(void);
float cimc_data_get_ch1_threshold(void);
bool cimc_data_set_ch0_ratio(float ratio);
bool cimc_data_set_ch1_ratio(float ratio);
bool cimc_data_set_ch0_threshold(float threshold);
bool cimc_data_set_ch1_threshold(float threshold);
bool cimc_data_set_dac_raw(uint16_t raw);
void cimc_data_update_ch2_value(void);
void cimc_data_float_to_be(float value, uint8_t out[4]);
float cimc_data_float_from_be(const uint8_t in[4]);
bool cimc_data_set_report_interval_code(uint8_t code);
void cimc_data_auto_report_start(void);
void cimc_data_auto_report_stop(void);
bool cimc_data_auto_report_is_running(void);
bool cimc_data_auto_report_is_due(void);
void cimc_data_build_auto_report_payload(uint8_t out[12]);

#endif 
