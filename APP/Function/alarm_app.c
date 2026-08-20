#include "alarm_app.h"

#define CIMC_ALARM_OFFSET           0xC00UL
#define CIMC_ALARM_ADDR             (BL_PARAM_START_ADDR + CIMC_ALARM_OFFSET)
#define CIMC_ALARM_MAGIC            0x414C4D43UL
#define CIMC_ALARM_VERSION          0x00010001UL
#define CIMC_ALARM_TAIL_MAGIC       0x414C4D54UL
#define CIMC_ALARM_MAX_RECORDS      10U
#define CIMC_ALARM_CHANNEL_CH0      0U
#define CIMC_ALARM_CHANNEL_CH1      1U
#define CIMC_ALARM_MODE_ACTIVE      0x01U
#define CIMC_ALARM_MODE_PASSIVE     0x02U

typedef struct {
    uint32_t timestamp;
    uint8_t channel;
    uint8_t reserved[3];
    float threshold;
    float actual;
} cimc_alarm_entry_t;

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t count;
    cimc_alarm_entry_t entries[CIMC_ALARM_MAX_RECORDS];
    uint32_t crc32;
    uint32_t tail_magic;
} cimc_alarm_record_t;

static cimc_alarm_record_t s_alarm_record;
static uint8_t s_alarm_page_cache[BL_PARAM_SIZE];
static uint8_t s_alarm_mode = CIMC_ALARM_MODE_PASSIVE;
static bool s_ch0_above;
static bool s_ch1_above;

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

static uint32_t alarm_calc_crc(const cimc_alarm_record_t *record)
{
    return crc32_calc((const uint8_t *)record, (uint32_t)offsetof(cimc_alarm_record_t, crc32));
}

static void alarm_record_set_empty(cimc_alarm_record_t *record)
{
    memset(record, 0, sizeof(*record));
    record->magic = CIMC_ALARM_MAGIC;
    record->version = CIMC_ALARM_VERSION;
    record->tail_magic = CIMC_ALARM_TAIL_MAGIC;
    record->crc32 = alarm_calc_crc(record);
}

static bool alarm_record_is_valid(const cimc_alarm_record_t *record)
{
    if((record->magic != CIMC_ALARM_MAGIC) ||
       (record->version != CIMC_ALARM_VERSION) ||
       (record->tail_magic != CIMC_ALARM_TAIL_MAGIC) ||
       (record->count > CIMC_ALARM_MAX_RECORDS)) {
        return false;
    }

    return record->crc32 == alarm_calc_crc(record);
}

__attribute__((section(".ramfunc"), noinline))
static bool alarm_flash_wait_ready(void)
{
    uint32_t timeout = 0x3FFFFFUL;

    while((RESET != fmc_flag_get(FMC_FLAG_BUSY)) && (timeout > 0U)) {
        timeout--;
    }

    return timeout > 0U;
}

__attribute__((section(".ramfunc"), noinline))
static void alarm_flash_clear_flags(void)
{
    fmc_flag_clear(FMC_FLAG_END);
    fmc_flag_clear(FMC_FLAG_WPERR);
    fmc_flag_clear(FMC_FLAG_PGSERR);
    fmc_flag_clear(FMC_FLAG_PGMERR);
}

__attribute__((section(".ramfunc"), noinline))
static bool alarm_flash_rewrite_param_page(const uint8_t *page_data)
{
    uint32_t addr;
    uint32_t word_value;
    fmc_state_enum fmc_state;

    if(page_data == NULL) {
        return false;
    }

    fmc_unlock();
    alarm_flash_clear_flags();

    fmc_state = fmc_page_erase(BL_PARAM_START_ADDR);
    if((fmc_state != FMC_READY) || !alarm_flash_wait_ready()) {
        fmc_lock();
        return false;
    }
    alarm_flash_clear_flags();

    addr = BL_PARAM_START_ADDR;
    while(addr < (BL_PARAM_START_ADDR + BL_PARAM_SIZE)) {
        memcpy(&word_value, page_data, sizeof(word_value));
        fmc_state = fmc_word_program(addr, word_value);
        if((fmc_state != FMC_READY) || !alarm_flash_wait_ready() || (*(volatile uint32_t *)addr != word_value)) {
            fmc_lock();
            return false;
        }
        addr += 4UL;
        page_data += 4U;
    }

    alarm_flash_clear_flags();
    fmc_lock();
    return true;
}

