#include "user_led.h"
#include "led.h"

static ledTypedef ledR, ledG, ledB;

void led_set_level(ledTypedef *led, ledActivateLevel level)
{
    if (led == &ledR)
    {
        HAL_GPIO_WritePin(LED_R_GPIO_Port, LED_R_Pin, (GPIO_PinState)level);
    }
    else if (led == &ledG)
    {
        HAL_GPIO_WritePin(LED_G_GPIO_Port, LED_G_Pin, (GPIO_PinState)level);
    }
    else if (led == &ledB)
    {
        HAL_GPIO_WritePin(LED_B_GPIO_Port, LED_B_Pin, (GPIO_PinState)level);
    }
}

void user_led_init(void)
{
    led_init(&ledR, LED_ACTIVATE_LOW);
    led_init(&ledG, LED_ACTIVATE_LOW);
    led_init(&ledB, LED_ACTIVATE_LOW);
}

void user_led_set_error(void)
{
    led_set_on(&ledB);
}
void user_led_clean_error(void)
{
    led_set_off(&ledB);
}

void user_led_binding(void)
{
    led_set_blink(&ledR, 20, 100, LED_INFINITY_CYCLE);
}
void user_led_loss_signal(void)
{
    led_set_blink(&ledR, 20, 1000, LED_INFINITY_CYCLE);
}
void user_led_connected(void)
{
    led_set_on(&ledR);
}
