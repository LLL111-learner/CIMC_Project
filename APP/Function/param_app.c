#include "param_app.h"

#define CIMC_PARAM_OFFSET           0x800UL
#define CIMC_PARAM_ADDR             (BL_PARAM_START_ADDR + CIMC_PARAM_OFFSET)
#define CIMC_DAC_PARAM_OFFSET       0x900UL
#define CIMC_DAC_PARAM_ADDR         (BL_PARAM_START_ADDR + CIMC_DAC_PARAM_OFFSET)
#define CIMC_PARAM_MAGIC            0x43494D43UL
#define CIMC_PARAM_VERSION          0x00010001UL
#define CIMC_PARAM_TAIL_MAGIC       0x5041524DUL
#define CIMC_DAC_PARAM_MAGIC        0x44414331UL
#define CIMC_DAC_PARAM_VERSION      0x00010001UL
#define CIMC_DAC_PARAM_TAIL_MAGIC   0x44414332UL

typedef struct {
    uint32_t magic;
    uint32_t version;
    cimc_param_values_t values;
    uint32_t crc32;
    uint32_t tail_magic;
} cimc_param_record_t;

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint16_t dac_raw;
    uint16_t reserved;
    uint32_t crc32;
    uint32_t tail_magic;
} cimc_dac_param_record_t;

static cimc_param_values_t s_param_values;
static uint16_t s_dac_raw;
static uint8_t s_param_page_cache[BL_PARAM_SIZE];

static uint32_t crc32_update(uint32_t crc, const uint8_t *data, uint32_t len)
{
    uint32_t i;
    uint32_t j;

    for(i = 0U; i < len; i++) {
        crc ^= data[i];
        for(j = 0U; j < 8U; j++) {
            if((crc & 1U) != 0U) {
                crc = (crc >> 1U) ^ 0xEDB88320UL;
            } else {
                crc >>= 1U;
            }
        }
    }

    return crc;
}

static uint32_t crc32_calc(const uint8_t *data, uint32_t len)
{
    return crc32_update(0xFFFFFFFFUL, data, len) ^ 0xFFFFFFFFUL;
}

static uint32_t record_calc_crc(const cimc_param_record_t *record)
{
    return crc32_calc((const uint8_t *)record, (uint32_t)offsetof(cimc_param_record_t, crc32));
}

static uint32_t dac_record_calc_crc(const cimc_dac_param_record_t *record)
{
    return crc32_calc((const uint8_t *)record, (uint32_t)offsetof(cimc_dac_param_record_t, crc32));
}

static void values_set_default(cimc_param_values_t *values)
{
    values->device_id = CIMC_DEVICE_ID_DEFAULT;
    values->baud_code = CIMC_BAUD_CODE_19200;
    values->reserved = 0U;
    values->ch0_ratio = 1.0f;
    values->ch1_ratio = 1.0f;
    values->ch0_threshold = 3000.0f;
    values->ch1_threshold = 3000.0f;
}

static void record_set_values(cimc_param_record_t *record, const cimc_param_values_t *values)
{
    memset(record, 0xFF, sizeof(*record));
    record->magic = CIMC_PARAM_MAGIC;
    record->version = CIMC_PARAM_VERSION;
    record->values = *values;
    record->tail_magic = CIMC_PARAM_TAIL_MAGIC;
    record->crc32 = record_calc_crc(record);
}

static void dac_record_set_value(cimc_dac_param_record_t *record, uint16_t dac_raw)
{
    memset(record, 0xFF, sizeof(*record));
    record->magic = CIMC_DAC_PARAM_MAGIC;
    record->version = CIMC_DAC_PARAM_VERSION;
    record->dac_raw = dac_raw;
    record->reserved = 0U;
    record->tail_magic = CIMC_DAC_PARAM_TAIL_MAGIC;
    record->crc32 = dac_record_calc_crc(record);
}

static bool record_is_valid(const cimc_param_record_t *record)
{
    if((record->magic != CIMC_PARAM_MAGIC) ||
       (record->version != CIMC_PARAM_VERSION) ||
       (record->tail_magic != CIMC_PARAM_TAIL_MAGIC)) {
        return false;
    }

    return (record->crc32 == record_calc_crc(record));
}

static bool dac_record_is_valid(const cimc_dac_param_record_t *record)
{
    if((record->magic != CIMC_DAC_PARAM_MAGIC) ||
       (record->version != CIMC_DAC_PARAM_VERSION) ||
       (record->tail_magic != CIMC_DAC_PARAM_TAIL_MAGIC) ||
       (record->dac_raw > 4095U)) {
        return false;
    }

    return (record->crc32 == dac_record_calc_crc(record));
}

__attribute__((section(".ramfunc"), noinline))
static bool flash_wait_ready(void)
{
    uint32_t timeout = 0x3FFFFFUL;

    while((RESET != fmc_flag_get(FMC_FLAG_BUSY)) && (timeout > 0U)) {
        timeout--;
    }

    return (timeout > 0U);
}

__attribute__((section(".ramfunc"), noinline))
static void flash_clear_flags(void)
{
    fmc_flag_clear(FMC_FLAG_END);
    fmc_flag_clear(FMC_FLAG_WPERR);
    fmc_flag_clear(FMC_FLAG_PGSERR);
    fmc_flag_clear(FMC_FLAG_PGMERR);
}

