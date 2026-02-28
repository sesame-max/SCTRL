#include "user_key.h"
#include "sLog.h"
#include "user_led.h"
#include "protocol.h"

keyTypeDef key1, key2;

void user_key_init(void)
{
    key_init(&key1, "key1", KEY_ACTIVATE_LOW);
    key_init(&key2, "key2", KEY_ACTIVATE_LOW);
    // key_set_double_click_flag(&mainKey, KEY_DOUBLE_CLICK_DISABLE);
    //  key_set_long_press_flag(&mainKey, KEY_LONG_PRESS_DISABLE);
}

keyLevel key_get_status(keyTypeDef *key)
{
    if (key == &key1)
    {
        /* return keyLevel */
        return (keyLevel)HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin);
    }
    else if (key == &key2)
    {
        /* return keyLevel */
        return (keyLevel)HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin);
    }
    return KEY_LOW;
}

void key_pressed_callback(keyTypeDef *key)
{
    if (key == &key1)
    {
        /* head key pressed event */
        sLog_print("%s pressed\r\n", key->name);
    }
    else if (key == &key1)
    {
        /* head key pressed event */
        sLog_print("%s pressed\r\n", key->name);
    }
}

void key_released_callback(keyTypeDef *key)
{
    if (key == &key1)
    {
        /* head key released event */
        sLog_print("%s released\r\n", key->name);
    }
    else if (key == &key2)
    {
        /* head key released event */
        sLog_print("%s released\r\n", key->name);
    }
}

void key_click_callback(keyTypeDef *key)
{
    if (key == &key1)
    {
        /* head key released event */
        sLog_print("%s clicked\r\n", key->name);
    }
    else if (key == &key2)
    {
        /* head key released event */
        sLog_print("%s clicked\r\n", key->name);
    }
}

void key_double_click_callback(keyTypeDef *key)
{
    if (key == &key1)
    {
        /* head key released event */
        sLog_print("%s double clicked\r\n", key->name);
        protocol_clean_bind_req();
    }
    else if (key == &key2)
    {
        /* head key released event */
        sLog_print("%s double clicked\r\n", key->name);
    }
}

void key_long_press_callback(keyTypeDef *key)
{
    if (key == &key1)
    {
        /* head key long press event */
        sLog_print("%s long press\r\n", key->name);
        protocol_bind_req();
    }
    else if (key == &key2)
    {
        /* head key long press event */
        sLog_print("%s long press\r\n", key->name);
    }
}

void key_click_long_press_callback(keyTypeDef *key)
{
    if (key == &key1)
    {
        /* head key click long press event */
        sLog_print("%s click long press\r\n", key->name);
        user_led_connected();
    }
    else if (key == &key2)
    {
        /* head key click long press event */
        sLog_print("%s click long press\r\n", key->name);
    }
}