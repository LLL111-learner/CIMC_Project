#ifndef PARAM_APP_H
#define PARAM_APP_H

#include "bsp_system.h"

typedef struct {
    uint16_t device_id;
    uint8_t baud_code;
    uint8_t reserved;
    float ch0_ratio;
    float ch1_ratio;
    float ch0_threshold;
    float ch1_threshold;
} cimc_param_values_t;

void cimc_param_init(void);
const cimc_param_values_t *cimc_param_get(void);
uint16_t cimc_param_get_dac_raw(void);
bool cimc_param_update_device_id(uint16_t device_id);
bool cimc_param_update_baud_code(uint8_t baud_code);
bool cimc_param_update_data(float ch0_ratio,
                            float ch1_ratio,
                            float ch0_threshold,
                            float ch1_threshold);
bool cimc_param_update_dac_raw(uint16_t dac_raw);

#endif 