__attribute__((section(".ramfunc"), noinline))
static bool flash_rewrite_param_page(const uint8_t *page_data)
{
    uint32_t addr;
    uint32_t word_value;
    fmc_state_enum fmc_state;

    if(page_data == NULL) {
        return false;
    }

    fmc_unlock();
    flash_clear_flags();

    fmc_state = fmc_page_erase(BL_PARAM_START_ADDR);
    if((fmc_state != FMC_READY) || !flash_wait_ready()) {
        fmc_lock();
        return false;
    }
    flash_clear_flags();

    addr = BL_PARAM_START_ADDR;
    while(addr < (BL_PARAM_START_ADDR + BL_PARAM_SIZE)) {
        memcpy(&word_value, page_data, sizeof(word_value));
        fmc_state = fmc_word_program(addr, word_value);
        if((fmc_state != FMC_READY) || !flash_wait_ready() || (*(volatile uint32_t *)addr != word_value)) {
            fmc_lock();
            return false;
        }
        addr += 4UL;
        page_data += 4U;
    }

    flash_clear_flags();
    fmc_lock();
    return true;
}

static bool param_commit(const cimc_param_values_t *values)
{
    cimc_param_record_t record;
    const cimc_param_record_t *flash_record = (const cimc_param_record_t *)CIMC_PARAM_ADDR;

    if(values == NULL) {
        return false;
    }

    memcpy(s_param_page_cache, (const void *)BL_PARAM_START_ADDR, BL_PARAM_SIZE);
    record_set_values(&record, values);
    memcpy(&s_param_page_cache[CIMC_PARAM_OFFSET], &record, sizeof(record));

    if(!flash_rewrite_param_page(s_param_page_cache)) {
        return false;
    }

    return record_is_valid(flash_record) && (memcmp(flash_record, &record, sizeof(record)) == 0);
}

static bool dac_param_commit(uint16_t dac_raw)
{
    cimc_dac_param_record_t record;
    const cimc_dac_param_record_t *flash_record = (const cimc_dac_param_record_t *)CIMC_DAC_PARAM_ADDR;

    if(dac_raw > 4095U) {
        return false;
    }

    memcpy(s_param_page_cache, (const void *)BL_PARAM_START_ADDR, BL_PARAM_SIZE);
    dac_record_set_value(&record, dac_raw);
    memcpy(&s_param_page_cache[CIMC_DAC_PARAM_OFFSET], &record, sizeof(record));

    if(!flash_rewrite_param_page(s_param_page_cache)) {
        return false;
    }

    return dac_record_is_valid(flash_record) && (memcmp(flash_record, &record, sizeof(record)) == 0);
}

void cimc_param_init(void)
{
    const cimc_param_record_t *record = (const cimc_param_record_t *)CIMC_PARAM_ADDR;
    const cimc_dac_param_record_t *dac_record = (const cimc_dac_param_record_t *)CIMC_DAC_PARAM_ADDR;

    if(record_is_valid(record)) {
        s_param_values = record->values;
    } else {
        values_set_default(&s_param_values);
    }

    if(dac_record_is_valid(dac_record)) {
        s_dac_raw = dac_record->dac_raw;
    } else {
        s_dac_raw = 0U;
    }
}

const cimc_param_values_t *cimc_param_get(void)
{
    return &s_param_values;
}

uint16_t cimc_param_get_dac_raw(void)
{
    return s_dac_raw;
}

bool cimc_param_update_device_id(uint16_t device_id)
{
    cimc_param_values_t values = s_param_values;

    if((device_id == CIMC_DEVICE_ID_BROADCAST) || (device_id == 0U)) {
        return false;
    }

    values.device_id = device_id;
    values.reserved = 0U;

    if(!param_commit(&values)) {
        return false;
    }

    s_param_values = values;
    return true;
}

bool cimc_param_update_baud_code(uint8_t baud_code)
{
    cimc_param_values_t values = s_param_values;

    if((baud_code != CIMC_BAUD_CODE_4800) &&
       (baud_code != CIMC_BAUD_CODE_9600) &&
       (baud_code != CIMC_BAUD_CODE_19200) &&
       (baud_code != CIMC_BAUD_CODE_115200)) {
        return false;
    }

    values.baud_code = baud_code;
    values.reserved = 0U;

    if(!param_commit(&values)) {
        return false;
    }

    s_param_values = values;
    return true;
}

bool cimc_param_update_data(float ch0_ratio,
                            float ch1_ratio,
                            float ch0_threshold,
                            float ch1_threshold)
{
    cimc_param_values_t values = s_param_values;

    values.ch0_ratio = ch0_ratio;
    values.ch1_ratio = ch1_ratio;
    values.ch0_threshold = ch0_threshold;
    values.ch1_threshold = ch1_threshold;
    values.reserved = 0U;

    if(!param_commit(&values)) {
        return false;
    }

    s_param_values = values;
    return true;
}

bool cimc_param_update_dac_raw(uint16_t dac_raw)
{
    if(dac_raw > 4095U) {
        return false;
    }

    if(!dac_param_commit(dac_raw)) {
        return false;
    }

    s_dac_raw = dac_raw;
    return true;
}
