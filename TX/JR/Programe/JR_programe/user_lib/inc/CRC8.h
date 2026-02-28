#ifndef CRC8_H
#define CRC8_H
#include "main.h"

typedef struct __CRC8TypeDef
{
    uint8_t list[256];
} CRC8TypeDef;

void CRC8_init(CRC8TypeDef *crc, uint8_t poly);
uint8_t CRC8_calculate(CRC8TypeDef *crc, uint8_t *data, uint8_t len);

#endif