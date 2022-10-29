#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "driver/gpio.h"

#include "config.h"
#include "led.h"
#include "isr.h"


static uint8_t led_state = 0;
static const char* TAG = "led";

QueueHandle_t queue_led;

void led_init(void)
{
    ESP_LOGI(TAG, "Configure LED");
    ESP_ERROR_CHECK(gpio_reset_pin(LED_GPIO));
    ESP_ERROR_CHECK(gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT));
}

void set_led_state(uint8_t state)
{
    uint32_t msg = state;
    xQueueSend(queue_led, &msg, 200 / portTICK_PERIOD_MS);
}

void LED_Control_Task(void *params)
{
    uint32_t msg;
    while (true)
    {
        if (xQueueReceive(queue_led, &msg, portMAX_DELAY))
        {
            if (msg == 3)
                led_state = !led_state;
            else
                led_state = msg;
            gpio_set_level(LED_GPIO, led_state);
        }
    }
}
