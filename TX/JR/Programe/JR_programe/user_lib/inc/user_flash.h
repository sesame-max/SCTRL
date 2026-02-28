#ifndef __USER_FLASH_H
#define __USER_FLASH_H
#include "main.h"
void user_flash_erase(uint8_t sector);
uint32_t user_flash_read(uint32_t addr);
void user_flash_write(uint32_t addr, uint32_t data);

#endif