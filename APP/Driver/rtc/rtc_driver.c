#include "rtc_driver.h"


rtc_parameter_struct rtc_initpara;
__IO uint32_t prescaler_a = 0, prescaler_s = 0;
uint32_t RTCSRC_FLAG = 0;

static int bsp_rtc_setup(void)
{
    int ret = 0;
    uint32_t tmp_hh = 0x00, tmp_mm = 0x00, tmp_ss = 0x00;

    rtc_initpara.factor_asyn = prescaler_a;
    rtc_initpara.factor_syn = prescaler_s;
    rtc_initpara.year = 0x26;
    rtc_initpara.day_of_week = RTC_THURSDAY;
    rtc_initpara.month = RTC_JAN;
    rtc_initpara.date = 0x01;
    rtc_initpara.display_format = RTC_24HOUR;
    rtc_initpara.am_pm = RTC_AM;

    rtc_initpara.hour = tmp_hh;
    rtc_initpara.minute = tmp_mm;
    rtc_initpara.second = tmp_ss;

    if(ERROR == rtc_init(&rtc_initpara)) {
        ret = -1;
    } else {
        RTC_BKP0 = BKP_VALUE;
    }
    return ret;
}

static void bsp_rtc_pre_cfg(void)
{
#if defined (RTC_CLOCK_SOURCE_IRC32K)
    rcu_osci_on(RCU_IRC32K);
    rcu_osci_stab_wait(RCU_IRC32K);
    rcu_rtc_clock_config(RCU_RTCSRC_IRC32K);

    prescaler_s = 0x13F;
    prescaler_a = 0x63;
#elif defined (RTC_CLOCK_SOURCE_LXTAL)
    rcu_osci_on(RCU_LXTAL);
    rcu_osci_stab_wait(RCU_LXTAL);
    rcu_rtc_clock_config(RCU_RTCSRC_LXTAL);

    prescaler_s = 0xFF;
    prescaler_a = 0x7F;
#else
#error RTC clock source should be defined.
#endif

    rcu_periph_clock_enable(RCU_RTC);
    rtc_register_sync_wait();
}

int bsp_rtc_init(void)
{
    int ret = 0;

    rcu_periph_clock_enable(RCU_PMU);
    pmu_backup_write_enable();

    bsp_rtc_pre_cfg();
    RTCSRC_FLAG = GET_BITS(RCU_BDCTL, 8, 9);

    if((BKP_VALUE != RTC_BKP0) || (0x00 == RTCSRC_FLAG)) {
        ret = bsp_rtc_setup();
    } else {
        if(RESET != rcu_flag_get(RCU_FLAG_PORRST)) {
            ret = 1;
        } else if(RESET != rcu_flag_get(RCU_FLAG_EPRST)) {
            ret = 2;
        }
    }

    rcu_all_reset_flag_clear();
    return ret;
}

void bsp_rtc_wakeup_config(uint16_t seconds)
{
    if(seconds == 0U) {
        return;
    }

    rcu_periph_clock_enable(RCU_PMU);
    pmu_backup_write_enable();
    rcu_periph_clock_enable(RCU_RTC);
    rtc_register_sync_wait();

    rtc_interrupt_disable(RTC_INT_WAKEUP);
    (void)rtc_wakeup_disable();
    rtc_flag_clear(RTC_FLAG_WT);
    exti_interrupt_flag_clear(EXTI_22);

    (void)rtc_wakeup_clock_set(WAKEUP_CKSPRE);
    (void)rtc_wakeup_timer_set((uint16_t)(seconds - 1U));

    exti_init(EXTI_22, EXTI_INTERRUPT, EXTI_TRIG_RISING);
    rtc_interrupt_enable(RTC_INT_WAKEUP);
    nvic_irq_enable(RTC_WKUP_IRQn, 1U, 0U);
    rtc_wakeup_enable();
}

void bsp_rtc_wakeup_deconfig(void)
{
    rtc_interrupt_disable(RTC_INT_WAKEUP);
    (void)rtc_wakeup_disable();
    rtc_flag_clear(RTC_FLAG_WT);
    exti_interrupt_flag_clear(EXTI_22);
    nvic_irq_disable(RTC_WKUP_IRQn);
}
