#include "system_app.h"


#define CIMC_UNIX_EPOCH_YEAR        1970U
#define CIMC_RTC_YEAR_BASE          2000U
#define CIMC_SECONDS_PER_DAY        86400UL

static uint32_t s_time_base_utc;
static uint32_t s_time_base_ms;
static uint8_t s_bl_param_page_cache[BL_PARAM_SIZE];

static uint8_t cimc_bcd_to_bin(uint8_t bcd)
{
    return (uint8_t)((((uint8_t)(bcd >> 4U)) * 10U) + (bcd & 0x0FU));
}

static uint8_t cimc_bin_to_bcd(uint8_t value)
{
    return (uint8_t)((uint8_t)((value / 10U) << 4U) | (uint8_t)(value % 10U));
}

static bool cimc_is_leap_year(uint32_t year)
{
    return (((year % 4U) == 0U) && ((year % 100U) != 0U)) || ((year % 400U) == 0U);
}

static uint8_t cimc_days_in_month(uint32_t year, uint8_t month)
{
    static const uint8_t month_days[12] = {
        31U, 28U, 31U, 30U, 31U, 30U,
        31U, 31U, 30U, 31U, 30U, 31U
    };

    if((month == 2U) && cimc_is_leap_year(year)) {
        return 29U;
    }

    return month_days[month - 1U];
}

static uint32_t cimc_datetime_to_utc(uint32_t year, uint8_t month, uint8_t day,
                                     uint8_t hour, uint8_t minute, uint8_t second)
{
    uint32_t days = 0U;
    uint32_t y;
    uint8_t m;

    if((year < CIMC_UNIX_EPOCH_YEAR) || (month == 0U) || (month > 12U) ||
       (day == 0U) || (day > cimc_days_in_month(year, month)) ||
       (hour > 23U) || (minute > 59U) || (second > 59U)) {
        return 0U;
    }

    for(y = CIMC_UNIX_EPOCH_YEAR; y < year; y++) {
        days += cimc_is_leap_year(y) ? 366U : 365U;
    }

    for(m = 1U; m < month; m++) {
        days += cimc_days_in_month(year, m);
    }

    days += (uint32_t)(day - 1U);

    return (days * CIMC_SECONDS_PER_DAY) +
           ((uint32_t)hour * 3600U) +
           ((uint32_t)minute * 60U) +
           (uint32_t)second;
}

static void cimc_utc_to_rtc(uint32_t utc_seconds, rtc_parameter_struct *rtc_time)
{
    uint32_t days = utc_seconds / CIMC_SECONDS_PER_DAY;
    uint32_t seconds = utc_seconds % CIMC_SECONDS_PER_DAY;
    uint32_t year = CIMC_UNIX_EPOCH_YEAR;
    uint32_t year_days;
    uint8_t month = 1U;
    uint8_t month_days;

    while(true) {
        year_days = cimc_is_leap_year(year) ? 366U : 365U;
        if(days < year_days) {
            break;
        }
        days -= year_days;
        year++;
    }

    while(true) {
        month_days = cimc_days_in_month(year, month);
        if(days < month_days) {
            break;
        }
        days -= month_days;
        month++;
    }

    memset(rtc_time, 0, sizeof(*rtc_time));
    rtc_time->factor_asyn = prescaler_a;
    rtc_time->factor_syn = prescaler_s;
    rtc_time->year = cimc_bin_to_bcd((uint8_t)(year - CIMC_RTC_YEAR_BASE));
    rtc_time->month = cimc_bin_to_bcd(month);
    rtc_time->date = cimc_bin_to_bcd((uint8_t)(days + 1U));
    rtc_time->day_of_week = (uint8_t)(((utc_seconds / CIMC_SECONDS_PER_DAY + 3U) % 7U) + 1U);
    rtc_time->display_format = RTC_24HOUR;
    rtc_time->am_pm = RTC_AM;
    rtc_time->hour = cimc_bin_to_bcd((uint8_t)(seconds / 3600U));
    seconds %= 3600U;
    rtc_time->minute = cimc_bin_to_bcd((uint8_t)(seconds / 60U));
    rtc_time->second = cimc_bin_to_bcd((uint8_t)(seconds % 60U));
}

