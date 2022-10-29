#pragma once

#include <stdio.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

void IRAM_ATTR gpio_isr_handler(void* arg);
void isr_init();
