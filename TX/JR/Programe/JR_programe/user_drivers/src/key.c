#include "key.h"
#include "sLog.h"

static keyTypeDef headKey = {NULL};

void key_init(keyTypeDef *key, const char *name, keyActivateLevel level)
{
    key->name = name;

    key->activateLevel = level;

    key->doubleClickFlag = KEY_DOUBLE_CLICK_ENABLE;
    key->longPressFlag = KEY_LONG_PRESS_ENABLE;

    key->pressDebounceTime = KEY_PRESS_DEBOUNCE_TIME_DEFAULT;
    key->releaseDebounceTime = KEY_RELEASE_DEBOUNCE_TIME_DEFAULT;

    key->longPressTimeThreshold = KEY_LONG_PPRESS_TIME_DEFAULT;
    key->clickConfirmThreshold = KEY_CLICK_CONFIRM_THRESHOLD_DEFAULT;

    key->pressTime = 0;
    key->releaseTime = 0;
    key->pressedFlag = KEY_NO_PRESSED;

    key->nextKey = headKey.nextKey;
    headKey.nextKey = key;
}

void key_tick(void)
{
    keyTypeDef *key = headKey.nextKey;
    while (key != NULL)
    {
        switch (key->status)
        {
        case KEY_IDEL_STATUS:
            if (key_get_status(key) == (keyLevel)key->activateLevel)
            {
                key->status = KEY_PRESS_STATUS;
            }
            break;
        case KEY_PRESS_STATUS:
            if (key_get_status(key) == (keyLevel)key->activateLevel)
            {
                key->pressTime += KEY_TICK_MS;
                key->releaseTime = 0;
                if ((key->pressedFlag == KEY_NO_PRESSED) && (key->pressTime > key->pressDebounceTime))
                {
                    key_pressed_callback(key);
                    key->pressedFlag = KEY_PRESSED;
                }

                if ((key->longPressFlag == KEY_LONG_PRESS_ENABLE) && (key->pressTime > key->longPressTimeThreshold))
                {
                    key_long_press_callback(key);
                    key->pressTime = 0;
                    key->pressedFlag = KEY_NO_PRESSED;
                    key->status = KEY_LONG_PRESS_STATUS;
                }
            }
            else
            {
                key->releaseTime += KEY_TICK_MS;
                key->pressTime = 0;
                if (key->pressedFlag == KEY_NO_PRESSED)
                {
                    key->releaseTime = 0;
                    key->status = KEY_IDEL_STATUS;
                }
                else if (key->releaseTime > key->releaseDebounceTime)
                {
                    key->releaseTime = 0;
                    key->pressedFlag = KEY_NO_PRESSED;
                    key_released_callback(key);
                    if (key->doubleClickFlag == KEY_DOUBLE_CLICK_ENABLE)
                    {
                        key->status = KEY_RELEASE_STATUS;
                    }
                    else
                    {
                        key_click_callback(key);
                        key->status = KEY_IDEL_STATUS;
                    }
                }
            }
            break;
        case KEY_RELEASE_STATUS:
            if (key_get_status(key) == (keyLevel)key->activateLevel)
            {
                key->pressTime += KEY_TICK_MS;
                key->releaseTime = 0;
                if ((key->pressedFlag == KEY_NO_PRESSED) && (key->pressTime > key->pressDebounceTime))
                {
                    key_pressed_callback(key);
                    key->pressedFlag = KEY_PRESSED;
                }
                if ((key->longPressFlag == KEY_LONG_PRESS_ENABLE) && (key->pressTime > key->longPressTimeThreshold))
                {
                    key->pressTime = 0;
                    key_click_long_press_callback(key);
                    key->pressedFlag = KEY_NO_PRESSED;
                    key->status = KEY_LONG_PRESS_STATUS;
                }
            }
            else
            {
                key->releaseTime += KEY_TICK_MS;
                key->pressTime = 0;
                if (key->pressedFlag == KEY_PRESSED)
                {
                    if (key->releaseTime > key->releaseDebounceTime)
                    {
                        key_released_callback(key);
                        key_double_click_callback(key);
                        key->pressedFlag = KEY_NO_PRESSED;
                        key->status = KEY_IDEL_STATUS;
                    }
                }
                else
                {
                    if (key->releaseTime > key->clickConfirmThreshold)
                    {
                        key_click_callback(key);
                        key->status = KEY_IDEL_STATUS;
                    }
                }
            }
            break;
        case KEY_LONG_PRESS_STATUS:
            if (key_get_status(key) == (keyLevel)key->activateLevel)
            {
                key->releaseTime = 0;
            }
            else
            {
                key->releaseTime += KEY_TICK_MS;
                if (key->releaseTime > key->releaseDebounceTime)
                {
                    key_released_callback(key);
                    key->status = KEY_IDEL_STATUS;
                }
            }
            break;
        default:
            break;
        }
        key = key->nextKey;
    }
}

__weak void key_pressed_callback(keyTypeDef *key)
{
    if (key == &headKey)
    {
        /* head key pressed event */
    }
}

__weak void key_released_callback(keyTypeDef *key)
{
    if (key == &headKey)
    {
        /* head key released event */
    }
}

__weak keyLevel key_get_status(keyTypeDef *key)
{
    if (key == &headKey)
    {
        /* return keyLevel */
    }
    return KEY_LOW;
}
__weak void key_click_callback(keyTypeDef *key)
{
    if (key == &headKey)
    {
        /* head key clicked event */
    }
}

__weak void key_double_click_callback(keyTypeDef *key)
{
    if (key == &headKey)
    {
        /* head key double click event */
    }
}

__weak void key_long_press_callback(keyTypeDef *key)
{
    if (key == &headKey)
    {
        /* head key long press event */
    }
}

__weak void key_click_long_press_callback(keyTypeDef *key)
{
    if (key == &headKey)
    {
        /* head key click long press event */
    }
}

void key_set_double_click_flag(keyTypeDef *key, keyDoubleClickFlag flag)
{
    key->doubleClickFlag = flag;
}

void key_set_long_press_flag(keyTypeDef *key, keyLongPressFlag flag)
{
    key->longPressFlag = flag;
}

void key_set_press_debounce_time(keyTypeDef *key, uint32_t time)
{
    key->pressDebounceTime = time;
}

void key_set_release_debounce_time(keyTypeDef *key, uint32_t time)
{
    key->releaseDebounceTime = time;
}

void key_set_long_press_time_threshold(keyTypeDef *key, uint32_t threshold)
{
    key->longPressTimeThreshold = threshold;
}

void key_set_click_confirm_threshold(keyTypeDef *key, uint32_t threshold)
{
    key->clickConfirmThreshold = threshold;
}