static uint32_t cimc_rtc_to_utc(void)
{
    rtc_parameter_struct rtc_time;
    uint32_t year;

    rtc_current_time_get(&rtc_time);

    year = CIMC_RTC_YEAR_BASE + cimc_bcd_to_bin(rtc_time.year);
    return cimc_datetime_to_utc(year,
                                cimc_bcd_to_bin(rtc_time.month),
                                cimc_bcd_to_bin(rtc_time.date),
                                cimc_bcd_to_bin(rtc_time.hour),
                                cimc_bcd_to_bin(rtc_time.minute),
                                cimc_bcd_to_bin(rtc_time.second));
}

static uint32_t cimc_bl_crc32_update(uint32_t crc, const uint8_t *data, uint32_t len)
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

static uint32_t cimc_bl_crc32_calc(const uint8_t *data, uint32_t len)
{
    return cimc_bl_crc32_update(0xFFFFFFFFUL, data, len) ^ 0xFFFFFFFFUL;
}

static uint32_t cimc_bl_param_calc_crc(const bl_param_t *param)
{
    return cimc_bl_crc32_calc((const uint8_t *)param, (uint32_t)offsetof(bl_param_t, param_crc32));
}

static void cimc_bl_param_set_default(bl_param_t *param)
{
    memset(param, 0, sizeof(bl_param_t));
    param->magic = BL_PARAM_MAGIC;
    param->version = BL_PARAM_VERSION;
    param->update_flag = BL_UPDATE_FLAG_IDLE;
    param->app1_addr = BL_APP1_START_ADDR;
    param->staging_addr = BL_STAGING_START_ADDR;
    param->tail_magic = BL_PARAM_TAIL_MAGIC;
    param->param_crc32 = cimc_bl_param_calc_crc(param);
}

static bool cimc_bl_param_is_min_valid(const bl_param_t *param)
{
    return (param->magic == BL_PARAM_MAGIC) &&
           (param->tail_magic == BL_PARAM_TAIL_MAGIC) &&
           (param->version == BL_PARAM_VERSION);
}

__attribute__((section(".ramfunc"), noinline))
static bool cimc_bl_flash_wait_ready(void)
{
    uint32_t timeout = 0x3FFFFFUL;

    while((RESET != fmc_flag_get(FMC_FLAG_BUSY)) && (timeout > 0U)) {
        timeout--;
    }

    return (timeout > 0U);
}

__attribute__((section(".ramfunc"), noinline))
static void cimc_bl_flash_clear_flags(void)
{
    fmc_flag_clear(FMC_FLAG_END);
    fmc_flag_clear(FMC_FLAG_WPERR);
    fmc_flag_clear(FMC_FLAG_PGSERR);
    fmc_flag_clear(FMC_FLAG_PGMERR);
}

__attribute__((section(".ramfunc"), noinline))
static bool cimc_bl_flash_rewrite_param_page(const uint8_t *page_data)
{
    uint32_t addr;
    uint32_t word_value;
    fmc_state_enum fmc_state;

    if(page_data == NULL) {
        return false;
    }

    fmc_unlock();
    cimc_bl_flash_clear_flags();

    fmc_state = fmc_page_erase(BL_PARAM_START_ADDR);
    if((fmc_state != FMC_READY) || !cimc_bl_flash_wait_ready()) {
        fmc_lock();
        return false;
    }
    cimc_bl_flash_clear_flags();

    addr = BL_PARAM_START_ADDR;
    while(addr < (BL_PARAM_START_ADDR + BL_PARAM_SIZE)) {
        memcpy(&word_value, page_data, sizeof(word_value));
        fmc_state = fmc_word_program(addr, word_value);
        if((fmc_state != FMC_READY) || !cimc_bl_flash_wait_ready() || (*(volatile uint32_t *)addr != word_value)) {
            fmc_lock();
            return false;
        }
        addr += 4UL;
        page_data += 4U;
    }

    cimc_bl_flash_clear_flags();
    fmc_lock();
    return true;
}

