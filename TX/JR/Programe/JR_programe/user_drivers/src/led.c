#include "led.h"

static ledTypedef headLed = {NULL};

static void led_set_activate(ledTypedef *led)
{
    if (led->activateState == LED_INACTIVATE)
    {
        led_set_level(led, (ledActivateLevel)led->activateLevel);
        led->activateState = LED_ACTIVATE;
    }
}
static void led_set_inactivate(ledTypedef *led)
{
    if (led->activateState == LED_ACTIVATE)
    {
        led_set_level(led, (ledActivateLevel)!led->activateLevel);
        led->activateState = LED_INACTIVATE;
    }
}

void led_init(ledTypedef *led, ledActivateLevel activateLevel)
{
    led->activateLevel = activateLevel;
    led->activateState = LED_INACTIVATE;
    led->status = LED_OFF;
    led_set_level(led, (ledActivateLevel)!led->activateLevel);

    led->timeCnt = 0;
    led->cycleCnt = 0;
    led->maxTrigerCnt = 0;
    led->trigerCalPeriod = 0;
    led->freqCalTimeCnt = 0;

    led->next = headLed.next;
    headLed.next = led;
}

void led_tick(void)
{
    ledTypedef *led = headLed.next;
    while (led != NULL)
    {
        switch (led->status)
        {
        case LED_OFF:
            led_set_inactivate(led);
            break;
        case LED_ON:
            led_set_activate(led);
            break;
        case LED_BLINK:
            if (led->timeCnt < led->onTime)
            {
                led_set_activate(led);
            }
            else if (led->timeCnt >= led->onTime)
            {
                led_set_inactivate(led);
            }

            if (led->timeCnt >= led->cyclePeriod)
            {
                led->timeCnt = 0;
                led->cycleCnt++;
            }
            else
            {
                led->timeCnt += LED_TICK_MS;
            }

            if (led->cyclesNumber != 0)
            {
                if (led->cycleCnt >= led->cyclesNumber)
                {
                    led->status = LED_OFF;
                }
            }
            break;
        case LED_FREQ:
            if (led->trigerCalPeriod > 0)
            {
                if (led->timeCnt < led->onTime)
                {
                    led_set_activate(led);
                }
                else if (led->timeCnt >= led->onTime)
                {
                    led_set_inactivate(led);
                }
                if (led->timeCnt >= led->cyclePeriod)
                {
                    led->timeCnt = 0;
                    led->cyclePeriod = led->trigerCalPeriod;
                }
                else if ((led->trigerCalPeriod == (uint32_t)(-1)) && (led->timeCnt >= led->onTime))
                {
                    led->timeCnt = led->onTime;
                }
                else
                {
                    led->timeCnt += LED_TICK_MS;
                }
            }
            else
            {
                if (led->trigerCnt > 0)
                {
                    led->trigerCalPeriod = (uint32_t)(-1);
                    led->cyclePeriod = led->trigerCalPeriod;
                    led->timeCnt = 0;
                }
                led_set_inactivate(led);
            }

            if (led->freqCalTimeCnt < LED_FREQ_CAL_PERIOD)
            {
                led->freqCalTimeCnt += LED_TICK_MS;
            }
            else
            {
                if (led->trigerCnt >= 2)
                {
                    led->trigerCnt = led->trigerCnt > led->maxTrigerCnt ? led->maxTrigerCnt : led->trigerCnt;
                    if (led->trigerCalPeriod == (uint32_t)(-1))
                    {
                        led->trigerCalPeriod = LED_FREQ_CAL_PERIOD / 2 - (LED_FREQ_CAL_PERIOD / 2 - 50) * (led->trigerCnt - 2) / (led->maxTrigerCnt - 2);
                        led->timeCnt = 0;
                        led->cyclePeriod = led->trigerCalPeriod;
                    }
                    else
                    {
                        led->trigerCalPeriod = LED_FREQ_CAL_PERIOD / 2 - (LED_FREQ_CAL_PERIOD / 2 - 50) * (led->trigerCnt - 2) / (led->maxTrigerCnt - 2);
                    }
                }
                else
                {
                    led->trigerCalPeriod = 0;
                    led->cyclePeriod = led->trigerCalPeriod;
                }

                led->trigerCnt = 0;
                led->freqCalTimeCnt = 0;
            }

            break;
        default:
            break;
        }

        led = led->next;
    }
}

__weak void led_set_level(ledTypedef *led, ledActivateLevel level)
{
    if (led == &headLed)
    {
        HAL_GPIO_WritePin(LED_R_GPIO_Port, LED_R_Pin, (GPIO_PinState)level);
    }
}

void led_set_on(ledTypedef *led)
{
    led->status = LED_ON;
}
void led_set_off(ledTypedef *led)
{
    led->status = LED_OFF;
}
void led_set_blink(ledTypedef *led, uint32_t onTime, uint32_t cyclePeriod, uint32_t cyclesNumber)
{
    led->timeCnt = 0;
    led->cycleCnt = 0;
    led->status = LED_BLINK;
    led->onTime = onTime;
    led->cyclePeriod = cyclePeriod;
    led->cyclesNumber = cyclesNumber;
}
void led_set_freq(ledTypedef *led, uint32_t maxTrigerCnt)
{
    led->timeCnt = 0;
    led->cycleCnt = 0;
    led->trigerCnt = 0;
    led->onTime = 10;
    led->trigerCalPeriod = 0;
    led->freqCalTimeCnt = 0;

    led->status = LED_FREQ;

    led->maxTrigerCnt = maxTrigerCnt;
}

void led_triger(ledTypedef *led)
{
    led->trigerCnt++;
}
