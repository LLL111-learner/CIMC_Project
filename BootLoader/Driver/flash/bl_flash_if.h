#ifndef BL_FLASH_IF_H
#define BL_FLASH_IF_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

bool bl_flash_erase(uint32_t addr, uint32_t size);
bool bl_flash_program(uint32_t addr, const uint8_t *data, size_t size);
bool bl_flash_program_param(const void *param, size_t size);
bool bl_flash_compare_bytes(uint32_t addr, const uint8_t *data, uint32_t size);
bool bl_flash_compare_flash(uint32_t addr_a, uint32_t addr_b, uint32_t size);

#endif