void cimc_system_init(void)
{
    s_time_base_utc = cimc_rtc_to_utc();
    s_time_base_ms = get_system_ms();
}

void cimc_system_set_time(uint32_t utc_seconds)
{
    cimc_utc_to_rtc(utc_seconds, &rtc_initpara);
    (void)rtc_init(&rtc_initpara);
    RTC_BKP0 = BKP_VALUE;
    s_time_base_utc = utc_seconds;
    s_time_base_ms = get_system_ms();
}

uint32_t cimc_system_get_time(void)
{
    uint32_t elapsed_sec = (get_system_ms() - s_time_base_ms) / 1000U;
    return s_time_base_utc + elapsed_sec;
}

uint8_t cimc_system_get_baud_code(void)
{
    return cimc_param_get()->baud_code;
}

uint32_t cimc_system_baud_code_to_value(uint8_t baud_code)
{
    switch(baud_code) {
    case CIMC_BAUD_CODE_4800:
        return 4800U;
    case CIMC_BAUD_CODE_9600:
        return 9600U;
    case CIMC_BAUD_CODE_19200:
        return 19200U;
    case CIMC_BAUD_CODE_115200:
        return 115200U;
    default:
        return 19200U;
    }
}

void cimc_system_apply_baud_rate(void)
{
    bsp_usart1_init(cimc_system_baud_code_to_value(cimc_system_get_baud_code()));
}

bool cimc_system_request_bootloader_upgrade(void)
{
    bl_param_t main_param;
    bl_param_t backup_param;

    memcpy(s_bl_param_page_cache, (const void *)BL_PARAM_PAGE_ADDR, BL_PARAM_PAGE_SIZE);
    memcpy(&main_param, (const void *)BL_PARAM_MAIN_ADDR, sizeof(bl_param_t));
    memcpy(&backup_param, (const void *)BL_PARAM_BACKUP_ADDR, sizeof(bl_param_t));

    if(!cimc_bl_param_is_min_valid(&main_param)) {
        if(cimc_bl_param_is_min_valid(&backup_param)) {
            main_param = backup_param;
        } else {
            cimc_bl_param_set_default(&main_param);
        }
    }

    main_param.app_size = 0UL;
    main_param.app_crc32 = 0UL;
    main_param.app1_addr = BL_APP1_START_ADDR;
    main_param.staging_addr = BL_STAGING_START_ADDR;
    main_param.update_flag = BL_UPDATE_FLAG_FAST_UPGRADE;
    main_param.last_error = BL_ERR_NONE;
    main_param.tail_magic = BL_PARAM_TAIL_MAGIC;
    main_param.param_crc32 = cimc_bl_param_calc_crc(&main_param);
    backup_param = main_param;

    memcpy(&s_bl_param_page_cache[BL_PARAM_MAIN_ADDR - BL_PARAM_PAGE_ADDR], &main_param, sizeof(bl_param_t));
    memcpy(&s_bl_param_page_cache[BL_PARAM_BACKUP_ADDR - BL_PARAM_PAGE_ADDR], &backup_param, sizeof(bl_param_t));

    return cimc_bl_flash_rewrite_param_page(s_bl_param_page_cache);
}

void cimc_system_reboot(void)
{
    NVIC_SystemReset();
}

void cimc_system_enter_sleep_and_report_wakeup(void)
{
    static const uint8_t wakeup_text[] = "instrument wakeup\r\n";

    bsp_enter_deepsleep_for_seconds(10U);
    rs485_send_bytes(wakeup_text, (uint16_t)(sizeof(wakeup_text) - 1U));
}
