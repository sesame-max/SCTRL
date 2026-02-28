#ifndef __LED_H
#define __LED_H
#include "main.h"

#define LED_TICK_MS 5
#define LED_FREQ_CAL_PERIOD 1000

#define LED_INFINITY_CYCLE 0

typedef enum
{
    LED_ACTIVATE_LOW = 0,
    LED_ACTIVATE_HIGH
} ledActivateLevel;

typedef enum
{
    LED_ACTIVATE = 0,
    LED_INACTIVATE
} ledActivateState;

typedef enum
{
    LED_OFF = 0,
    LED_ON,
    LED_BLINK,
    LED_FREQ
} ledStatus;

typedef struct __ledTypeDef ledTypedef;

struct __ledTypeDef
{
    ledTypedef *next;
    ledActivateLevel activateLevel;
    ledActivateState activateState;
    ledStatus status;

    uint32_t timeCnt;
    uint32_t onTime;
    uint32_t cyclePeriod;

    uint32_t cycleCnt;
    uint32_t cyclesNumber;

    uint32_t trigerCnt;
    uint32_t maxTrigerCnt;
    uint32_t trigerCalPeriod;
    uint32_t freqCalTimeCnt;
};

void led_init(ledTypedef *led, ledActivateLevel activateLevel);
void led_tick(void);

void led_set_level(ledTypedef *led, ledActivateLevel level);

void led_set_on(ledTypedef *led);
void led_set_off(ledTypedef *led);
void led_set_blink(ledTypedef *led, uint32_t onTime, uint32_t cyclePeriod, uint32_t cyclesNumber);
void led_set_freq(ledTypedef *led, uint32_t maxTrigerCnt);
void led_triger(ledTypedef *led);
#endif
