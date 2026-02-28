#include "main.h"
#include "tim.h"
#include "spi.h"
#include "st7735s.h"
#include "LCD.h"
#include "LCD_front.h"

uint8_t LCD_buff[LCD_W * LCD_H * 2] = {0};
/// @brief LCD刷新一次
/// @param  *
void LCD_refresh(void)
{
    LCD_CS_Clr();
    HAL_SPI_Transmit_DMA(&hspi1, LCD_buff, sizeof(LCD_buff));
}

/// @brief 设置LCD背光亮度
/// @param backlight
void LCD_set_backlight(uint16_t backlight)
{
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, backlight);
}

/// @brief 在指定位置画点
/// @param x 画点坐标
/// @param y 画点坐标
/// @param color 点的颜色
void LCD_DrawPoint(uint16_t x, uint16_t y, uint16_t color)
{
    LCD_buff[(y * LCD_W + x) * 2] = color >> 8;
    LCD_buff[(y * LCD_W + x) * 2 + 1] = color;
}
/// @brief LCD清屏
/// @param color 清屏的颜色
void LCD_Clr(uint16_t color)
{
    LCD_Fill(0, 0, 159, 79, color);
    HAL_Delay(2);
}

/// @brief 在指定区域填充颜色
/// @param xsta 起始坐标
/// @param ysta 起始坐标
/// @param xend 终止坐标
/// @param yend 终止坐标
/// @param color 要填充的颜色
void LCD_Fill(uint16_t xsta, uint16_t ysta, uint16_t xend, uint16_t yend, uint16_t color)
{
    uint16_t i, j;
    for (i = xsta; i < xend; i++)
    {
        for (j = ysta; j < yend; j++)
        {
            LCD_DrawPoint(i, j, color);
        }
    }
}

/// @brief 画线
/// @param x1 起始坐标
/// @param y1 起始坐标
/// @param x2 终止坐标
/// @param y2 终止坐标
/// @param color 线的颜色
void LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    uint16_t t;
    int xerr = 0, yerr = 0, delta_x, delta_y, distance;
    int incx, incy, uRow, uCol;
    delta_x = x2 - x1; // 计算坐标增量
    delta_y = y2 - y1;
    uRow = x1; // 画线起点坐标
    uCol = y1;
    if (delta_x > 0)
        incx = 1; // 设置单步方向
    else if (delta_x == 0)
        incx = 0; // 垂直线
    else
    {
        incx = -1;
        delta_x = -delta_x;
    }
    if (delta_y > 0)
        incy = 1;
    else if (delta_y == 0)
        incy = 0; // 水平线
    else
    {
        incy = -1;
        delta_y = -delta_y;
    }
    if (delta_x > delta_y)
        distance = delta_x; // 选取基本增量坐标轴
    else
        distance = delta_y;
    for (t = 0; t < distance + 1; t++)
    {
        LCD_DrawPoint(uRow, uCol, color); // 画点
        xerr += delta_x;
        yerr += delta_y;
        if (xerr > distance)
        {
            xerr -= distance;
            uRow += incx;
        }
        if (yerr > distance)
        {
            yerr -= distance;
            uCol += incy;
        }
    }
}

/// @brief 画矩形
/// @param x1 起始坐标
/// @param y1 起始坐标
/// @param x2 终止坐标
/// @param y2 终止坐标
/// @param color 矩形的颜色
void LCD_DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    LCD_DrawLine(x1, y1, x2, y1, color);
    LCD_DrawLine(x1, y1, x1, y2, color);
    LCD_DrawLine(x1, y2, x2, y2, color);
    LCD_DrawLine(x2, y1, x2, y2, color);
}

/// @brief 画圆
/// @param x0 圆心坐标
/// @param y0 圆心坐标
/// @param r 半径
/// @param color 圆的颜色
void Draw_Circle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color)
{
    int a, b;
    a = 0;
    b = r;
    while (a <= b)
    {
        LCD_DrawPoint(x0 - b, y0 - a, color); // 3
        LCD_DrawPoint(x0 + b, y0 - a, color); // 0
        LCD_DrawPoint(x0 - a, y0 + b, color); // 1
        LCD_DrawPoint(x0 - a, y0 - b, color); // 2
        LCD_DrawPoint(x0 + b, y0 + a, color); // 4
        LCD_DrawPoint(x0 + a, y0 - b, color); // 5
        LCD_DrawPoint(x0 + a, y0 + b, color); // 6
        LCD_DrawPoint(x0 - b, y0 + a, color); // 7
        a++;
        if ((a * a + b * b) > (r * r)) // 判断要画的点是否过远
        {
            b--;
        }
    }
}