static bool alarm_commit(void)
{
    const cimc_alarm_record_t *flash_record = (const cimc_alarm_record_t *)CIMC_ALARM_ADDR;

    s_alarm_record.crc32 = alarm_calc_crc(&s_alarm_record);
    memcpy(s_alarm_page_cache, (const void *)BL_PARAM_START_ADDR, BL_PARAM_SIZE);
    memcpy(&s_alarm_page_cache[CIMC_ALARM_OFFSET], &s_alarm_record, sizeof(s_alarm_record));

    if(!alarm_flash_rewrite_param_page(s_alarm_page_cache)) {
        return false;
    }

    return alarm_record_is_valid(flash_record) &&
           (memcmp(flash_record, &s_alarm_record, sizeof(s_alarm_record)) == 0);
}

static bool is_leap_year(uint32_t year)
{
    return (((year % 4U) == 0U) && ((year % 100U) != 0U)) || ((year % 400U) == 0U);
}

static void format_time(uint32_t timestamp, char *out, uint16_t out_size)
{
    static const uint8_t days_in_month_common[12] = {
        31U, 28U, 31U, 30U, 31U, 30U, 31U, 31U, 30U, 31U, 30U, 31U
    };
    uint32_t days = timestamp / 86400U;
    uint32_t seconds = timestamp % 86400U;
    uint32_t year = 1970U;
    uint32_t month = 1U;
    uint32_t day;
    uint32_t days_this_year;
    uint32_t days_this_month;
    uint32_t hour;
    uint32_t minute;
    uint32_t second;

    while(1) {
        days_this_year = is_leap_year(year) ? 366U : 365U;
        if(days < days_this_year) {
            break;
        }
        days -= days_this_year;
        year++;
    }

    while(month <= 12U) {
        days_this_month = days_in_month_common[month - 1U];
        if((month == 2U) && is_leap_year(year)) {
            days_this_month++;
        }
        if(days < days_this_month) {
            break;
        }
        days -= days_this_month;
        month++;
    }

    day = days + 1U;
    hour = seconds / 3600U;
    minute = (seconds % 3600U) / 60U;
    second = seconds % 60U;

    (void)snprintf(out, out_size, "%04lu-%02lu-%02lu %02lu:%02lu:%02lu",
                   (unsigned long)year,
                   (unsigned long)month,
                   (unsigned long)day,
                   (unsigned long)hour,
                   (unsigned long)minute,
                   (unsigned long)second);
}

static void format_float2(float value, char *out, uint16_t out_size)
{
    int32_t scaled;
    int32_t integer_part;
    int32_t decimal_part;
    const char *sign = "";

    if(value < 0.0f) {
        sign = "-";
        value = -value;
    }

    scaled = (int32_t)((value * 100.0f) + 0.5f);
    integer_part = scaled / 100;
    decimal_part = scaled % 100;

    (void)snprintf(out, out_size, "%s%ld.%02ld",
                   sign,
                   (long)integer_part,
                   (long)decimal_part);
}

static void format_entry(const cimc_alarm_entry_t *entry, char *out, uint16_t out_size)
{
    char time_text[20];
    char threshold_text[16];
    char actual_text[16];

    format_time(entry->timestamp, time_text, sizeof(time_text));
    format_float2(entry->threshold, threshold_text, sizeof(threshold_text));
    format_float2(entry->actual, actual_text, sizeof(actual_text));
    (void)snprintf(out, out_size, "%s | CH%u | %s | %s",
                   time_text,
                   (unsigned int)entry->channel,
                   threshold_text,
                   actual_text);
}

static void alarm_report_string(const cimc_alarm_entry_t *entry)
{
    char line[96];
    size_t used;

    format_entry(entry, line, sizeof(line));
    used = strlen(line);
    (void)snprintf(&line[used], sizeof(line) - used, "\r\n");
    rs485_send_bytes((const uint8_t *)line, (uint16_t)strlen(line));
}

