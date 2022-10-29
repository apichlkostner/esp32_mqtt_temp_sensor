#include "esp_log.h"
#include "driver/gpio.h"
#include "config.h"
#include "button.h"
#include "isr.h"


static const char *TAG = "button";

void button_init(void)
{
    ESP_LOGI(TAG, "Configure button");
    ESP_ERROR_CHECK(gpio_reset_pin(BUTTON_GPIO));
    ESP_ERROR_CHECK(gpio_set_direction(BUTTON_GPIO, GPIO_MODE_INPUT));
    ESP_ERROR_CHECK(gpio_pulldown_dis(BUTTON_GPIO));
    ESP_ERROR_CHECK(gpio_pullup_dis(BUTTON_GPIO));
    ESP_ERROR_CHECK(gpio_set_intr_type(BUTTON_GPIO, GPIO_INTR_NEGEDGE));
    ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT));
    ESP_ERROR_CHECK(gpio_isr_handler_add(BUTTON_GPIO, gpio_isr_handler, (void *)BUTTON_GPIO));
    ESP_ERROR_CHECK(gpio_intr_enable(BUTTON_GPIO));
}