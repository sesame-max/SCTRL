#include "main.h"
#include "tim.h"
#include "key.h"
#include "led.h"
#include "protocol.h"
#include "LCD.h"

extern uint16_t rxFlag;
extern uint8_t channelNum, sendChannel[8];
extern int8_t NRF_mode;
uint8_t uart_send_data[2];
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	static uint8_t cnt;

	static uint8_t FPS;
	if (htim->Instance == TIM10) // 5ms
	{
		LCD_refresh(); // 启动LCD刷新，写入内存数据到LCD
		key_tick();
		led_tick();
	}
	else if (htim->Instance == TIM11) // 跳频定时器
	{
		protocol_timer_callback();
	}
	else if (htim->Instance == TIM9)
	{
		protocol_tick();
	}
}
