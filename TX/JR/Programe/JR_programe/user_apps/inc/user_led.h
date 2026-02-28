#ifndef __USER_LED_H
#define __USER_LED_H
#include "main.h"

void user_led_init(void);

void user_led_set_error(void);
void user_led_clean_error(void);

void user_led_binding(void);
void user_led_loss_signal(void);
void user_led_connected(void);

#endif