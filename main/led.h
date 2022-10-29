#pragma once

#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

extern QueueHandle_t queue_led;

void led_init(void);
void LED_Control_Task(void *params);
void set_led_state(uint8_t state);