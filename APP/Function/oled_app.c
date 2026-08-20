#include "oled_app.h"

#define CIMC_TEAM_NUMBER "2026335222"


int oled_printf(uint8_t x, uint8_t y, const char *format, ...)
{
  char buffer[512];
  va_list arg;
  int len;

  va_start(arg, format);
  len = vsnprintf(buffer, sizeof(buffer), format, arg);
  va_end(arg);

  OLED_ShowStr(x, y, buffer, 8);
  return len;
}

static int oled_printf_16(uint8_t x, uint8_t y, const char *format, ...)
{
  char buffer[512];
  va_list arg;
  int len;

  va_start(arg, format);
  len = vsnprintf(buffer, sizeof(buffer), format, arg);
  va_end(arg);

  OLED_ShowStr(x, y, buffer, 16);
  return len;
}

void oled_show_bootloader_status(void)
{
    oled_printf_16(0, 2, "Bootloader");
}

void oled_task(void)
{
    static uint8_t initialized = 0U;
    static uint8_t last_auto_report = 0xFFU;
    uint8_t auto_report = cimc_data_auto_report_is_running() ? 1U : 0U;

    if (initialized == 0U)
    {
        OLED_Clear();
        oled_printf_16(0, 0, "%s", CIMC_TEAM_NUMBER);
        initialized = 1U;
    }
	
    if (auto_report != last_auto_report)
    {
        oled_printf_16(0, 2, auto_report ? "AutoSample" : "IDLE      ");
        last_auto_report = auto_report;
    }
}

