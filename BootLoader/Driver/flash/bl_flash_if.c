#include "gd32f4xx.h"
#include "bl_config.h"
#include "bl_flash_if.h"
#include <string.h>

static void bl_flash_clear_flags(void)
{
    fmc_flag_clear(FMC_FLAG_END);
    fmc_flag_clear(FMC_FLAG_WPERR);
    fmc_flag_clear(FMC_FLAG_PGSERR);
    fmc_flag_clear(FMC_FLAG_PGMERR);
}

static bool bl_flash_wait_ready(void)
{
    uint32_t timeout = 0x3FFFFFUL;
    while ((RESET != fmc_flag_get(FMC_FLAG_BUSY)) && (timeout > 0UL))
    {
        timeout--;
    }
    return (timeout > 0UL);
}

bool bl_flash_erase(uint32_t addr, uint32_t size)
{
    uint32_t page_addr;
    uint32_t end_addr;

    if ((size == 0UL) || (addr < BL_FLASH_BASE_ADDR))
    {
        return false;
    }

    end_addr = addr + size - 1UL;
    if (end_addr > BL_FLASH_END_ADDR)
    {
        return false;
    }


    page_addr = addr - (addr % BL_FLASH_PAGE_SIZE);
    end_addr = end_addr - (end_addr % BL_FLASH_PAGE_SIZE);

    fmc_unlock();
    bl_flash_clear_flags();

    while (page_addr <= end_addr)
    {
        fmc_page_erase(page_addr);
        if (!bl_flash_wait_ready())
        {
            fmc_lock();
            return false;
        }
        bl_flash_clear_flags();
        page_addr += BL_FLASH_PAGE_SIZE;
    }

    fmc_lock();
    return true;
}

bool bl_flash_program(uint32_t addr, const uint8_t *data, size_t size)
{
    size_t i;
    uint32_t word_value;

    if ((data == 0) || (size == 0U))
    {
        return false;
    }
    if ((addr < BL_FLASH_BASE_ADDR) || ((addr + size - 1U) > BL_FLASH_END_ADDR))
    {
        return false;
    }

    fmc_unlock();
    bl_flash_clear_flags();


    while (((addr & 3UL) == 0UL) && (size >= 4U))
    {
        memcpy(&word_value, data, sizeof(word_value));
        fmc_word_program(addr, word_value);
        if (!bl_flash_wait_ready())
        {
            fmc_lock();
            return false;
        }
        addr += 4UL;
        data += 4U;
        size -= 4U;
    }

    for (i = 0U; i < size; i++)
    {
        fmc_byte_program(addr + (uint32_t)i, data[i]);
        if (!bl_flash_wait_ready())
        {
            fmc_lock();
            return false;
        }
    }

    bl_flash_clear_flags();
    fmc_lock();
    return true;
}

bool bl_flash_program_param(const void *param, size_t size)
{
    if ((param == 0) || (size == 0U) || (size > BL_PARAM_SIZE))
    {
        return false;
    }

    if (!bl_flash_erase(BL_PARAM_START_ADDR, BL_PARAM_SIZE))
    {
        return false;
    }


    return bl_flash_program(BL_PARAM_START_ADDR, (const uint8_t *)param, size);
}

bool bl_flash_compare_bytes(uint32_t addr, const uint8_t *data, uint32_t size)
{
    uint32_t i;

    if ((data == 0) || (size == 0UL))
    {
        return false;
    }

    if ((addr < BL_FLASH_BASE_ADDR) || ((addr + size - 1UL) > BL_FLASH_END_ADDR))
    {
        return false;
    }

    for (i = 0UL; i < size; i++)
    {
        if (*(volatile uint8_t *)(addr + i) != data[i])
        {
            return false;
        }
    }

    return true;
}

bool bl_flash_compare_flash(uint32_t addr_a, uint32_t addr_b, uint32_t size)
{
    uint32_t i;

    if (size == 0UL)
    {
        return false;
    }

    if ((addr_a < BL_FLASH_BASE_ADDR) || ((addr_a + size - 1UL) > BL_FLASH_END_ADDR) ||
        (addr_b < BL_FLASH_BASE_ADDR) || ((addr_b + size - 1UL) > BL_FLASH_END_ADDR))
    {
        return false;
    }

    for (i = 0UL; i < size; i++)
    {
        if (*(volatile uint8_t *)(addr_a + i) != *(volatile uint8_t *)(addr_b + i))
        {
            return false;
        }
    }

    return true;
}
