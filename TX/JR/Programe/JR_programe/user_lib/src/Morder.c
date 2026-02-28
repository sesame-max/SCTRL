#include "main.h"
#include "Morder.h"

static const uint16_t feedback[9] = {0203, 0211, 0217, 0235, 0277, 0313, 0325, 0345, 0367};
void Morder_generate(uint8_t seed, uint8_t coefficient, uint8_t *order)
{
    uint8_t i, j;
    uint8_t dect, cnt, temp, rate;
    temp = feedback[coefficient];
    temp = temp & 0xfe;
    for (i = 0; i < 8; i++)
    {
        rate = (rate << 1) | (temp & 0x01);
        temp = temp >> 1;
    }

    order[0] = (seed % 127) + 1;

    for (i = 1; i < 127; i++)
    {
        cnt = 0;
        dect = order[i - 1] & rate;
        while (dect > 0)
        {
            dect = dect & (dect - 1);
            cnt++;
        }

        if (cnt % 2)
        {
            order[i] = (order[i - 1] >> 1) | 0x40;
        }
        else
        {
            order[i] = order[i - 1] >> 1;
        }
    }
}