/// @brief 显示单个字符
/// @param x 显示坐标
/// @param y 显示坐标
/// @param num 要显示的字符
/// @param fc 字的颜色
/// @param bc 字的背景色
/// @param sizey 字号
/// @param mode 0非叠加模式  1叠加模式
void LCD_ShowChar(uint16_t x, uint16_t y, char num, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode)
{
    uint8_t temp, sizex, t, m = 0;
    uint16_t i, TypefaceNum; // 一个字符所占字节大小
    uint16_t x0 = x;
    sizex = sizey / 2;
    TypefaceNum = (sizex / 8 + ((sizex % 8) ? 1 : 0)) * sizey;
    num = num - ' '; // 得到偏移后的值
    for (i = 0; i < TypefaceNum; i++)
    {
        if (sizey == 12)
            temp = ascii_1206[num][i]; // 调用6x12字体
        else if (sizey == 16)
            temp = ascii_1608[num][i]; // 调用8x16字体
        else if (sizey == 24)
            temp = ascii_2412[num][i]; // 调用12x24字体
        else if (sizey == 32)
            temp = ascii_3216[num][i]; // 调用16x32字体
        else
            return;
        for (t = 0; t < 8; t++)
        {
            if (!mode) // 非叠加模式
            {
                if (temp & (0x01 << t))
                {
                    LCD_DrawPoint(x, y, fc); // 画一个点
                }
                else
                {
                    LCD_DrawPoint(x, y, bc); // 画一个点
                }
                x++;
                if ((x - x0) == sizex)
                {
                    x = x0;
                    y++;
                    break;
                }
            }
            else // 叠加模式
            {
                if (temp & (0x01 << t))
                    LCD_DrawPoint(x, y, fc); // 画一个点
                x++;
                if ((x - x0) == sizex)
                {
                    x = x0;
                    y++;
                    break;
                }
            }
        }
    }
}

/// @brief 显示字符串
/// @param x 显示坐标
/// @param y 显示坐标
/// @param p 要显示的字符串
/// @param fc 字的颜色
/// @param bc 字的背景色
/// @param sizey 字号
/// @param mode 0非叠加模式  1叠加模式
void LCD_ShowString(uint16_t x, uint16_t y, char *p, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode)
{
    while (*p != '\0')
    {
        LCD_ShowChar(x, y, *p, fc, bc, sizey, mode);
        x += sizey / 2;
        p++;
    }
}

/// @brief 显示数字
/// @param m 底数
/// @param n 指数
/// @return
uint32_t mypow(uint8_t m, uint8_t n)
{
    uint32_t result = 1;
    while (n--)
        result *= m;
    return result;
}

/// @brief 显示整数变量
/// @param x 显示坐标
/// @param y 显示坐标
/// @param num 要显示整数变量
/// @param len 要显示的位数
/// @param fc 字的颜色
/// @param bc 字的背景色
/// @param sizey 字号
void LCD_ShowIntNum(uint16_t x, uint16_t y, uint16_t num, uint8_t len, uint16_t fc, uint16_t bc, uint8_t sizey)
{
    uint8_t t, temp;
    uint8_t enshow = 0;
    uint8_t sizex = sizey / 2;
    for (t = 0; t < len; t++)
    {
        temp = (num / mypow(10, len - t - 1)) % 10;
        if (enshow == 0 && t < (len - 1))
        {
            if (temp == 0)
            {
                LCD_ShowChar(x + t * sizex, y, '0', fc, bc, sizey, 0);
                continue;
            }
            else
                enshow = 1;
        }
        LCD_ShowChar(x + t * sizex, y, temp + 48, fc, bc, sizey, 0);
    }
}

/// @brief 显示两位小数变量
/// @param x 显示坐标
/// @param y 显示坐标
/// @param num 要显示小数变量
/// @param len 要显示的位数
/// @param fc 字的颜色
/// @param bc 字的背景色
/// @param sizey 字号
void LCD_ShowFloatNum1(uint16_t x, uint16_t y, float num, uint8_t len, uint16_t fc, uint16_t bc, uint8_t sizey)
{
    uint8_t t, temp, sizex;
    uint16_t num1;
    sizex = sizey / 2;
    num1 = num * 100;
    for (t = 0; t < len; t++)
    {
        temp = (num1 / mypow(10, len - t - 1)) % 10;
        if (t == (len - 2))
        {
            LCD_ShowChar(x + (len - 2) * sizex, y, '.', fc, bc, sizey, 0);
            t++;
            len += 1;
        }
        LCD_ShowChar(x + t * sizex, y, temp + 48, fc, bc, sizey, 0);
    }
}

void LCD_print(uint8_t *chr, uint8_t len)
{
    uint8_t i = 0;
    uint8_t line, row;
    for (line = 71; line > 11; line--)
    {
        for (row = 0; row < 160; row++)
        {
            LCD_buff[line * 320 + row * 2] = LCD_buff[(line - 12) * 320 + row * 2];
            LCD_buff[line * 320 + row * 2 + 1] = LCD_buff[(line - 12) * 320 + row * 2 + 1];
        }
    }
    LCD_Fill(0, 0, 159, 11, 0x0000);
    if (len > 26)
    {
        len = 26;
    }
    while (i < len)
    {
        if (chr[i] <= '~' && chr[i] >= ' ')
        {
            LCD_ShowChar(i * 6, 0, chr[i], 0xffff, 0x0000, 12, 0);
        }
        else
        {
            LCD_ShowChar(i * 6, 0, ' ', 0xffff, 0x0000, 12, 0);
        }
        i++;
    }
}

/// @brief 显示图片
/// @param x 起点坐标
/// @param y 起点坐标
/// @param length 图片长度
/// @param width 图片宽度
/// @param pic 图片数组
void LCD_ShowPicture(uint16_t x, uint16_t y, uint16_t length, uint16_t width, const uint8_t pic[])
{
    uint16_t i, j;
    uint32_t k = 0;
    for (i = 0; i < length; i++)
    {
        for (j = 0; j < width; j++)
        {
            LCD_DrawPoint(x + i, y + j, ((pic[k * 2]) | pic[k * 2 + 1] << 8));
            k++;
        }
    }
}
