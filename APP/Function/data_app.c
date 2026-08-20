#include "data_app.h"

static float s_ch0_ratio = 1.0f;
static float s_ch1_ratio = 1.0f;
static float s_ch0_threshold = 3000.0f;
static float s_ch1_threshold = 3000.0f;
static float s_ch2_resistance;
static float s_ch2_temperature;
static float s_ch2_adc_voltage;
static uint16_t s_dac_raw;
static uint32_t s_report_interval_ms = 1000U;
static uint32_t s_last_report_ms;
static bool s_auto_report_running;

typedef struct {
    float measured;
    float actual;
} resistance_cal_point_t;

static const resistance_cal_point_t s_ch2_r_cal_table[] = {
    {80.500f, 81.0f},
    {82.500f, 83.8f},
    {99.900f, 100.0f},
    {106.900f, 107.6f},
    {112.800f, 113.3f},
    {115.365f, 115.3f},
    {123.900f, 124.0f},
    {130.600f, 130.8f},
    {139.800f, 137.0f},
    {150.850f, 150.0f},
    {153.810f, 154.0f},
};

#define CIMC_CH2_R_CAL_POINT_NUM (sizeof(s_ch2_r_cal_table) / sizeof(s_ch2_r_cal_table[0]))
#define CIMC_CH2_TLV431_VOL      1.8230f
#define CIMC_CH2_G_DIFF          1.60f
#define CIMC_CH2_G_INA           10.022f
#define CIMC_CH2_I_RTD_MA        0.9999f

static void put_u32_be_payload(uint8_t *buf, uint32_t value)
{
    buf[0] = (uint8_t)(value >> 24U);
    buf[1] = (uint8_t)(value >> 16U);
    buf[2] = (uint8_t)(value >> 8U);
    buf[3] = (uint8_t)value;
}

static float ch2_resistance_to_temperature(float resistance)
{
    const float r0 = 100.0f;
    const float a = 3.9083e-3f;
    const float b = -5.775e-7f;
    float ratio;
    float discriminant;

    ratio = resistance / r0;
    discriminant = (a * a) - (4.0f * b * (1.0f - ratio));
    if(discriminant < 0.0f) {
        return -999.0f;
    }

    return (-a + sqrtf(discriminant)) / (2.0f * b);
}

static float ch2_calibrate_resistance(float resistance)
{
    uint32_t i;
    const resistance_cal_point_t *p1 = &s_ch2_r_cal_table[CIMC_CH2_R_CAL_POINT_NUM - 2U];
    const resistance_cal_point_t *p2 = &s_ch2_r_cal_table[CIMC_CH2_R_CAL_POINT_NUM - 1U];

    if(CIMC_CH2_R_CAL_POINT_NUM < 2U) {
        return resistance;
    }

    if(resistance <= s_ch2_r_cal_table[0].measured) {
        p1 = &s_ch2_r_cal_table[0];
        p2 = &s_ch2_r_cal_table[1];
    } else if(resistance >= s_ch2_r_cal_table[CIMC_CH2_R_CAL_POINT_NUM - 1U].measured) {
        p1 = &s_ch2_r_cal_table[CIMC_CH2_R_CAL_POINT_NUM - 2U];
        p2 = &s_ch2_r_cal_table[CIMC_CH2_R_CAL_POINT_NUM - 1U];
    } else {
        for(i = 0U; i < (CIMC_CH2_R_CAL_POINT_NUM - 1U); i++) {
            if((resistance >= s_ch2_r_cal_table[i].measured) &&
               (resistance <= s_ch2_r_cal_table[i + 1U].measured)) {
                p1 = &s_ch2_r_cal_table[i];
                p2 = &s_ch2_r_cal_table[i + 1U];
                break;
            }
        }
    }

    if(p2->measured == p1->measured) {
        return p1->actual;
    }

    return p1->actual +
           ((resistance - p1->measured) * (p2->actual - p1->actual) /
            (p2->measured - p1->measured));
}

void cimc_data_init(void)
{
    const cimc_param_values_t *params = cimc_param_get();

    s_ch0_ratio = params->ch0_ratio;
    s_ch1_ratio = params->ch1_ratio;
    s_ch0_threshold = params->ch0_threshold;
    s_ch1_threshold = params->ch1_threshold;
    s_ch2_resistance = 0.0f;
    s_ch2_temperature = 0.0f;
    s_ch2_adc_voltage = 0.0f;
    s_dac_raw = cimc_param_get_dac_raw();
    if(s_dac_raw > 4095U) {
        s_dac_raw = 0U;
    }
    convertarr[0] = s_dac_raw;
    dac_data_set(DAC0, DAC_OUT0, DAC_ALIGN_12B_R, s_dac_raw);
    s_report_interval_ms = 1000U;
    s_last_report_ms = get_system_ms();
    s_auto_report_running = false;
}

