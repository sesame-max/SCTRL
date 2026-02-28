#ifndef __KEY_H
#define __KEY_H

#include "main.h"

#define KEY_TICK_MS 5

#define KEY_PRESS_DEBOUNCE_TIME_DEFAULT 4
#define KEY_RELEASE_DEBOUNCE_TIME_DEFAULT 4
#define KEY_CLICK_CONFIRM_THRESHOLD_DEFAULT 250
#define KEY_LONG_PPRESS_TIME_DEFAULT 750

typedef enum
{
    KEY_ACTIVATE_LOW = 0,
    KEY_ACTIVATE_HIGH
} keyActivateLevel;

typedef enum
{
    KEY_LOW = 0,
    KEY_HIGH
} keyLevel;

typedef enum
{
    KEY_DOUBLE_CLICK_ENABLE = 0,
    KEY_DOUBLE_CLICK_DISABLE
} keyDoubleClickFlag;

typedef enum
{
    KEY_LONG_PRESS_ENABLE = 0,
    KEY_LONG_PRESS_DISABLE
} keyLongPressFlag;

typedef enum
{
    KEY_PRESSED = 0,
    KEY_NO_PRESSED
} keyPressedFlag;

typedef enum
{
    KEY_IDEL_STATUS = 0,
    KEY_PRESS_STATUS,
    KEY_CLICK_STATUS,
    KEY_LONG_PRESS_STATUS,
    KEY_RELEASE_STATUS,
    KEY_DOUBLE_CLICK_STATUS
} keyStatus;

typedef struct __keyTypeDef keyTypeDef;
struct __keyTypeDef
{
    keyTypeDef *nextKey;
    const char *name;
    keyStatus status;
    keyActivateLevel activateLevel;
    keyPressedFlag pressedFlag;
    keyDoubleClickFlag doubleClickFlag;
    keyLongPressFlag longPressFlag;
    uint32_t pressTime;
    uint32_t releaseTime;
    uint32_t pressDebounceTime;
    uint32_t releaseDebounceTime;
    uint32_t longPressTimeThreshold;
    uint32_t clickConfirmThreshold;
};

keyLevel key_get_status(keyTypeDef *key);

void key_init(keyTypeDef *key, const char *name, keyActivateLevel level);
void key_tick(void);

void key_pressed_callback(keyTypeDef *key);
void key_released_callback(keyTypeDef *key);
void key_click_callback(keyTypeDef *key);
void key_double_click_callback(keyTypeDef *key);
void key_long_press_callback(keyTypeDef *key);
void key_click_long_press_callback(keyTypeDef *key);

void key_set_double_click_flag(keyTypeDef *key, keyDoubleClickFlag flag);
void key_set_long_press_flag(keyTypeDef *key, keyLongPressFlag flag);
void key_set_press_debounce_time(keyTypeDef *key, uint32_t time);
void key_set_release_debounce_time(keyTypeDef *key, uint32_t time);
void key_set_long_press_time_threshold(keyTypeDef *key, uint32_t threshold);
void key_set_click_confirm_threshold(keyTypeDef *key, uint32_t threshold);

#endif
