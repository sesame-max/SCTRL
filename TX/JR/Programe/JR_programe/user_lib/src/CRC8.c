#include "main.h"
#include "CRC8.h"

/// @brief 初始化CRC
/// @param poly CRC系数
void CRC8_init(CRC8TypeDef *crc, uint8_t poly)
{
    uint16_t i;
    uint8_t CRCVale, j;
    for (i = 0; i < 256; i++)
    {
        CRCVale = i;
        for (j = 0; j < 8; j++)
        {
            if (CRCVale & 0x80)
            {
                CRCVale = (CRCVale << 1) ^ poly;
            }
            else
            {
                CRCVale = (CRCVale << 1) ^ 0;
            }
        }
        crc->list[i] = CRCVale & 0xff;
    }
}
/// @brief 计算数据CRC值
/// @param data 数据指针
/// @param len 数据长度
/// @return CRC值
uint8_t CRC8_calculate(CRC8TypeDef *crc, uint8_t *data, uint8_t len)
{
    uint8_t CRCVale = 0;
    while (len--)
    {
        CRCVale = crc->list[CRCVale ^ *data++];
    }
    return CRCVale;
}