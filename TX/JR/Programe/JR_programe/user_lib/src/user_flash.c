#include "main.h"
#include "user_flash.h"

void user_flash_erase(uint8_t sector)
{
    uint32_t PageErr = 0;
    FLASH_EraseInitTypeDef Flash;
    HAL_FLASH_Unlock();
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);
    Flash.TypeErase = FLASH_TYPEERASE_SECTORS;
    Flash.Sector = sector;
    Flash.NbSectors = 1;
    Flash.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    HAL_FLASHEx_Erase(&Flash, &PageErr);
    HAL_FLASH_Lock();
}
uint32_t user_flash_read(uint32_t addr)
{
    uint32_t readData;
    readData = *(uint32_t *)addr;
    return readData;
}
void user_flash_write(uint32_t addr, uint32_t data)
{
    HAL_FLASH_Unlock();
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);
    HAL_FLASH_Program(TYPEPROGRAM_WORD, addr, data);
    HAL_FLASH_Lock();
}