static void alarm_store(uint8_t channel, float threshold, float actual)
{
    cimc_alarm_entry_t entry;
    uint32_t move_count;

    entry.timestamp = cimc_system_get_time();
    entry.channel = channel;
    entry.reserved[0] = 0U;
    entry.reserved[1] = 0U;
    entry.reserved[2] = 0U;
    entry.threshold = threshold;
    entry.actual = actual;

    move_count = s_alarm_record.count;
    if(move_count >= CIMC_ALARM_MAX_RECORDS) {
        move_count = CIMC_ALARM_MAX_RECORDS - 1U;
    }
    if(move_count > 0U) {
        memmove(&s_alarm_record.entries[1],
                &s_alarm_record.entries[0],
                (size_t)(move_count * sizeof(cimc_alarm_entry_t)));
    }
    s_alarm_record.entries[0] = entry;
    if(s_alarm_record.count < CIMC_ALARM_MAX_RECORDS) {
        s_alarm_record.count++;
    }

    (void)alarm_commit();

    if(s_alarm_mode == CIMC_ALARM_MODE_ACTIVE) {
        alarm_report_string(&entry);
    }
}

static void alarm_check_channel(uint8_t channel, float threshold, float actual, bool *above_latch)
{
    bool above_now;

    if(above_latch == NULL) {
        return;
    }

    above_now = actual > threshold;
    if(above_now && !(*above_latch)) {
        alarm_store(channel, threshold, actual);
    }

    *above_latch = above_now;
}

void cimc_alarm_init(void)
{
    const cimc_alarm_record_t *record = (const cimc_alarm_record_t *)CIMC_ALARM_ADDR;

    if(alarm_record_is_valid(record)) {
        s_alarm_record = *record;
    } else {
        alarm_record_set_empty(&s_alarm_record);
    }

    s_alarm_mode = CIMC_ALARM_MODE_PASSIVE;
    s_ch0_above = false;
    s_ch1_above = false;
}

bool cimc_alarm_set_active_mode(uint8_t mode)
{
    if((mode != CIMC_ALARM_MODE_ACTIVE) && (mode != CIMC_ALARM_MODE_PASSIVE)) {
        return false;
    }

    s_alarm_mode = mode;
    s_ch0_above = false;
    s_ch1_above = false;
    return true;
}

void cimc_alarm_task(void)
{
    alarm_check_channel(CIMC_ALARM_CHANNEL_CH0,
                        cimc_data_get_ch0_threshold(),
                        cimc_data_get_ch0_value(),
                        &s_ch0_above);
    alarm_check_channel(CIMC_ALARM_CHANNEL_CH1,
                        cimc_data_get_ch1_threshold(),
                        cimc_data_get_ch1_value(),
                        &s_ch1_above);
}

bool cimc_alarm_clear_records(void)
{
    alarm_record_set_empty(&s_alarm_record);
    s_ch0_above = cimc_data_get_ch0_value() > cimc_data_get_ch0_threshold();
    s_ch1_above = cimc_data_get_ch1_value() > cimc_data_get_ch1_threshold();
    return alarm_commit();
}

void cimc_alarm_build_query_response(char *out, uint16_t out_size)
{
    uint32_t i;
    size_t used = 0U;
    int written;
    char line[96];

    if((out == NULL) || (out_size == 0U)) {
        return;
    }

    out[0] = '\0';
    if(s_alarm_record.count == 0U) {
        (void)snprintf(out, out_size, "empty\r\n");
        return;
    }

    for(i = 0U; (i < s_alarm_record.count) && (i < CIMC_ALARM_MAX_RECORDS); i++) {
        format_entry(&s_alarm_record.entries[i], line, sizeof(line));
        written = snprintf(&out[used], (size_t)out_size - used, "%s\r\n", line);
        if((written < 0) || ((size_t)written >= ((size_t)out_size - used))) {
            out[out_size - 1U] = '\0';
            return;
        }
        used += (size_t)written;
    }
}