float cimc_data_get_ch0_value(void)
{
    return (float)adc_value[0] * s_ch0_ratio;
}

float cimc_data_get_ch1_value(void)
{
    return (float)adc_value[1] * s_ch1_ratio;
}

float cimc_data_get_ch2_value(void)
{
    return s_ch2_temperature;
}

void cimc_data_update_ch2_value(void)
{
    s_ch2_adc_voltage = GD30AD3344_AD_Read(GD30AD3344_Channel_4, GD30AD3344_PGA_2V048);
    s_ch2_resistance = (CIMC_CH2_TLV431_VOL - (s_ch2_adc_voltage / CIMC_CH2_G_DIFF)) /
                       CIMC_CH2_G_INA / CIMC_CH2_I_RTD_MA * 1000.0f;
    s_ch2_resistance = ch2_calibrate_resistance(s_ch2_resistance);
    s_ch2_temperature = ch2_resistance_to_temperature(s_ch2_resistance);
}

float cimc_data_get_ch0_ratio(void)
{
    return s_ch0_ratio;
}

float cimc_data_get_ch1_ratio(void)
{
    return s_ch1_ratio;
}

float cimc_data_get_ch0_threshold(void)
{
    return s_ch0_threshold;
}

float cimc_data_get_ch1_threshold(void)
{
    return s_ch1_threshold;
}

bool cimc_data_set_ch0_ratio(float ratio)
{
    if(!cimc_param_update_data(ratio, s_ch1_ratio, s_ch0_threshold, s_ch1_threshold)) {
        return false;
    }
    s_ch0_ratio = ratio;
    return true;
}

bool cimc_data_set_ch1_ratio(float ratio)
{
    if(!cimc_param_update_data(s_ch0_ratio, ratio, s_ch0_threshold, s_ch1_threshold)) {
        return false;
    }
    s_ch1_ratio = ratio;
    return true;
}

bool cimc_data_set_ch0_threshold(float threshold)
{
    if(!cimc_param_update_data(s_ch0_ratio, s_ch1_ratio, threshold, s_ch1_threshold)) {
        return false;
    }
    s_ch0_threshold = threshold;
    return true;
}

bool cimc_data_set_ch1_threshold(float threshold)
{
    if(!cimc_param_update_data(s_ch0_ratio, s_ch1_ratio, s_ch0_threshold, threshold)) {
        return false;
    }
    s_ch1_threshold = threshold;
    return true;
}

bool cimc_data_set_dac_raw(uint16_t raw)
{
    if(raw > 4095U) {
        return false;
    }

    if(!cimc_param_update_dac_raw(raw)) {
        return false;
    }

    s_dac_raw = raw;
    convertarr[0] = s_dac_raw;
    dac_data_set(DAC0, DAC_OUT0, DAC_ALIGN_12B_R, s_dac_raw);
    return true;
}

void cimc_data_float_to_be(float value, uint8_t out[4])
{
    uint32_t raw;

    memcpy(&raw, &value, sizeof(raw));
    out[0] = (uint8_t)(raw >> 24U);
    out[1] = (uint8_t)(raw >> 16U);
    out[2] = (uint8_t)(raw >> 8U);
    out[3] = (uint8_t)raw;
}

float cimc_data_float_from_be(const uint8_t in[4])
{
    uint32_t raw;
    float value;

    raw = ((uint32_t)in[0] << 24U) |
          ((uint32_t)in[1] << 16U) |
          ((uint32_t)in[2] << 8U) |
          (uint32_t)in[3];
    memcpy(&value, &raw, sizeof(value));
    return value;
}

bool cimc_data_set_report_interval_code(uint8_t code)
{
    switch(code) {
    case 0x01U:
        s_report_interval_ms = 1000U;
        return true;
    case 0x02U:
        s_report_interval_ms = 3000U;
        return true;
    case 0x03U:
        s_report_interval_ms = 5000U;
        return true;
    default:
        return false;
    }
}

void cimc_data_auto_report_start(void)
{
    s_auto_report_running = true;
    s_last_report_ms = get_system_ms();
}

void cimc_data_auto_report_stop(void)
{
    s_auto_report_running = false;
}

bool cimc_data_auto_report_is_running(void)
{
    return s_auto_report_running;
}

bool cimc_data_auto_report_is_due(void)
{
    uint32_t now_ms;

    if(!s_auto_report_running) {
        return false;
    }

    now_ms = get_system_ms();
    if((now_ms - s_last_report_ms) < s_report_interval_ms) {
        return false;
    }

    s_last_report_ms = now_ms;
    return true;
}

void cimc_data_build_auto_report_payload(uint8_t out[12])
{
    if(out == NULL) {
        return;
    }

    put_u32_be_payload(&out[0], cimc_system_get_time());
    cimc_data_float_to_be(cimc_data_get_ch0_value(), &out[4]);
    cimc_data_float_to_be(cimc_data_get_ch1_value(), &out[8]);
}
