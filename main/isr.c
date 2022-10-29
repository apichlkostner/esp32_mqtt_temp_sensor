#include "isr.h"
#include "led.h"
#include "config.h"

static const uint32_t toggle_led = 3;

void gpio_isr_handler(void* arg)
{
    //uint32_t gpio_num = (uint32_t)arg;
    (void)arg;
    xQueueSendFromISR(queue_led, &toggle_led, NULL);
}

void isr_init()
{
    queue_led = xQueueCreate(ISR_QUEUE_SIZE, sizeof(uint32_t));
}